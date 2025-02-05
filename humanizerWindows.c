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

// Stop the compiler from complaining about unsafe string functions
#define _CRT_SECURE_NO_WARNINGS

// ANSI color codes for output formatting
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Constants defining limits for text and file handling
#define MAX_TEXT_LENGTH 1000000
#define MAX_LINE_LENGTH 1000
#define PROGRESS_BAR_WIDTH 50

// Base words per minute (WPM) and adjustment for accuracy
#define BASE_WPM 100
#define WPM_ADJUSTMENT 15 // Adjustment for actual typing speed based on accuracy

#define MAX_PATH_LENGTH 260
#define SUPPORTED_EXTENSIONS ".txt\0.doc\0.docx\0"

// Structure to store typing statistics
typedef struct {
    double current_wpm;    // Current words per minute
    size_t chars_typed;    // Number of characters typed
    size_t words_typed;    // Number of words typed
    time_t start_time;     // Start time for typing simulation
    double elapsed_time;   // Elapsed time since typing started
} TypingStats;

// Structure for the typing simulator, including text and stats
typedef struct {
    char* text;            // Pointer to the text being typed
    size_t length;         // Length of the text
    TypingStats stats;     // Typing statistics for the simulation
} TypingSimulator;

// Function prototypes
void InitializeSimulator(TypingSimulator* simulator);
void CleanupSimulator(TypingSimulator* simulator);
void DisplayProgressBar(size_t current, size_t total);
bool LoadFileContent(const char* filepath, TypingSimulator* simulator);
void UpdateTypingStats(TypingStats* stats);
void DisplayTypingStats(TypingStats* stats);
void SimulateTyping(TypingSimulator* simulator);
bool IsSupportedFileType(const char* filepath);
void HandleInputChoice(TypingSimulator* simulator);
void HandleManualInput(TypingSimulator* simulator);
void HandleFileDrop(TypingSimulator* simulator);
void ClearScreen(void);
void ClearInputBuffer(void);

// Clear the console screen
void ClearScreen(void) {
    system("cls");
}

// Clear input buffer safely
void ClearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Display the progress bar in the console
void DisplayProgressBar(size_t current, size_t total) {
    size_t progress = (size_t)((double)current / total * PROGRESS_BAR_WIDTH);
    printf(ANSI_COLOR_BLUE "\rPreparing: [" ANSI_COLOR_RESET);

    for (size_t i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < progress) {
            printf(ANSI_COLOR_GREEN "#" ANSI_COLOR_RESET);
        } else {
            printf(" ");
        }
    }

    printf("] %zu%%", (size_t)((double)current / total * 100));
    fflush(stdout);
}

// Update typing statistics such as current words per minute (WPM)
void UpdateTypingStats(TypingStats* stats) {
    stats->elapsed_time = difftime(time(NULL), stats->start_time);
    if (stats->elapsed_time > 0) {
        stats->current_wpm = (double)stats->words_typed / (stats->elapsed_time / 60.0);
    }
}

// Display the current typing statistics on the console
void DisplayTypingStats(TypingStats* stats) {
    printf(ANSI_COLOR_BLUE "\rCurrent WPM: %.1f | Chars Typed: %zu | Words: %zu | Time: %.1fs" ANSI_COLOR_RESET,
           stats->current_wpm, stats->chars_typed, stats->words_typed, stats->elapsed_time);
    fflush(stdout);
}

// Handle the manual input mode where the user types/pastes text
void HandleManualInput(TypingSimulator* simulator) {
    ClearScreen();
    printf("\nEnter your text (type 'END' on a new line to finish):\n");

    char* temp_buffer = (char*)malloc(MAX_TEXT_LENGTH);
    if (!temp_buffer) {
        printf(ANSI_COLOR_RED "Error: Memory allocation failed\n" ANSI_COLOR_RESET);
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
            printf(ANSI_COLOR_RED "Error: Text too long\n" ANSI_COLOR_RESET);
            free(temp_buffer);
            return;
        }

        errno_t err = strcpy_s(temp_buffer + pos, MAX_TEXT_LENGTH - pos, line);
        if (err != 0) {
            printf(ANSI_COLOR_RED "Error: Failed to copy text (error: %d)\n" ANSI_COLOR_RESET, err);
            free(temp_buffer);
            return;
        }

        pos += line_len;
    }

    if (pos == 0) {
        printf(ANSI_COLOR_RED "Error: No text entered\n" ANSI_COLOR_RESET);
        free(temp_buffer);
        return;
    }

    simulator->text = temp_buffer;
    simulator->length = pos;

    SimulateTyping(simulator);
}

