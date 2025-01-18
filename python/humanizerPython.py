# Depreciated, do not use 

import random
import time
import logging
import sys
import json
import os
from collections import deque
import pyautogui
import keyboard
from enum import Enum
from threading import Event, Lock
import re

# Setup logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

class TypingState(Enum):
    RUNNING = 1
    PAUSED = 2
    STOPPED = 3

class ListState:
    def __init__(self):
        self.in_list = False
        self.current_level = 0
        self.list_type = None  # 'bullet', 'numbered', or None
        self.number_counter = {}  # Dictionary to track numbering for each level
        self.last_indent = 0

# Global variables
settings = {
    "base_wpm": 60, 
    "typo_probability": 0.35,
    "speed_variation": 0.45,
    "pause_key": "f6",
    "resume_key": "f6",
    "stop_key": "f8",
    "indent_spaces": 3  # Number of spaces for each indentation level
}

class TypingSimulator:
    def __init__(self):
        self.state = TypingState.STOPPED
        self.pause_event = Event()
        self.pause_lock = Lock()
        self.typo_count = 0
        self.typed_output = []
        self.error_history = deque(maxlen=100)
        self.list_state = ListState()
        self.settings_file = "humanizer.json"
        self.word_count = 0
        self.actual_typing_time = 0
        self.load_settings()
        self.setup_hotkeys()
        self.keyboard_layout = {
            'q': 'was', 'w': 'qeasd', 'e': 'wrsdf', 'r': 'etdfg', 't': 'ryfgh',
            'y': 'tughj', 'u': 'yihjk', 'i': 'uojkl', 'o': 'ipkl;', 'p': 'o[l;\'',
            'a': 'qwsz', 's': 'awedxz', 'd': 'serfcx', 'f': 'drtgvc', 'g': 'ftyhbv',
            'h': 'gyujnb', 'j': 'huikmn', 'k': 'jiolm,', 'l': 'kop;,.', 'z': 'asx',
            'x': 'zsdc', 'c': 'xdfv', 'v': 'cfgb', 'b': 'vghn', 'n': 'bhjm', 'm': 'njk,'
        }

    def load_settings(self):
        global settings
        if os.path.exists(self.settings_file):
            try:
                with open(self.settings_file, 'r') as f:
                    settings.update(json.load(f))
                logging.info(f"Settings loaded: {settings}")
            except Exception as e:
                logging.error(f"Error loading settings: {e}")

    def save_settings(self):
        try:
            with open(self.settings_file, 'w') as f:
                json.dump(settings, f)
            logging.info("Settings saved")
        except Exception as e:
            logging.error(f"Error saving settings: {e}")

    def setup_hotkeys(self):
        keyboard.on_press_key(settings["pause_key"], lambda _: self.toggle_pause())
        keyboard.on_press_key(settings["stop_key"], lambda _: self.stop())

    def toggle_pause(self):
        with self.pause_lock:
            if self.state == TypingState.RUNNING:
                self.state = TypingState.PAUSED
                self.pause_event.clear()
                logging.info("Typing paused")
            elif self.state == TypingState.PAUSED:
                self.state = TypingState.RUNNING
                self.pause_event.set()
                logging.info("Typing resumed")

    def stop(self):
        self.state = TypingState.STOPPED
        self.pause_event.set()

    def count_actual_words(self, text):
        # Remove list markers and other formatting
        cleaned_text = re.sub(r'^\s*[-*+]\s+', '', text)  # Remove bullet points
        cleaned_text = re.sub(r'^\s*\d+\.\s+', '', cleaned_text)  # Remove numbered lists
        # Split by whitespace and filter out empty strings
        words = [word for word in cleaned_text.split() if word.strip()]
        return len(words)

    def calculate_indent_level(self, spaces):
        return spaces // settings['indent_spaces']

    def type_list_marker(self, marker, indent_level):
        # Type the proper indentation
        for _ in range(indent_level * settings['indent_spaces']):
            pyautogui.press('space')
            time.sleep(self.get_typing_delay() * 0.5)
        
        # Type the marker
        if isinstance(marker, int):  # Numbered list
            pyautogui.typewrite(f"{marker}. ")
        else:  # Bullet list
            pyautogui.typewrite(f"{marker} ")
        
        time.sleep(self.get_typing_delay())

    def handle_list_line(self, line):
        # Extract leading spaces
        leading_spaces = len(line) - len(line.lstrip())
        indent_level = self.calculate_indent_level(leading_spaces)
        stripped_line = line.lstrip()

        # Check for list markers
        bullet_match = re.match(r'^([-*+])\s+(.*)', stripped_line)
        number_match = re.match(r'^(\d+)\.\s+(.*)', stripped_line)

        if bullet_match:
            marker, content = bullet_match.groups()
            self.type_list_marker(marker, indent_level)
            return content
        elif number_match:
            number, content = number_match.groups()
            self.type_list_marker(int(number), indent_level)
            return content
        
        # No list marker, preserve original indentation
        for _ in range(leading_spaces):
            pyautogui.press('space')
            time.sleep(self.get_typing_delay() * 0.5)
        return stripped_line

    def get_typing_delay(self):
        # Calculate delay based on WPM, excluding formatting characters
        base_delay = 60 / (settings['base_wpm'] * 5)  # 5 chars per word
        variation = random.uniform(-settings['speed_variation'], settings['speed_variation'])
        return base_delay * (1 + variation)

    def handle_typo(self, char):
        typo_char = self.generate_typo(char)
        pyautogui.typewrite(typo_char)
        time.sleep(random.uniform(0.1, 0.3))
        pyautogui.press('backspace')
        pyautogui.typewrite(char)
        self.typo_count += 1
        self.typed_output.extend([typo_char, char])
        self.error_history.append(1)

    def generate_typo(self, char):
        if char.lower() in self.keyboard_layout:
            nearby_keys = self.keyboard_layout[char.lower()]
            typo = random.choice(nearby_keys)
            return typo if char.islower() else typo.upper()
        return char

    def simulate_typing(self, text):
        self.state = TypingState.RUNNING
        self.pause_event.set()
        self.typo_count = 0
        self.typed_output = []
        self.word_count = 0
        self.actual_typing_time = 0
        
        start_time = time.time()
        last_pause_time = start_time
        lines = text.split('\n')
        
        try:
            for line in lines:
                # Handle pause markers
                if '{PAUSE' in line:
                    pause_match = re.match(r'.*\{PAUSE(?::(\d+))?\}.*', line)
                    if pause_match:
                        # Record typing time before pause
                        self.actual_typing_time += time.time() - last_pause_time
                        
                        duration = pause_match.group(1)
                        if duration:
                            time.sleep(int(duration))
                        else:
                            self.toggle_pause()
                            self.pause_event.wait()
                        
                        # Update last pause time
                        last_pause_time = time.time()
                        line = line.replace(pause_match.group(0), '')

                # Count actual words in the line
                self.word_count += self.count_actual_words(line)
                
                # Process the line for list formatting
                processed_line = self.handle_list_line(line)
                
                # Type the processed line
                for char in processed_line:
                    if self.state == TypingState.STOPPED:
                        self.actual_typing_time += time.time() - last_pause_time
                        return start_time, time.time()
                    
                    self.pause_event.wait()
                    
                    delay = self.get_typing_delay()
                    time.sleep(delay)
                    
                    if random.random() < settings['typo_probability']:
                        self.handle_typo(char)
                    else:
                        pyautogui.typewrite(char)
                        self.typed_output.append(char)
                        self.error_history.append(0)
                
                # Add newline after each line unless it's the last line
                if line != lines[-1]:
                    pyautogui.press('enter')
                    time.sleep(self.get_typing_delay() * 2)

        except KeyboardInterrupt:
            self.stop()
        
        self.actual_typing_time += time.time() - last_pause_time
        end_time = time.time()
        return start_time, end_time

