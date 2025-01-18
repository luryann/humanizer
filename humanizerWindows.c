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
#define BASE_WPM 85
#define WPM_ADJUSTMENT 15 // adjustment for actual typing speed based on accuracy

#define MAX_PATH_LENGTH 260
#define SUPPORTED_EXTENSIONS ".txt\0.doc\0.docx\0"

// structure to store typing statistics
typedef struct {
    double current_wpm;  // current words per minute
    int chars_typed;     // number of characters typed
    int words_typed;     // number of words typed
    time_t start_time;   // start time for typing simulation
    double elapsed_time; // elapsed time since typing started
} TypingStats;

// structure for the typing simulator, including text and stats
typedef struct {
    char* text;          // pointer to the text being typed
    size_t length;       // length of the text
    TypingStats stats;   // typing statistics for the simulation
} TypingSimulator;

// function prototypes
void init_simulator(TypingSimulator* sim);
void cleanup_simulator(TypingSimulator* sim);
void display_progress_bar(int current, int total);
bool load_file_content(const char* filepath, TypingSimulator* sim);
void update_typing_stats(TypingStats* stats);
void display_typing_stats(TypingStats* stats);
void simulate_typing(TypingSimulator* sim);
bool is_supported_file_type(const char* filepath);
void handle_input_choice(TypingSimulator* sim);
void handle_manual_input(TypingSimulator* sim);
void handle_file_drop(TypingSimulator* sim);
void clear_screen(void);

// clear the console screen
void clear_screen() {
    system("cls");
}

// display the progress bar in the console
void display_progress_bar(int current, int total) {
    int progress = (int)((double)current / total * PROGRESS_BAR_WIDTH);
    printf(ANSI_COLOR_BLUE "\rpreparing: [" ANSI_COLOR_RESET);
    // draw progress bar with green '#' for progress
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < progress) {
            printf(ANSI_COLOR_GREEN "#" ANSI_COLOR_RESET);
        }
        else {
            printf(" ");
        }
    }
    // print the percentage completion
    printf("] %d%%", (int)((double)current / total * 100));
    fflush(stdout);
}

// update typing statistics such as current words per minute (wpm)
void update_typing_stats(TypingStats* stats) {
    stats->elapsed_time = difftime(time(NULL), stats->start_time);
    if (stats->elapsed_time > 0) {
        stats->current_wpm = (stats->words_typed / (stats->elapsed_time / 60.0));
    }
}

// display the current typing statistics on the console
void display_typing_stats(TypingStats* stats) {
    printf(ANSI_COLOR_BLUE "\rcurrent wpm: %.1f | chars typed: %d | words: %d | time: %.1fs" ANSI_COLOR_RESET,
        stats->current_wpm, stats->chars_typed, stats->words_typed, stats->elapsed_time);
    fflush(stdout);
}

// handle the manual input mode where the user types/pastes text
void handle_manual_input(TypingSimulator* sim) {
    clear_screen();  // Clear screen before displaying manual input prompt
    printf("\nenter your text (type 'END' on a new line to finish):\n");

    char* temp_buffer = (char*)malloc(MAX_TEXT_LENGTH); // buffer for storing user input text
    if (!temp_buffer) {
        printf(ANSI_COLOR_RED "error: memory allocation failed\n" ANSI_COLOR_RESET);
        return;
    }

    int pos = 0;
    while (pos < MAX_TEXT_LENGTH - 1) {
        char line[MAX_LINE_LENGTH];
        if (!fgets(line, MAX_LINE_LENGTH, stdin)) break; // read user input
        if (strcmp(line, "END\n") == 0) break; // check if user typed 'END' to stop input

        int line_len = strlen(line);
        if (pos + line_len >= MAX_TEXT_LENGTH) {
            printf(ANSI_COLOR_RED "error: text too long\n" ANSI_COLOR_RESET);
            free(temp_buffer);
            return;
        }

        strcpy(temp_buffer + pos, line); // append line to buffer
        pos += line_len;
    }

    if (pos == 0) {
        printf(ANSI_COLOR_RED "error: no text entered\n" ANSI_COLOR_RESET);
        free(temp_buffer);
        return;
    }

    sim->text = temp_buffer;
    sim->length = pos;

    simulate_typing(sim); // simulate typing with the entered text
}

// check if the file extension is supported for loading
bool is_supported_file_type(const char* filepath) {
    char* ext = strrchr(filepath, '.');
    if (!ext) return false; // no file extension found

    char* supported = strdup(SUPPORTED_EXTENSIONS); // supported file extensions list
    char* token = strtok(supported, "\0"); // split extensions

    while (token != NULL) {
        if (_stricmp(ext, token) == 0) { // compare file extension with supported types
            free(supported);
            return true;
        }
        token = strtok(NULL, "\0");
    }

    free(supported);
    return false;
}