// Check if the file extension is supported for loading
bool IsSupportedFileType(const char* filepath) {
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

// Load the content of a file into the simulator's text buffer
bool LoadFileContent(const char* filepath, TypingSimulator* simulator) {
    if (!IsSupportedFileType(filepath)) {
        printf(ANSI_COLOR_RED "Error: Unsupported file type\n" ANSI_COLOR_RESET);
        return false;
    }

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filepath, "r");
    if (err != 0 || file == NULL) {
        printf(ANSI_COLOR_RED "Error: Could not open file\n" ANSI_COLOR_RESET);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size == 0) {
        printf(ANSI_COLOR_RED "Error: File is empty\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    if (file_size > MAX_TEXT_LENGTH - 1) {
        printf(ANSI_COLOR_RED "Error: File too large\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    char* temp_buffer = (char*)malloc(file_size + 1);
    if (!temp_buffer) {
        printf(ANSI_COLOR_RED "Error: Memory allocation failed\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    size_t read_size = fread(temp_buffer, 1, file_size, file);
    if (read_size != file_size) {
        printf(ANSI_COLOR_RED "Error: Failed to read file\n" ANSI_COLOR_RESET);
        free(temp_buffer);
        fclose(file);
        return false;
    }

    temp_buffer[read_size] = '\0';
    simulator->text = temp_buffer;
    simulator->length = read_size;

    fclose(file);
    return true;
}

// Simulate the typing process, typing each character with delay
void SimulateTyping(TypingSimulator* simulator) {
    ClearScreen();
    printf("Preparing to type...\n");
    printf("Switch to your target window now!\n\n");

    for (size_t i = 0; i <= 100; i++) {
        DisplayProgressBar(i, 100);
        Sleep(30);
    }
    printf("\n\n");

    simulator->stats.start_time = time(NULL);
    bool in_word = false;

    double time_per_word = 60.0 / (BASE_WPM + WPM_ADJUSTMENT);
    double chars_per_word = 5.0;
    double time_per_char = time_per_word / chars_per_word;

    for (size_t i = 0; i < simulator->length; i++) {
        if (isspace((unsigned char)simulator->text[i])) {
            if (in_word) {
                simulator->stats.words_typed++;
                in_word = false;
            }
        } else {
            in_word = true;
        }

        SHORT vk = VkKeyScanA(simulator->text[i]);
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
        simulator->stats.chars_typed++;

        UpdateTypingStats(&simulator->stats);
        DisplayTypingStats(&simulator->stats);

        Sleep((DWORD)(time_per_char * 1000));
    }

    if (in_word) {
        simulator->stats.words_typed++;
    }

    printf("\n\nTyping complete!\n");
}

// Handle the file drop or file path input from the user
void HandleFileDrop(TypingSimulator* simulator) {
    ClearScreen();
    char filepath[MAX_PATH_LENGTH];
    printf("Enter file path or drag file here: ");

    if (!fgets(filepath, MAX_PATH_LENGTH, stdin)) {
        printf(ANSI_COLOR_RED "Error: Failed to read input\n" ANSI_COLOR_RESET);
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

    if (LoadFileContent(filepath, simulator)) {
        SimulateTyping(simulator);
    }
}

// Handle user choice for input method (manual text or file)
void HandleInputChoice(TypingSimulator* simulator) {
    printf("\nSelect input method:\n");
    printf("1. Type/Paste text\n");
    printf("2. Drag and drop file\n");
    printf("3. Exit\n");
    printf("\nEnter your choice (1-3): ");

    char choice;
    if (scanf_s(" %c", &choice, 1) != 1) {
        printf(ANSI_COLOR_RED "Error: Invalid input\n" ANSI_COLOR_RESET);
        ClearInputBuffer();
        return;
    }

    ClearInputBuffer();

    switch (choice) {
        case '1':
            HandleManualInput(simulator);
            break;
        case '2':
            HandleFileDrop(simulator);
            break;
        case '3':
            exit(0);
        default:
            printf(ANSI_COLOR_RED "Invalid choice. Please try again.\n" ANSI_COLOR_RESET);
    }
}

// Initialize the typing simulator with default values
void InitializeSimulator(TypingSimulator* simulator) {
    simulator->text = NULL;
    simulator->length = 0;
    simulator->stats.chars_typed = 0;
    simulator->stats.words_typed = 0;
    simulator->stats.current_wpm = 0.0;
    simulator->stats.elapsed_time = 0.0;
}

// Clean up resources used by the simulator
void CleanupSimulator(TypingSimulator* simulator) {
    if (simulator->text) {
        free(simulator->text);
        simulator->text = NULL;
    }
    simulator->length = 0;
}

int main(void) {
    // Enable ANSI escape sequences for color output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        printf("Error: Could not get console handle\n");
        return 1;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        printf("Error: Could not get console mode\n");
        return 1;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        printf("Warning: Could not enable virtual terminal processing\n");
    }

    TypingSimulator simulator;
    InitializeSimulator(&simulator);
    ClearScreen();

    printf(ANSI_COLOR_BLUE "Humanizer Typing Simulator\n" ANSI_COLOR_RESET);
    printf("============================\n");
    printf("Base WPM: %d (Actual WPM: %d)\n", BASE_WPM, BASE_WPM + WPM_ADJUSTMENT);
    printf("Supported file types: txt, doc, docx\n");

    while (1) {
        HandleInputChoice(&simulator);
        CleanupSimulator(&simulator);
        InitializeSimulator(&simulator);
        printf("\n");
    }

    // Cleanup before exit (though this won't be reached due to while(1))
    CleanupSimulator(&simulator);
    return 0;
}
