// Current Issues:
// - Case handling does not work
// - Settings have to be manually adjusted in the code, settings panel broken 


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
    int number_counter[10];  // Support up to 10 levels of nesting
    int last_indent;
} ListState;

typedef struct {
    double base_wpm;
    double typo_probability;
    double speed_variation;
    char pause_key;
    char stop_key;
    int indent_spaces;
    double newline_multiplier;
    double special_char_multiplier;
    double min_delay;
    double max_delay;
    double pause_variation;
    double thoughtful_pause_multiplier;
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

// Keyboard layout for typos - properly defined as array of structs
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
void load_settings(TypingSimulator* sim, const char* filename);
void save_settings(TypingSimulator* sim, const char* filename);
double get_typing_delay(TypingSimulator* sim);
void simulate_keypress(char c);
void handle_typo(TypingSimulator* sim, char c);
void process_text(TypingSimulator* sim, const char* text);
void toggle_pause(TypingSimulator* sim);
void stop_typing(TypingSimulator* sim);
void display_settings_menu(TypingSimulator* sim);

// Function implementations
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
    sim->settings.pause_key = 'p';
    sim->settings.stop_key = 's';
    sim->settings.indent_spaces = DEFAULT_INDENT_SPACES;
    sim->settings.newline_multiplier = 1.5;
    sim->settings.special_char_multiplier = 1.2;
    sim->settings.min_delay = 0.0;
    sim->settings.max_delay = 0.05;
    sim->settings.pause_variation = 0.10;
    sim->settings.thoughtful_pause_multiplier = 2.0;

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

void load_settings(TypingSimulator* sim, const char* filename) {
    // Implement loading settings from a file
}

void save_settings(TypingSimulator* sim, const char* filename) {
    // Implement saving settings to a file
}

double get_typing_delay(TypingSimulator* sim) {
    double chars_per_second = (sim->settings.base_wpm * 5.0) / 60.0;
    double base_delay = 1.0 / (chars_per_second + 0.01);

    double variation_scale = base_delay / 2.0;
    if (variation_scale > 0.5) variation_scale = 0.5;

    double variation_amount = base_delay * sim->settings.speed_variation * variation_scale;
    double variation = ((double)rand() / RAND_MAX) * variation_amount * 2 - variation_amount;

    double final_delay = base_delay + variation;
    if (final_delay < sim->settings.min_delay) final_delay = sim->settings.min_delay;
    if (final_delay > sim->settings.max_delay) final_delay = sim->settings.max_delay;

    return final_delay;
}

void simulate_keypress(char c) {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = MapVirtualKey(VkKeyScanA(c) & 0xFF, 0);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));

    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(10); // Small delay between keystrokes
}

void handle_typo(TypingSimulator* sim, char c) {
    // Simple typo simulation - just press an adjacent key
    char typo = 'x';  // Default typo character
    simulate_keypress(typo);
    Sleep(100);  // Brief pause
    simulate_keypress('\b');  // Backspace
    Sleep(50);
    simulate_keypress(c);  // Correct character
    sim->typo_count++;
}

void process_text(TypingSimulator* sim, const char* text) {
    sim->state = RUNNING;
    time_t start_time = time(NULL);
    time_t last_pause_time = start_time;

    char line[MAX_LINE_LENGTH];
    const char* text_ptr = text;

    while (*text_ptr && sim->state != STOPPED) {
        // Extract line
        int line_len = 0;
        while (*text_ptr && *text_ptr != '\n' && line_len < MAX_LINE_LENGTH - 1) {
            line[line_len++] = *text_ptr++;
        }
        line[line_len] = '\0';

        // Process line
        for (int i = 0; i < line_len; i++) {
            if (sim->state == STOPPED) break;

            while (sim->state == PAUSED) {
                Sleep(100);  // Sleep 100ms while paused
            }

            double delay = get_typing_delay(sim);

            if ((double)rand() / RAND_MAX < sim->settings.typo_probability) {
                handle_typo(sim, line[i]);
            }
            else {
                simulate_keypress(line[i]);
            }

            Sleep((DWORD)(delay * 1000));  // Convert seconds to milliseconds
        }

        if (*text_ptr == '\n') {
            simulate_keypress('\r');
            simulate_keypress('\n');
            text_ptr++;
            Sleep((DWORD)(sim->settings.newline_multiplier * get_typing_delay(sim) * 1000));
        }
    }
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

void display_settings_menu(TypingSimulator* sim) {
    // Implement settings menu functionality here
    printf("Settings menu:\n");
    printf("1. Adjust WPM\n");
    printf("2. Adjust Typo Probability\n");
    printf("3. Adjust Speed Variation\n");
    // Add other settings options here
}

int main() {
    TypingSimulator sim;
    char choice;
    char text[MAX_TEXT_LENGTH];

    init_simulator(&sim);
    srand((unsigned int)time(NULL));

    while (1) {
        printf("\nhumanizer - Advanced Typing Simulator\n");
        printf("=====================================\n");
        printf("1. Start Humanizer\n");
        printf("2. Settings\n");
        printf("3. Exit\n");
        printf("\nEnter your choice (1-3): ");

        scanf(" %c", &choice);
        getchar();  // Consume newline

        switch (choice) {
        case '1':
            printf("\nEnter text to simulate typing (type 'END' on a new line to finish):\n");
            int pos = 0;
            while (pos < MAX_TEXT_LENGTH - 1) {
                char line[MAX_LINE_LENGTH];
                if (!fgets(line, MAX_LINE_LENGTH, stdin)) break;
                if (strcmp(line, "END\n") == 0) break;
                strcpy(text + pos, line);
                pos += strlen(line);
            }
            text[pos] = '\0';

            printf("\nPreparing to type in 3 seconds...\n");
            printf("Switch to your target window now!\n");
            for (int i = 3; i > 0; i--) {
                printf("%d...\n", i);
                Sleep(1000);
            }

            process_text(&sim, text);
            break;

        case '2':
            display_settings_menu(&sim);
            break;

        case '3':
            printf("\nExiting Humanizer. Goodbye!\n");
            cleanup_simulator(&sim);
            return 0;

        default:
            printf("\nInvalid choice. Please enter 1, 2, or 3.\n");
        }
    }

    return 0;
}