def main():
    simulator = TypingSimulator()
    
    while True:
        print("\nhumanizer")
        print("------------------------")
        print("1. Start Humanizer")
        print("2. Settings")
        print("3. Exit")
        print(f"\nHotkeys: {settings['pause_key'].upper()} to pause/resume, {settings['stop_key'].upper()} to stop")
        
        choice = input("\nEnter your choice (1-3): ")
        
        if choice == '1':
            print("\nEnter the text to simulate typing (type 'END' on a new line to finish):")
            print("Special commands:")
            print("- {PAUSE} to pause until resume key is pressed")
            print("- {PAUSE:N} to pause for N seconds")
            print("- Use '*', '-', or '+' for bullet points (indent with 3 spaces)")
            print("- Use '1.', '2.' etc. for numbered lists")
            
            lines = []
            while True:
                line = input()
                if line.strip().upper() == 'END':
                    break
                lines.append(line)
            
            text = '\n'.join(lines)
            print("\nPreparing to type in 3 seconds... Switch to your desired window.")
            time.sleep(3)
            
            try:
                start_time, end_time = simulator.simulate_typing(text)
                
                # Calculate WPM based only on actual words typed
                actual_wpm = (simulator.word_count / simulator.actual_typing_time) * 60
                
                print(f"\nTyping Session Summary")
                print("-" * 20)
                print(f"Time taken: {simulator.actual_typing_time:.2f} seconds")
                print(f"Actual words typed: {simulator.word_count}")
                print(f"Words per minute (WPM): {actual_wpm:.2f}")
                print(f"Number of typos: {simulator.typo_count}")
                
                # Calculate accuracy based on actual content
                total_chars = len(''.join(simulator.typed_output))
                accuracy = ((total_chars - simulator.typo_count) / total_chars * 100) if total_chars > 0 else 100
                print(f"Accuracy: {accuracy:.2f}%")
            
            except KeyboardInterrupt:
                print("\nTyping simulation interrupted.")
        
        elif choice == '2':
            print("\nCurrent Settings:")
            for key, value in settings.items():
                print(f"{key}: {value}")
            
            setting_to_change = input("\nEnter setting to change (or 'done' to finish): ")
            while setting_to_change.lower() != 'done':
                if setting_to_change in settings:
                    new_value = input(f"Enter new value for {setting_to_change}: ")
                    try:
                        if isinstance(settings[setting_to_change], bool):
                            settings[setting_to_change] = new_value.lower() == 'true'
                        elif isinstance(settings[setting_to_change], int):
                            settings[setting_to_change] = int(new_value)
                        elif isinstance(settings[setting_to_change], float):
                            settings[setting_to_change] = float(new_value)
                        else:
                            settings[setting_to_change] = new_value
                    except ValueError:
                        print("Invalid value type!")
                else:
                    print("Invalid setting!")
                
                setting_to_change = input("\nEnter setting to change (or 'done' to finish): ")
            
            simulator.save_settings()
            print("Settings saved!")
        
        elif choice == '3':
            print("Exiting program.")
            break
        
        else:
            print("Invalid choice. Please try again.")

if __name__ == "__main__":
    main()
