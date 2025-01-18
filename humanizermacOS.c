#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <Carbon/Carbon.h>

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
#define SUPPORTED_EXTENSIONS ".txt\0"

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
    system("clear");
}

// display the progress bar in the console
void display_progress_bar(int current, int total) {
    int progress = (int)((double)current / total * PROGRESS_BAR_WIDTH);
    printf(ANSI_COLOR_BLUE "\rpreparing: [" ANSI_COLOR_RESET);
    // draw progress bar with green '#' for progress
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < progress) {
            printf(ANSI_COLOR_GREEN "#" ANSI_COLOR_RESET);
        } else {
            printf(" ");
        }
    }
    // print the percentage completion
    printf("] %d%%", (int)((double)current / total * 100));
    fflush(stdout);
}

// update typing statistics
void update_typing_stats(TypingStats* stats) {
    stats->elapsed_time = difftime(time(NULL), stats->start_time);
    if (stats->elapsed_time > 0) {
        stats->current_wpm = (stats->words_typed / (stats->elapsed_time / 60.0));
    }
}

// display typing statistics
void display_typing_stats(TypingStats* stats) {
    printf(ANSI_COLOR_BLUE "\rcurrent wpm: %.1f | chars typed: %d | words: %d | time: %.1fs" ANSI_COLOR_RESET,
           stats->current_wpm, stats->chars_typed, stats->words_typed, stats->elapsed_time);
    fflush(stdout);
}

// check if the file extension is supported
bool is_supported_file_type(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    return ext && strcmp(ext, ".txt") == 0;
}

// load the content of a file
bool load_file_content(const char* filepath, TypingSimulator* sim) {
    if (!is_supported_file_type(filepath)) {
        printf(ANSI_COLOR_RED "error: unsupported file type\n" ANSI_COLOR_RESET);
        return false;
    }

    FILE* file = fopen(filepath, "r");
    if (!file) {
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

    if (file_size > MAX_TEXT_LENGTH) {
        printf(ANSI_COLOR_RED "error: file too large\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    sim->text = (char*)malloc(file_size + 1);
    if (!sim->text) {
        printf(ANSI_COLOR_RED "error: memory allocation failed\n" ANSI_COLOR_RESET);
        fclose(file);
        return false;
    }

    fread(sim->text, 1, file_size, file);
    sim->text[file_size] = '\0';
    sim->length = file_size;

    fclose(file);
    return true;
}

// simulate typing on macOS using CoreGraphics
void simulate_typing(TypingSimulator* sim) {
    clear_screen();
    printf("preparing to type...\n");
    printf("switch to your target window now!\n\n");

    for (int i = 0; i <= 100; i++) {
        display_progress_bar(i, 100);
        usleep(30000); // 30ms
    }
    printf("\n\n");

    sim->stats.start_time = time(NULL);
    bool in_word = false;

    double time_per_word = 60.0 / (BASE_WPM + WPM_ADJUSTMENT);
    double chars_per_word = 5.0;
    double time_per_char = time_per_word / chars_per_word;

    for (size_t i = 0; i < sim->length; i++) {
        if (isspace(sim->text[i])) {
            if (in_word) {
                sim->stats.words_typed++;
                in_word = false;
            }
        } else {
            in_word = true;
        }

        CGEventRef key_down = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)sim->text[i], true);
        CGEventRef key_up = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)sim->text[i], false);
        CGEventPost(kCGHIDEventTap, key_down);
        CGEventPost(kCGHIDEventTap, key_up);
        CFRelease(key_down);
        CFRelease(key_up);

        sim->stats.chars_typed++;
        update_typing_stats(&sim->stats);
        display_typing_stats(&sim->stats);

        usleep((useconds_t)(time_per_char * 1000000));
    }

    if (in_word) {
        sim->stats.words_typed++;
    }

    printf("\n\ntyping complete!\n");
}

// handle input choice
void handle_input_choice(TypingSimulator* sim) {
    printf("\nselect input method:\n");
    printf("1. type/paste text\n");
    printf("2. drag and drop file\n");
    printf("3. exit\n");
    printf("\nenter your choice (1-3): ");

    char choice;
    scanf(" %c", &choice);
    getchar();

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

// initialize simulator
void init_simulator(TypingSimulator* sim) {
    sim->text = NULL;
    sim->length = 0;
    sim->stats.chars_typed = 0;
    sim->stats.words_typed = 0;
    sim->stats.current_wpm = 0;
    sim->stats.elapsed_time = 0;
}

// cleanup simulator
void cleanup_simulator(TypingSimulator* sim) {
    if (sim->text) {
        free(sim->text);
        sim->text = NULL;
    }
}

// main function
int main() {
    TypingSimulator sim;
    init_simulator(&sim);
    clear_screen();
    printf(ANSI_COLOR_BLUE "humanizer typing simulator (macOS)\n" ANSI_COLOR_RESET);
    printf("============================\n");
    printf("base wpm: %d (actual wpm: %d)\n", BASE_WPM, BASE_WPM + WPM_ADJUSTMENT);
    printf("supported file types: txt\n");

    while (1) {
        handle_input_choice(&sim);
        cleanup_simulator(&sim);
        init_simulator(&sim);
        printf("\n");
    }

    return 0;
}