// load the content of a file into the simulator's text buffer
bool load_file_content(const char* filepath, TypingSimulator* sim) {
    if (!is_supported_file_type(filepath)) { // check if file type is supported
        printf(ANSI_COLOR_RED "error: unsupported file type\n" ANSI_COLOR_RESET);
        return false;
    }

    FILE* file = fopen(filepath, "r"); // open the file for reading
    if (!file) {
        printf(ANSI_COLOR_RED "error: could not open file\n" ANSI_COLOR_RESET);
        return false;
    }

    fseek(file, 0, SEEK_END); // move to end of file to get size
    long file_size = ftell(file);
    rewind(file); // rewind to the start of the file

    if (file_size == 0) {
        printf(ANSI_COLOR_RED "error: file is empty\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    if (file_size > MAX_TEXT_LENGTH) { // check if file size exceeds max allowed length
        printf(ANSI_COLOR_RED "error: file too large\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    sim->text = (char*)malloc(file_size + 1); // allocate memory for file content
    if (!sim->text) {
        printf(ANSI_COLOR_RED "error: memory allocation failed\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    size_t read_size = fread(sim->text, 1, file_size, file); // read file content
    sim->text[read_size] = '\0'; // null-terminate the text
    sim->length = read_size;

    fclose(file);
    return true;
}

// simulate the typing process, typing each character with delay
void simulate_typing(TypingSimulator* sim) {
    clear_screen();  // Clear screen before displaying typing progress
    printf("preparing to type...\n");
    printf("switch to your target window now!\n\n");

    // display progress bar as preparation for typing
    for (int i = 0; i <= 100; i++) {
        display_progress_bar(i, 100);
        Sleep(30); // wait for 30ms (3 seconds total)
    }
    printf("\n\n");

    sim->stats.start_time = time(NULL); // set the start time for typing
    bool in_word = false;

    // define typing speed and time per character
    double time_per_word = 60.0 / (BASE_WPM + WPM_ADJUSTMENT);
    double chars_per_word = 5.0;
    double time_per_char = time_per_word / chars_per_word;

    // simulate typing each character from the loaded text
    for (size_t i = 0; i < sim->length; i++) {
        if (isspace(sim->text[i])) { // check for word boundaries
            if (in_word) {
                sim->stats.words_typed++; // increment word count when space is encountered
                in_word = false;
            }
        }
        else {
            in_word = true;
        }

        // simulate key press for the current character
        SHORT vk = VkKeyScanA(sim->text[i]);
        WORD vk_code = vk & 0xFF;
        WORD shift_state = (vk >> 8) & 0xFF;

        INPUT inputs[4] = { 0 };
        int input_count = 0;

        // simulate SHIFT key press if required
        if (shift_state & 1) {
            inputs[input_count].type = INPUT_KEYBOARD;
            inputs[input_count].ki.wVk = VK_SHIFT;
            input_count++;
        }

        // simulate key press for character
        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = vk_code;
        input_count++;

        // simulate key release for character
        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = vk_code;
        inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
        input_count++;

        // simulate SHIFT key release if required
        if (shift_state & 1) {
            inputs[input_count].type = INPUT_KEYBOARD;
            inputs[input_count].ki.wVk = VK_SHIFT;
            inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
            input_count++;
        }

        SendInput(input_count, inputs, sizeof(INPUT)); // send input to system
        sim->stats.chars_typed++; // increment typed character count

        update_typing_stats(&sim->stats); // update statistics
        display_typing_stats(&sim->stats); // display statistics

        Sleep((DWORD)(time_per_char * 1000)); // delay between characters
    }

    if (in_word) {
        sim->stats.words_typed++; // increment word count if last word wasn't followed by space
    }

    printf("\n\ntyping complete!\n");
}

// handle the file drop or file path input from the user
void handle_file_drop(TypingSimulator* sim) {
    clear_screen();  // Clear screen before asking for file input
    char filepath[MAX_PATH_LENGTH];
    printf("enter file path or drag file here: ");
    fgets(filepath, MAX_PATH_LENGTH, stdin);

    // remove newline character
    filepath[strcspn(filepath, "\n")] = 0;

    if (strcmp(filepath, "exit") == 0) {
        exit(0); // exit the program if user typed "exit"
    }

    // remove quotes from filepath if present
    if (filepath[0] == '"') {
        memmove(filepath, filepath + 1, strlen(filepath));
        filepath[strlen(filepath) - 1] = '\0';
    }

    // load the file content and simulate typing
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
    scanf(" %c", &choice);
    getchar(); // consume newline after choice

    switch (choice) {
    case '1':
        handle_manual_input(sim); // handle manual input
        break;
    case '2':
        handle_file_drop(sim); // handle file drop
        break;
    case '3':
        exit(0); // exit the program
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
    sim->stats.current_wpm = 0;
    sim->stats.elapsed_time = 0;
}

// clean up resources used by the simulator
void cleanup_simulator(TypingSimulator* sim) {
    if (sim->text) {
        free(sim->text); // free allocated memory for text
        sim->text = NULL;
    }
}

// main function
int main() {
    // enable ansi escape sequences for color in console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    TypingSimulator sim;
    init_simulator(&sim);
    clear_screen();
    printf(ANSI_COLOR_BLUE "humanizer typing simulator\n" ANSI_COLOR_RESET);
    printf("============================\n");
    printf("base wpm: %d (actual wpm: %d)\n", BASE_WPM, BASE_WPM + WPM_ADJUSTMENT);
    printf("supported file types: txt, doc, docx\n");

    // main program loop
    while (1) {
        handle_input_choice(&sim); // handle user input choice
        cleanup_simulator(&sim); // cleanup after each session
        init_simulator(&sim); // reinitialize for next session
        printf("\n");
    }

    return 0;
}
