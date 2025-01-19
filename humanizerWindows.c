#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// stop the compiler from complaining about unsafe string functions
#define _CRT_SECURE_NO_WARNINGS

// ansi color codes for output formatting
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// constants defining limits for text and file handling
#define MAX_TEXT_LENGTH 1000000
#define MAX_LINE_LENGTH 1000
#define PROGRESS_BAR_WIDTH 50

// base words per minute (wpm) and adjustment for accuracy
#define BASE_WPM 100
#define WPM_ADJUSTMENT 15 // adjustment for actual typing speed based on accuracy

#define MAX_PATH_LENGTH 260
#define SUPPORTED_EXTENSIONS ".txt\0.doc\0.docx\0"

// structure to store typing statistics
typedef struct {
    double current_wpm;    // current words per minute
    size_t chars_typed;    // number of characters typed
    size_t words_typed;    // number of words typed
    time_t start_time;     // start time for typing simulation
    double elapsed_time;   // elapsed time since typing started
} TypingStats;

// structure for the typing simulator, including text and stats
typedef struct {
    char* text;            // pointer to the text being typed
    size_t length;         // length of the text
    TypingStats stats;     // typing statistics for the simulation
} TypingSimulator;

// function prototypes
void init_simulator(TypingSimulator* sim);
void cleanup_simulator(TypingSimulator* sim);
void display_progress_bar(size_t current, size_t total);
bool load_file_content(const char* filepath, TypingSimulator* sim);
void update_typing_stats(TypingStats* stats);
void display_typing_stats(TypingStats* stats);
void simulate_typing(TypingSimulator* sim);
bool is_supported_file_type(const char* filepath);
void handle_input_choice(TypingSimulator* sim);
void handle_manual_input(TypingSimulator* sim);
void handle_file_drop(TypingSimulator* sim);
void clear_screen(void);
void clear_input_buffer(void);

// clear the console screen
void clear_screen(void) {
    system("cls");
}

// clear input buffer safely
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// display the progress bar in the console
void display_progress_bar(size_t current, size_t total) {
    size_t progress = (size_t)((double)current / total * PROGRESS_BAR_WIDTH);
    printf(ANSI_COLOR_BLUE "\rpreparing: [" ANSI_COLOR_RESET);

    for (size_t i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < progress) {
            printf(ANSI_COLOR_GREEN "#" ANSI_COLOR_RESET);
        }
        else {
            printf(" ");
        }
    }

    printf("] %zu%%", (size_t)((double)current / total * 100));
    fflush(stdout);
}

// update typing statistics such as current words per minute (wpm)
void update_typing_stats(TypingStats* stats) {
    stats->elapsed_time = difftime(time(NULL), stats->start_time);
    if (stats->elapsed_time > 0) {
        stats->current_wpm = (double)stats->words_typed / (stats->elapsed_time / 60.0);
    }
}

// display the current typing statistics on the console
void display_typing_stats(TypingStats* stats) {
    printf(ANSI_COLOR_BLUE "\rcurrent wpm: %.1f | chars typed: %zu | words: %zu | time: %.1fs" ANSI_COLOR_RESET,
        stats->current_wpm, stats->chars_typed, stats->words_typed, stats->elapsed_time);
    fflush(stdout);
}

