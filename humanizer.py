import random
import time
import logging
import sys
import json
import os
import pyautogui
import datetime
import pyperclip
import numpy as np
from tqdm import tqdm
from sklearn.neural_network import MLPRegressor

# Setup logging to the command prompt window
logging.basicConfig(level=logging.DEBUG, stream=sys.stdout, format='%(asctime)s - %(levelname)s - %(message)s')

# Global counters and variables
typo_count = 0
start_time = None
end_time = None
typing_style = "average"
error_history = []
typed_output = []  # To keep track of typed characters
profiles_path = "typing_profiles.json"

# Define typing profiles
typing_profiles = {
    "slow": {"delay_range": (0.15, 0.4), "typo_probability": 0.15},
    "average": {"delay_range": (0.05, 0.2), "typo_probability": 0.05},
    "fast": {"delay_range": (0.01, 0.1), "typo_probability": 0.02},
}

# Machine learning model for typing speed adaptation
model = MLPRegressor(hidden_layer_sizes=(50, 50), max_iter=500)

# Initial data for model training
initial_data_X = np.array([[0, 1, 1], [1, 1, 2], [2, 1, 3]])  # Extended features
initial_data_y = np.array([0.2, 0.15, 0.1])
model.fit(initial_data_X, initial_data_y)

def simulate_typing(text):
    global typo_count, start_time, end_time, error_history, typed_output
    logging.info(f"Started typing simulation for text: {text}")
    typed_output = []  # Reset typed_output for each run
    typo_count = 0

    start_time = time.time()
    words = text.split()
    for word_index, word in enumerate(words):
        for char_index, char in enumerate(word):
            try:
                delay = get_typing_delay(word_index, len(word), char_index)
                time.sleep(delay)
                if char == '\n':
                    pyautogui.press('enter')
                    typed_output.append('\n')
                else:
                    pyautogui.typewrite(char)
                    typed_output.append(char)
                logging.debug(f"Typed character: '{char}' with delay: {delay:.2f}s")

                # Simulate typo and correction
                if random.random() < typing_profiles[typing_style]["typo_probability"]:
                    typo_char = generate_typo(char)
                    logging.debug(f"Generated typo character: '{typo_char}' instead of '{char}'")
                    handle_typo(typo_char, char)

                # Simulate random deletions and retyping
                if random.random() < 0.02 and len(typed_output) > 10:
                    num_chars_to_delete = random.randint(1, 5)
                    for _ in range(num_chars_to_delete):
                        if typed_output:
                            pyautogui.press('backspace')
                            typed_output.pop()
                    logging.debug(f"Revised last {num_chars_to_delete} characters")
                    retype_chars(num_chars_to_delete)

                # Simulate thinking pauses
                if random.random() < 0.05:
                    pause_duration = random.uniform(0.5, 2.0)
                    logging.debug(f"Pausing for {pause_duration:.2f}s to simulate thinking")
                    time.sleep(pause_duration)

            except Exception as e:
                logging.error(f"Error while typing character '{char}': {e}")

        pyautogui.typewrite(' ')  # Add space after each word
        typed_output.append(' ')

    end_time = time.time()
    logging.info("Finished typing simulation")

def handle_typo(typo_char, original_char):
    """
    Handles typing a typo and then correcting it.
    """
    global typo_count, typed_output
    # Type the typo
    pyautogui.press('backspace')
    pyautogui.typewrite(typo_char)
    typo_count += 1
    typed_output[-1] = typo_char
    error_history.append((len(typed_output) - 1, original_char, typo_char))

    # Correct the typo
    pyautogui.press('backspace')
    pyautogui.typewrite(original_char)
    typed_output[-1] = original_char
    logging.debug(f"Corrected typo to: '{original_char}'")

def retype_chars(num_chars):
    """
    Retypes the given number of characters from the end of the current output.
    """
    global typed_output
    retyped_text = typed_output[-num_chars:]
    for char in retyped_text:
        pyautogui.typewrite(char)
        logging.debug(f"Retyped character: '{char}'")

def generate_typo(char):
    """
    Generates a typo character similar to the given character.
    """
    typo_variants = {
        'a': 's', 's': 'a', 'd': 'f', 'f': 'd', 'g': 'h', 'h': 'g', 'j': 'k', 'k': 'j', 'l': 'k',
        'q': 'w', 'w': 'q', 'e': 'r', 'r': 'e', 't': 'y', 'y': 't', 'u': 'i', 'i': 'u', 'o': 'p', 'p': 'o',
        'z': 'x', 'x': 'z', 'c': 'v', 'v': 'c', 'b': 'n', 'n': 'b', 'm': 'n', 'n': 'm',
        'A': 'S', 'S': 'A', 'D': 'F', 'F': 'D', 'G': 'H', 'H': 'G', 'J': 'K', 'K': 'J', 'L': 'K',
        'Q': 'W', 'W': 'Q', 'E': 'R', 'R': 'E', 'T': 'Y', 'Y': 'T', 'U': 'I', 'I': 'U', 'O': 'P', 'P': 'O',
        'Z': 'X', 'X': 'Z', 'C': 'V', 'V': 'C', 'B': 'N', 'N': 'B', 'M': 'N', 'N': 'M',
        '0': '9', '1': '2', '2': '1', '3': '4', '4': '3', '5': '6', '6': '5', '7': '8', '8': '7', '9': '0',
        '!': '@', '@': '!', '#': '$', '$': '#', '%': '^', '^': '%', '&': '*', '*': '&', '(': ')', ')': '(',
        '_': '-', '-': '_', '+': '=', '=': '+', '[': ']', ']': '[', '{': '}', '}': '{', ';': "'", "'": ";",
        ':': '"', '"': ':', ',': '.', '.': ',', '<': '>', '>': '<', '?': '/', '/': '?',
        'á': 'a', 'é': 'e', 'í': 'i', 'ó': 'o', 'ú': 'u', 'ü': 'u', 'ñ': 'n', 'ç': 'c',
        'ß': 's', 'ø': 'o', 'å': 'a', 'œ': 'o', 'æ': 'a', 'ð': 'd'
    }
    if char in typo_variants:
        return typo_variants[char] if random.random() < 0.5 else random.choice(list(typo_variants.values()))
    else:
        return char

