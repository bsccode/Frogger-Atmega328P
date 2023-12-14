# Frogger Project for ENS2257/ENS6155

## Overview

This project is a Frogger game implementation for ENS2257/ENS6155 using Lab Kits with ATmega328PB. The game includes sound production, collision handling, and a scoring system.

## Features

- Custom keypad input handling for gameplay control.
- Sound production for game events.
- Dynamic vehicle spawning and movement logic.
- Collision detection and handling.
- Score calculation based on gameplay performance.
- Visual representation of the game state on an LCD.

## Key Components

- ATmega328PB Microcontroller
- LCD for game display
- Keypad for user input
- Integrated sound production

## Game Mechanics

- The player navigates the frog using the keypad.
- Vehicles move across the screen, and the player must avoid collisions.
- Reaching the goal increases the score, and collisions decrease lives.
- The game includes a timer and a scoring system to enhance the challenge.

## How to Run

1. Load the code into an ATmega328PB microcontroller.
2. Connect an LCD and keypad as per the defined PIN configurations.
3. Compile and upload the code to the microcontroller.
4. Interact with the game using the keypad.

## Development Environment

- AVR GCC Compiler
- Atmel START for initialisation and configuration

## Code Structure

The code is organized into several sections:

- **Keypad Initialization and Input Handling**: For reading user input from the keypad.
- **Menu Functions**: To display different game modes and options.
- **Sound Production Functions**: For generating game sounds.
- **Game Logic and State Management**: Includes vehicle spawning, collision handling, and score management.
- **Game Rendering**: Functions to display the game state on the LCD.
- **Main Function**: Where the game modes are selected and the main game loop is executed.

## Conclusion

This project demonstrates the application of embedded systems concepts in game development, specifically using the ATmega328PB microcontroller, and provides a fun and interactive way to understand these concepts in practice.