// handle the manual input mode where the user types/pastes text
void handle_manual_input(TypingSimulator* sim) {
    clear_screen();
    printf("\nenter your text (type 'END' on a new line to finish):\n");

    char* temp_buffer = (char*)malloc(MAX_TEXT_LENGTH);
    if (!temp_buffer) {
        printf(ANSI_COLOR_RED "error: memory allocation failed\n" ANSI_COLOR_RESET);
        return;
    }
    memset(temp_buffer, 0, MAX_TEXT_LENGTH);

    size_t pos = 0;
    char line[MAX_LINE_LENGTH];

    while (pos < MAX_TEXT_LENGTH - 1) {
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        if (strcmp(line, "END\n") == 0) {
            break;
        }

        size_t line_len = strlen(line);
        if (pos + line_len >= MAX_TEXT_LENGTH) {
            printf(ANSI_COLOR_RED "error: text too long\n" ANSI_COLOR_RESET);
            free(temp_buffer);
            return;
        }

        errno_t err = strcpy_s(temp_buffer + pos, MAX_TEXT_LENGTH - pos, line);
        if (err != 0) {
            printf(ANSI_COLOR_RED "error: failed to copy text (error: %d)\n" ANSI_COLOR_RESET, err);
            free(temp_buffer);
            return;
        }

        pos += line_len;
    }

    if (pos == 0) {
        printf(ANSI_COLOR_RED "error: no text entered\n" ANSI_COLOR_RESET);
        free(temp_buffer);
        return;
    }

    sim->text = temp_buffer;
    sim->length = pos;

    simulate_typing(sim);
}

// check if the file extension is supported for loading
bool is_supported_file_type(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    if (!ext) {
        return false;
    }

    char supported[sizeof(SUPPORTED_EXTENSIONS)];
    errno_t err = strcpy_s(supported, sizeof(supported), SUPPORTED_EXTENSIONS);
    if (err != 0) {
        return false;
    }

    char* context = NULL;
    char* token = strtok_s(supported, "\0", &context);

    while (token != NULL) {
        if (_stricmp(ext, token) == 0) {
            return true;
        }
        token = strtok_s(NULL, "\0", &context);
    }

    return false;
}

