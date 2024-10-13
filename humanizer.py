import random
import time
import logging
import sys
import json
import os
from collections import deque
import pyautogui
import keyboard

# Setup logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

# Global variables
settings = {
    "base_wpm": 90, # Base words per minute
    "typo_probability": 0.15, # How often a typo is going to occur, default is 15%
    "speed_variation": 0.2  # aka 20% variation in speed
}
typo_count = 0
typed_output = []
error_history = deque(maxlen=100)  # Store last 100 typing events
settings_file = "humanizer.json"

# Keyboard layout for realistic typos
keyboard_layout = {
    'q': 'was', 'w': 'qeasd', 'e': 'wrsdf', 'r': 'etdfg', 't': 'ryfgh', 'y': 'tughj', 'u': 'yihjk',
    'i': 'uojkl', 'o': 'ipkl;', 'p': 'o[l;\'', 'a': 'qwsz', 's': 'awedxz', 'd': 'serfcx', 'f': 'drtgvc',
    'g': 'ftyhbv', 'h': 'gyujnb', 'j': 'huikmn', 'k': 'jiolm,', 'l': 'kop;,.', 'z': 'asx', 'x': 'zsdc',
    'c': 'xdfv', 'v': 'cfgb', 'b': 'vghn', 'n': 'bhjm', 'm': 'njk,',
}

def load_settings():
    global settings
    if os.path.exists(settings_file):
        with open(settings_file, 'r') as f:
            settings.update(json.load(f))
    logging.info(f"Settings loaded: {settings}")

def save_settings():
    with open(settings_file, 'w') as f:
        json.dump(settings, f)
    logging.info("Settings saved")

def settings_menu():
    global settings
    while True:
        print("\nSettings Menu:")
        print(f"1. Base WPM: {settings['base_wpm']}")
        print(f"2. Typo Probability: {settings['typo_probability']}")
        print(f"3. Speed Variation: {settings['speed_variation']}")
        print("4. Save and Exit")
        
        choice = input("Enter your choice (1-4): ")
        if choice == '1':
            settings['base_wpm'] = int(input("Enter new Base WPM: "))
        elif choice == '2':
            settings['typo_probability'] = float(input("Enter new Typo Probability (0-1): "))
        elif choice == '3':
            settings['speed_variation'] = float(input("Enter new Speed Variation (0-1): "))
        elif choice == '4':
            save_settings()
            break
        else:
            print("Invalid choice. Please try again.")

def generate_typo(char):
    if char.lower() in keyboard_layout:
        nearby_keys = keyboard_layout[char.lower()]
        typo = random.choice(nearby_keys)
        return typo if char.islower() else typo.upper()
    return char

def handle_typo(char):
    global typo_count, typed_output
    typo_char = generate_typo(char)
    pyautogui.typewrite(typo_char)
    time.sleep(random.uniform(0.1, 0.3))  # Pause before correction
    pyautogui.press('backspace')
    pyautogui.typewrite(char)
    typo_count += 1
    typed_output.extend([typo_char, char])
    error_history.append(1)  # 1 indicates a typo

def get_typing_delay():
    base_delay = 60 / (settings['base_wpm'] * 5)  # Assuming average word length of 5 characters
    variation = random.uniform(-settings['speed_variation'], settings['speed_variation'])
    return base_delay * (1 + variation)

def simulate_typing(text):
    global typo_count, typed_output
    logging.info(f"Started typing simulation for text: {text}")
    typed_output = []
    typo_count = 0

    start_time = time.time()
    for char in text:
        delay = get_typing_delay()
        time.sleep(delay)

        if random.random() < settings['typo_probability']:
            handle_typo(char)
        else:
            pyautogui.typewrite(char)
            typed_output.append(char)
            error_history.append(0)  # 0 indicates correct typing

    end_time = time.time()
    logging.info("Finished typing simulation")
    return start_time, end_time

def print_summary(start_time, end_time, text):
    elapsed_time = end_time - start_time
    num_chars = len(text)
    num_words = len(text.split())
    wpm = (num_words / elapsed_time) * 60
    accuracy = (num_chars - typo_count) / num_chars * 100

    summary = f"""
    Typing Session Summary
    ----------------------
    Time taken: {elapsed_time:.2f} seconds
    Characters typed: {num_chars}
    Words typed: {num_words}
    Words per minute (WPM): {wpm:.2f}
    Number of typos: {typo_count}
    Accuracy: {accuracy:.2f}%
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

def main():
    load_settings()
    while True:
        print("\nMain Menu:")
        print("1. Start Typing Simulation")
        print("2. Settings")
        print("3. Exit")
        choice = input("Enter your choice (1-3): ")

        if choice == '1':
            user_text = get_user_text()
            print("Preparing to type... Press Ctrl+C to stop.")
            time.sleep(3)  # Give user time to switch to desired typing area
            try:
                start_time, end_time = simulate_typing(user_text)
                print_summary(start_time, end_time, user_text)
            except KeyboardInterrupt:
                print("\nTyping simulation interrupted.")
        elif choice == '2':
            settings_menu()
        elif choice == '3':
            print("Exiting program.")
            break
        else:
            print("Invalid choice. Please try again.")

if __name__ == "__main__":
    main()