def get_typing_delay(word_index, word_length, char_index):
    delays = typing_profiles[typing_style]["delay_range"]
    historical_data = np.array(error_history)
    if len(historical_data) > 0:
        input_features = np.array([[word_index, word_length, char_index]])
        try:
            predicted_delay = model.predict(input_features)[0]
            logging.debug(f"Predicted delay: {predicted_delay:.2f}s for word index: {word_index}, word length: {word_length}, char index: {char_index}")
            return max(delays[0], min(predicted_delay, delays[1]))
        except Exception as e:
            logging.warning(f"Prediction error: {e}")
            return random.uniform(*delays)
    else:
        return random.uniform(*delays)

def display_progress_bar(seconds):
    print("Starting shortly...")
    for _ in tqdm(range(seconds), desc="Preparing to type"):
        time.sleep(1)

def print_summary(start_time, end_time, text, typo_count):
    elapsed_time = end_time - start_time
    num_letters = len(text)
    num_words = len(text.split())
    wpm = (num_words / elapsed_time) * 60

    summary = f"""
    Typing Session Summary
    ----------------------
    Time taken: {elapsed_time:.2f} seconds
    Number of letters typed: {num_letters}
    Number of words typed: {num_words}
    Words per minute (WPM): {wpm:.2f}
    Number of typos: {typo_count}
    """
    print(summary)
    logging.info(summary)

def get_user_text():
    print("Enter the text to simulate typing (type 'END' on a new line to finish):")
    lines = []
    while True:
        line = input()
        if line.strip().upper() == 'END':
            break
        lines.append(line)
    return '\n'.join(lines)

def set_typing_style(style):
    global typing_style
    if style in typing_profiles:
        typing_style = style
    else:
        print(f"Typing style '{style}' not recognized. Using default 'average'.")

def save_typing_profile():
    data = {
        'typing_profiles': typing_profiles,
        'error_history': error_history
    }
    with open(profiles_path, 'w') as f:
        json.dump(data, f)
    logging.info("Typing profiles and error history saved.")

def load_typing_profile():
    global typing_profiles, error_history
    if os.path.exists(profiles_path):
        with open(profiles_path, 'r') as f:
            data = json.load(f)
            typing_profiles = data.get('typing_profiles', typing_profiles)
            error_history = data.get('error_history', error_history)
        logging.info("Typing profiles and error history loaded.")
    else:
        logging.info("No existing typing profiles found.")

def display_cli_statistics():
    """
    Displays typing statistics on the CLI.
    """
    if not error_history:
        print("No typing statistics to display.")
        return

    total_chars = sum(len(err[1]) for err in error_history)
    total_errors = len(error_history)
    error_rate = total_errors / total_chars * 100 if total_chars > 0 else 0
    avg_typo_delay = np.mean([err[2] for err in error_history]) if error_history else 0

    print(f"\nTyping Statistics:")
    print(f"-------------------")
    print(f"Total characters: {total_chars}")
    print(f"Total errors: {total_errors}")
    print(f"Error rate: {error_rate:.2f}%")
    print(f"Average typo delay: {avg_typo_delay:.2f}s\n")
    logging.info(f"Typing Statistics - Total chars: {total_chars}, Total errors: {total_errors}, Error rate: {error_rate:.2f}%, Avg typo delay: {avg_typo_delay:.2f}s")

def main():
    try:
        load_typing_profile()

        print("Choose typing style (slow, average, fast):")
        style = input().strip()
        set_typing_style(style)

        user_text = get_user_text()

        # Calculate and display the estimated end time before countdown
        average_delay = sum(typing_profiles[typing_style]["delay_range"]) / 2
        estimated_end_time = datetime.datetime.now() + datetime.timedelta(seconds=len(user_text) * average_delay)
        print(f"Estimated end time: {estimated_end_time.strftime('%Y-%m-%d %H:%M:%S')}")
        logging.info(f"Estimated end time: {estimated_end_time.strftime('%Y-%m-%d %H:%M:%S')}")

        display_progress_bar(5)
        simulate_typing(user_text)
        print_summary(start_time, end_time, user_text, typo_count)

        if error_history:
            X = np.array([[i, len(err[1]), len(typed_output) - i] for i, err in enumerate(error_history)])
            y = np.array([random.uniform(*typing_profiles[typing_style]["delay_range"]) for _ in error_history])
            model.fit(X, y)

        # Display CLI statistics instead of a graph
        display_cli_statistics()

        save_typing_profile()

    except Exception as e:
        logging.error(f"Unhandled exception: {e}")

if __name__ == "__main__":
    main()