// load the content of a file into the simulator's text buffer
bool load_file_content(const char* filepath, TypingSimulator* sim) {
    if (!is_supported_file_type(filepath)) {
        printf(ANSI_COLOR_RED "error: unsupported file type\n" ANSI_COLOR_RESET);
        return false;
    }

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filepath, "r");
    if (err != 0 || file == NULL) {
        printf(ANSI_COLOR_RED "error: could not open file\n" ANSI_COLOR_RESET);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size == 0) {
        printf(ANSI_COLOR_RED "error: file is empty\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    if (file_size > MAX_TEXT_LENGTH - 1) {
        printf(ANSI_COLOR_RED "error: file too large\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    char* temp_buffer = (char*)malloc(file_size + 1);
    if (!temp_buffer) {
        printf(ANSI_COLOR_RED "error: memory allocation failed\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    size_t read_size = fread(temp_buffer, 1, file_size, file);
    if (read_size != file_size) {
        printf(ANSI_COLOR_RED "error: failed to read file\n" ANSI_COLOR_RESET);
        free(temp_buffer);
        fclose(file);
        return false;
    }

    temp_buffer[read_size] = '\0';
    sim->text = temp_buffer;
    sim->length = read_size;

    fclose(file);
    return true;
}

// simulate the typing process, typing each character with delay
void simulate_typing(TypingSimulator* sim) {
    clear_screen();
    printf("preparing to type...\n");
    printf("switch to your target window now!\n\n");

    for (size_t i = 0; i <= 100; i++) {
        display_progress_bar(i, 100);
        Sleep(30);
    }
    printf("\n\n");

    sim->stats.start_time = time(NULL);
    bool in_word = false;

    double time_per_word = 60.0 / (BASE_WPM + WPM_ADJUSTMENT);
    double chars_per_word = 5.0;
    double time_per_char = time_per_word / chars_per_word;

    for (size_t i = 0; i < sim->length; i++) {
        if (isspace((unsigned char)sim->text[i])) {
            if (in_word) {
                sim->stats.words_typed++;
                in_word = false;
            }
        }
        else {
            in_word = true;
        }

        SHORT vk = VkKeyScanA(sim->text[i]);
        WORD vk_code = vk & 0xFF;
        WORD shift_state = (vk >> 8) & 0xFF;

        INPUT inputs[4] = { 0 };
        int input_count = 0;

        if (shift_state & 1) {
            inputs[input_count].type = INPUT_KEYBOARD;
            inputs[input_count].ki.wVk = VK_SHIFT;
            input_count++;
        }

        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = vk_code;
        input_count++;

        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = vk_code;
        inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
        input_count++;

        if (shift_state & 1) {
            inputs[input_count].type = INPUT_KEYBOARD;
            inputs[input_count].ki.wVk = VK_SHIFT;
            inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
            input_count++;
        }

        SendInput(input_count, inputs, sizeof(INPUT));
        sim->stats.chars_typed++;

        update_typing_stats(&sim->stats);
        display_typing_stats(&sim->stats);

        Sleep((DWORD)(time_per_char * 1000));
    }

    if (in_word) {
        sim->stats.words_typed++;
    }

    printf("\n\ntyping complete!\n");
}

// handle the file drop or file path input from the user
void handle_file_drop(TypingSimulator* sim) {
    clear_screen();
    char filepath[MAX_PATH_LENGTH];
    printf("enter file path or drag file here: ");

    if (!fgets(filepath, MAX_PATH_LENGTH, stdin)) {
        printf(ANSI_COLOR_RED "error: failed to read input\n" ANSI_COLOR_RESET);
        return;
    }

    filepath[strcspn(filepath, "\n")] = 0;

    if (strcmp(filepath, "exit") == 0) {
        exit(0);
    }

    // Remove quotes if present
    if (filepath[0] == '"' && strlen(filepath) > 2) {
        size_t len = strlen(filepath);
        memmove(filepath, filepath + 1, len - 2);
        filepath[len - 2] = '\0';
    }

    if (load_file_content(filepath, sim)) {
        simulate_typing(sim);
    }
}

// handle user choice for input method (manual text or file)
void handle_input_choice(TypingSimulator* sim) {
    printf("\nselect input method:\n");
    printf("1. type/paste text\n");
    printf("2. drag and drop file\n");
    printf("3. exit\n");
    printf("\nenter your choice (1-3): ");

    char choice;
    if (scanf_s(" %c", &choice, 1) != 1) {
        printf(ANSI_COLOR_RED "error: invalid input\n" ANSI_COLOR_RESET);
        clear_input_buffer();
        return;
    }

    clear_input_buffer();

    switch (choice) {
    case '1':
        handle_manual_input(sim);
        break;
    case '2':
        handle_file_drop(sim);
        break;
    case '3':
        exit(0);
    default:
        printf(ANSI_COLOR_RED "invalid choice. please try again.\n" ANSI_COLOR_RESET);
    }
}

// initialize the typing simulator with default values
void init_simulator(TypingSimulator* sim) {
    sim->text = NULL;
    sim->length = 0;
    sim->stats.chars_typed = 0;
    sim->stats.words_typed = 0;
    sim->stats.current_wpm = 0.0;
    sim->stats.elapsed_time = 0.0;
}

// clean up resources used by the simulator
void cleanup_simulator(TypingSimulator* sim) {
    if (sim->text) {
        free(sim->text);
        sim->text = NULL;
    }
    sim->length = 0;
}

int main(void) {
    // Enable ANSI escape sequences for color output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("error: could not get console handle\n");
        return 1;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        printf("error: could not get console mode\n");
        return 1;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        printf("warning: could not enable virtual terminal processing\n");
    }

    TypingSimulator sim;
    init_simulator(&sim);
    clear_screen();

printf(ANSI_COLOR_BLUE "humanizer typing simulator\n" ANSI_COLOR_RESET);
    printf("============================\n");
    printf("base wpm: %d (actual wpm: %d)\n", BASE_WPM, BASE_WPM + WPM_ADJUSTMENT);
    printf("supported file types: txt, doc, docx\n");

    while (1) {
        handle_input_choice(&sim);
        cleanup_simulator(&sim);
        init_simulator(&sim);
        printf("\n");
    }

    // Cleanup before exit (though this won't be reached due to while(1))
    cleanup_simulator(&sim);
    return 0;
}
