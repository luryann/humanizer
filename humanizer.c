#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

#pragma comment(lib, "user32.lib")

// Constants
#define MAX_TEXT_LENGTH 10000
#define MAX_LINE_LENGTH 1000
#define MAX_HISTORY_LENGTH 100
#define DEFAULT_INDENT_SPACES 3
#define KEYBOARD_LAYOUT_SIZE 26

// Enums
typedef enum {
    RUNNING,
    PAUSED,
    STOPPED
} TypingState;

typedef enum {
    NONE,
    BULLET,
    NUMBERED
} ListType;

// Structures
typedef struct {
    bool in_list;
    int current_level;
    ListType list_type;
    int number_counter[10];
    int last_indent;
} ListState;

typedef struct {
    double base_wpm;
    double typo_probability;
    double speed_variation;
    double min_delay;
    double max_delay;
    double newline_multiplier;
    time_t start_time;    // Track session time
    int chars_typed;      // Track total characters
    int words_typed;      // Track total words
} Settings;

typedef struct {
    TypingState state;
    int typo_count;
    char* typed_output;
    int output_length;
    int* error_history;
    int history_length;
    ListState list_state;
    int word_count;
    double actual_typing_time;
    Settings settings;
} TypingSimulator;

// Keyboard layout for typos
typedef struct {
    char key;
    const char* adjacent;
} KeyboardLayout;

KeyboardLayout keyboard_layout[KEYBOARD_LAYOUT_SIZE] = {
    {'a', "qwsz"},
    {'b', "vghn"},
    {'c', "xdfv"},
    {'d', "erfcxs"},
    {'e', "wrsdf"},
    {'f', "rtgvcd"},
    {'g', "tyhbvf"},
    {'h', "yujnbg"},
    {'i', "ujklo"},
    {'j', "uikmnh"},
    {'k', "ioljm"},
    {'l', "opk"},
    {'m', "njk"},
    {'n', "bhjm"},
    {'o', "ipkl"},
    {'p', "o[l"},
    {'q', "was"},
    {'r', "etdfg"},
    {'s', "awedxz"},
    {'t', "ryfgh"},
    {'u', "yihjk"},
    {'v', "cfgb"},
    {'w', "qeasd"},
    {'x', "zsdc"},
    {'y', "tughj"},
    {'z', "asx"}
};

// Function prototypes
void init_simulator(TypingSimulator* sim);
void cleanup_simulator(TypingSimulator* sim);
double get_typing_delay(TypingSimulator* sim);
void simulate_keypress(char c);
void handle_typo(TypingSimulator* sim, char c);
void process_text(TypingSimulator* sim, const char* text);
void toggle_pause(TypingSimulator* sim);
void stop_typing(TypingSimulator* sim);
void clear_screen(void);

// Function implementations
void clear_screen() {
    system("cls");
}

void init_simulator(TypingSimulator* sim) {
    sim->state = STOPPED;
    sim->typo_count = 0;
    sim->typed_output = (char*)malloc(MAX_TEXT_LENGTH);
    sim->output_length = 0;
    sim->error_history = (int*)malloc(MAX_HISTORY_LENGTH * sizeof(int));
    sim->history_length = 0;
    sim->word_count = 0;
    sim->actual_typing_time = 0;

    // Initialize default settings
    sim->settings.base_wpm = 100;
    sim->settings.typo_probability = 0.2;
    sim->settings.speed_variation = 0.25;
    sim->settings.min_delay = 0.0;
    sim->settings.max_delay = 0.05;
    sim->settings.newline_multiplier = 1.5;
    sim->settings.chars_typed = 0;
    sim->settings.words_typed = 0;

    // Initialize list state
    sim->list_state.in_list = false;
    sim->list_state.current_level = 0;
    sim->list_state.list_type = NONE;
    sim->list_state.last_indent = 0;
    memset(sim->list_state.number_counter, 0, sizeof(sim->list_state.number_counter));
}

void cleanup_simulator(TypingSimulator* sim) {
    free(sim->typed_output);
    free(sim->error_history);
}

double get_typing_delay(TypingSimulator* sim) {
    // Calculate characters per second based on WPM
    double chars_per_second = (sim->settings.base_wpm * 5.0) / 60.0;

    // Base delay for each character (in seconds)
    double base_delay = 1.0 / chars_per_second;

    // Apply random variation to simulate natural typing
    double variation = ((double)rand() / RAND_MAX) * sim->settings.speed_variation * base_delay;
    double final_delay = base_delay + variation;

    // Ensure the delay stays within bounds
    if (final_delay < sim->settings.min_delay) final_delay = sim->settings.min_delay;
    if (final_delay > sim->settings.max_delay) final_delay = sim->settings.max_delay;

    return final_delay;
}

void simulate_keypress(char c) {
    SHORT vk = VkKeyScanA(c);
    WORD vk_code = vk & 0xFF;
    WORD shift_state = (vk >> 8) & 0xFF;

    INPUT inputs[4] = { 0 };  // Max 4 inputs: shift down, key down, key up, shift up
    int input_count = 0;

    // If shift is needed (for uppercase or special chars)
    if (shift_state & 1) {
        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = VK_SHIFT;
        input_count++;
    }

    // Key down
    inputs[input_count].type = INPUT_KEYBOARD;
    inputs[input_count].ki.wVk = vk_code;
    input_count++;

    // Key up
    inputs[input_count].type = INPUT_KEYBOARD;
    inputs[input_count].ki.wVk = vk_code;
    inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
    input_count++;

    // Release shift if it was pressed
    if (shift_state & 1) {
        inputs[input_count].type = INPUT_KEYBOARD;
        inputs[input_count].ki.wVk = VK_SHIFT;
        inputs[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
        input_count++;
    }

    SendInput(input_count, inputs, sizeof(INPUT));
    Sleep(10);  // Small delay between keystrokes
}

void handle_typo(TypingSimulator* sim, char c) {
    // Find adjacent keys for the current character
    char typo = c;  // Default to same character if no adjacents found
    for (int i = 0; i < KEYBOARD_LAYOUT_SIZE; i++) {
        if (tolower(c) == keyboard_layout[i].key) {
            int adj_len = strlen(keyboard_layout[i].adjacent);
            typo = keyboard_layout[i].adjacent[rand() % adj_len];
            break;
        }
    }

    simulate_keypress(typo);
    Sleep(100);  // Brief pause
    simulate_keypress('\b');  // Backspace
    Sleep(50);
    simulate_keypress(c);  // Correct character
    sim->typo_count++;
}

void display_typing_recap(TypingSimulator* sim) {
    clear_screen();
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, sim->settings.start_time);

    printf("\nstatistics\n");
    printf("please note the WPM calculation is not perfect and will vary slightly.\n");
    printf("===================\n");
    printf("total time: %.1f seconds\n", elapsed_time);
    printf("characters typed: %d\n", sim->settings.chars_typed);
    printf("words typed: %d\n", sim->settings.words_typed);
    printf("average WPM: %.1f\n", (sim->settings.words_typed / (elapsed_time / 60.0)));
    printf("typos made: %d\n", sim->typo_count);
    printf("\npress any key to continue...");
    _getch();
    clear_screen();
}

void process_text(TypingSimulator* sim, const char* text) {
    sim->state = RUNNING;
    sim->settings.start_time = time(NULL);
    sim->settings.chars_typed = 0;
    sim->settings.words_typed = 0;
    bool in_word = false;

    char line[MAX_LINE_LENGTH];
    const char* text_ptr = text;

    // Calculate time per word based on base_wpm
    double time_per_word = 60.0 / sim->settings.base_wpm; // In seconds
    double chars_per_word = 5.0; // Standard WPM assumption
    double time_per_char = time_per_word / chars_per_word;

    while (*text_ptr && sim->state != STOPPED) {
        int line_len = 0;
        while (*text_ptr && *text_ptr != '\n' && line_len < MAX_LINE_LENGTH - 1) {
            line[line_len++] = *text_ptr++;
        }
        line[line_len] = '\0';

        for (int i = 0; i < line_len; i++) {
            if (sim->state == STOPPED) break;

            while (sim->state == PAUSED) {
                Sleep(100);
            }

            // Track words
            if (isspace(line[i])) {
                if (in_word) {
                    sim->settings.words_typed++;
                    in_word = false;
                }
            }
            else {
                in_word = true;
            }

            // Simulate typing the character
            simulate_keypress(line[i]);
            sim->settings.chars_typed++;

            // Delay per character to match WPM
            Sleep((DWORD)(time_per_char * 1000));
        }

        // Count the last word in the line if it exists
        if (in_word) {
            sim->settings.words_typed++;
            in_word = false;
        }

        if (*text_ptr == '\n') {
            simulate_keypress('\n'); // Simulate newline only
            text_ptr++;
            Sleep((DWORD)(time_per_char * chars_per_word * sim->settings.newline_multiplier * 1000));
        }
    }

    display_typing_recap(sim);
    sim->state = STOPPED;
}

void toggle_pause(TypingSimulator* sim) {
    if (sim->state == PAUSED) {
        sim->state = RUNNING;
    }
    else {
        sim->state = PAUSED;
    }
}

void stop_typing(TypingSimulator* sim) {
    sim->state = STOPPED;
}

int main() {
    TypingSimulator sim;
    char choice;
    char text[MAX_TEXT_LENGTH];

    init_simulator(&sim);
    srand((unsigned int)time(NULL));

    while (1) {
        printf("\nhumanizer\n");
        printf("=====================================\n");
        printf("1. start humanizer\n");
        printf("2. exit\n");
        printf("\nenter your choice (1-2): ");

        scanf(" %c", &choice);
        getchar();  // Consume newline

        switch (choice) {
        case '1':
            printf("\nenter text (type 'END' on a new line to finish):\n");
            int pos = 0;
            while (pos < MAX_TEXT_LENGTH - 1) {
                char line[MAX_LINE_LENGTH];
                if (!fgets(line, MAX_LINE_LENGTH, stdin)) break;
                if (strcmp(line, "END\n") == 0) break;
                strcpy(text + pos, line);
                pos += strlen(line);
            }
            text[pos] = '\0';

            printf("\npreparing to type in 3 seconds...\n");
            printf("switch to your target window now!\n");
            for (int i = 3; i > 0; i--) {
                printf("%d...\n", i);
                Sleep(1000);
            }

            process_text(&sim, text);
            break;

        case '2':
            printf("\nexiting humanizer\n");
            cleanup_simulator(&sim);
            return 0;

        default:
            printf("\ninvalid choice. please enter 1 or 2.\n");
        }
    }

    return 0;
}
