// Frogger Functionality
// for ENS2257/ENS6155 with Lab Kits with ATmega328PB

#define F_CPU 16000000UL

#include <atmel_start.h>
#include <stdint-gcc.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/iom328pb.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <lcd.h>
#include <math.h>
#include <string.h>
#include <avr/io.h>

// PIN DEFINITIONS:
// PD2 -- KEYPAD COL1 (left column)
// PD0 -- KEYPAD COL2 (center column)
// PD4 -- KEYPAD COL3 (right column)
// PD1 -- KEYPAD ROW1 (top row)
// PD6 -- KEYPAD ROW2 (second row)
// PD5 -- KEYPAD ROW3 (third row)
// PD3 -- KEYPAD ROW4 (fourth row)

#define GRID_WIDTH 20		//Grid Parameters for LCD Screen
#define GRID_HEIGHT 4

#define MAX_VEHICLES 5      // Maximum Vehicles to be generated at one time  

// Note frequencies (in Hz) for C4 to B4

#define freq_C4 261       
#define freq_D4 294
#define freq_E4 330
#define freq_F4 349
#define freq_G4 392
#define freq_A4 440
#define freq_B4 494
#define freq_C5 523

// Global variables
char grid[GRID_HEIGHT][GRID_WIDTH];
int frogX, frogY;
int frogMarkY, frogMarkX;
int frogMarkY2, frogMarkX2;
int lives = 3;
int previousFrogY = GRID_HEIGHT - 1;    
int score = 0;
int frogsAtHome = 0;
float timer = 20;
int seed;

// Struct for Vehicles (Trucks and Cars)         
typedef struct {
    int x;
    int y;
    int length;
    char representation[6];
    char reversed_representation[6];
    bool active;
    int type;
} Vehicle;

Vehicle vehicles[MAX_VEHICLES];

//********************************************************************************************************
//*			KeyPad Functions and init - Used from Lab 3													 *
//********************************************************************************************************

//**************************************************
void keypad_init() {
    // Subroutine to set ports to required input and output states and enable pull up resistors for inputs
    
    // Set the columns as output
    DDRD |= (1<<PD0) | (1<<PD2) | (1<<PD4);
    
    // Set row pins to input mode  
    DDRD &= ~(1<<PD1) & ~(1<<PD3) & ~(1<<PD5) & ~(1<<PD6);
    
    // Turn on the internal resistors for the input pins
    PORTD |= (1<<PD1) | (1<<PD3) | (1<<PD5) | (1<<PD6);
    
    // Initialise the column output pins low, so input low if contact made
    PORTD &= ~(1<<PD0) & ~(1<<PD2) & ~(1<<PD4);
}  // END keypad_init

//**************************************************

uint8_t keypressed() {
    uint8_t rowval;
    uint8_t kp;
    
    PORTD &= ~(1<<PD0) & ~(1<<PD2) & ~(1<<PD4);
    _delay_us(10);
    
    rowval = PIND & 0x6A;
    
    kp = (rowval != 0x6A);
    
    return kp;
}

//**************************************************

char keypad_read(char lastchar) {
    uint8_t rowval;
    char keych;
    
    keych = '$';
    
    PORTD &= ~(1<<PD2);
    PORTD |= (1<<PD0) | (1<<PD4);
    _delay_us(10);
    
    rowval = PIND & 0x6A;
    
    switch(rowval) {
        case 0x68: keych = '1'; break;
        case 0x2A: keych = '4'; break;
        case 0x4A: keych = '7'; break;
        case 0x62: keych = '*'; break;
        default: keych = '$'; break;
    }
    
    if (keych=='$') {
        PORTD &= ~(1<<PD0);
        PORTD |= (1<<PD2) | (1<<PD4);
        _delay_us(10);
        
        rowval = PIND & 0x6A;
        
        switch(rowval) {
            case 0x68: keych = '2'; break;
            case 0x2A: keych = '5'; break;
            case 0x4A: keych = '8'; break;
            case 0x62: keych = '0'; break;
            default: keych = '$'; break;
        }
    }
    
    if (keych=='$') {
        PORTD &= ~(1<<PD4);
        PORTD |= (1<<PD0) | (1<<PD2);
        _delay_us(10);
        
        rowval = PIND & 0x6A;
        
        switch(rowval) {
            case 0x68: keych = '3'; break;
            case 0x2A: keych = '6'; break;
            case 0x4A: keych = '9'; break;
            case 0x62: keych = '#'; break;
            default: keych = '$'; break;
        }
    }
    
    if (keych != '$') {
        lastchar = keych;
    }
    
    return lastchar;
}  // END keypad_read

//**************************************************

uint8_t get_keypad_input() {
    if (keypressed()) {
        char keychar = keypad_read('$');
        return keychar;
    }
    return '$';
}

//**************************************************

void init_interrupts() {
    PCMSK0 = (1<<PCINT1);
    PCICR &= ~(1<<PCIE1) & ~(1<<PCIE2);
    PCICR |= (1<<PCIE0);
    sei();
}  // END init_interrupts

//**************************************************

//********************************************************************************************************
//*					Menu Functions																		 *
//********************************************************************************************************

//Displays Welcome Menu 

void display_menu(void) {
    lcd_clear_and_home();
    lcd_write_string(PSTR("1: Play Tones"));
    lcd_goto_position(1, 0);  
    lcd_write_string(PSTR("2: Play Frogger"));
    lcd_goto_position(2, 0);  
    lcd_write_string(PSTR("3: Scored Frogger"));
    lcd_goto_position(3, 0);  
    lcd_write_string(PSTR("4: C Major"));
}


//Displays 3x4 grid for tone generation choice
void display_tone_grid(void) {
    lcd_clear_and_home();
    lcd_goto_position(0, 8);  
    lcd_write_string(PSTR("CDE"));
    lcd_goto_position(1, 8);  
    lcd_write_string(PSTR("FGA"));
    lcd_goto_position(2, 8);  
    lcd_write_string(PSTR("BC."));
    lcd_goto_position(3, 8);  
    lcd_write_string(PSTR("..X"));
}

//********************************************************************************************************
//*					Task 1 - Function 1 - Sound Production												 *
//********************************************************************************************************

//Custom delay_us to allow for dynamic variables to be used (unlike AVR Library _delay_us

void delay_us(uint16_t us) {
    while (us--) {
        _delay_us(1);
    }
}

void play_tone(uint16_t frequency) {
    uint16_t delayTime = 500000 / frequency / 2;
    
    for (uint32_t i = 0; i < (frequency / 2)/4; ++i) {
        PORTB |= (1 << PB0);
        delay_us(delayTime);
        PORTB &= ~(1 << PB0);
        delay_us(delayTime);
    }
}

void hold_tone(uint16_t frequency) {
    uint16_t delayTime = 500000 / frequency / 2;
    
    while (keypressed()){
        PORTB |= (1 << PB0);
        delay_us(delayTime);
        PORTB &= ~(1 << PB0);
        delay_us(delayTime);
    }
}

void play_tones(void) {
    display_tone_grid();

    while(1) {
        char input = get_keypad_input();

        if (input == '#') {
            display_menu();
            return;
        }

        switch(input) {
            case '1':
                hold_tone(freq_C4);
                break;
            case '2':
                hold_tone(freq_D4);
                break;
            case '3':
                hold_tone(freq_E4);
                break;
            case '4':
                hold_tone(freq_F4);
                break;
            case '5':
                hold_tone(freq_G4);
                break;
            case '6':
                hold_tone(freq_A4);
                break;
            case '7':
                hold_tone(freq_B4);
                break;
            case '8':
                hold_tone(freq_C5);
                break;
            default:
                break;
        }
    }
}

void c_scale() {
    DDRB |= (1 << PB0);
    
    play_tone(freq_C4);
    _delay_ms(500);
    play_tone(freq_D4);
    _delay_ms(500);
    play_tone(freq_E4);
    _delay_ms(500);
    play_tone(freq_F4);
    _delay_ms(500);
    play_tone(freq_G4);
    _delay_ms(500);
    play_tone(freq_A4);
    _delay_ms(500);
    play_tone(freq_B4);
    _delay_ms(500);
    play_tone(freq_C5);
    _delay_ms(500);

    display_menu();
    return;
}

//********************************************************************************************************
//*					Task 1 and 2 - Shared Functions Frogger												*
//********************************************************************************************************

//Checks if space is clear for vehicle spawn
bool is_space_clear(int x, int y, int length) {
    for (int i = 0; i < length; i++) {
        if (grid[y][x + i] != ' ') {
            return false;
        }
    }
    return true;
}

//Reverse Vehicle string for both directions
void reverse_string(char* str, char* reversed, int length) {
    for (int i = 0; i < length; i++) {
        reversed[i] = str[length - i - 1];
    }
    reversed[length] = '\0';
}

void spawn_vehicle() {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i].active = true;
            vehicles[i].y = rand() % 2 + 1;
            int length;
            if (rand() % 2 == 0) {
                length = 5;
                strcpy(vehicles[i].representation, "####T");
            } else {
                length = 3;
                strcpy(vehicles[i].representation, "##8");
            }
            reverse_string(vehicles[i].representation, vehicles[i].reversed_representation, length);
            vehicles[i].length = length;
            if (vehicles[i].y == 1) {
                vehicles[i].x = 0;
                if (!is_space_clear(vehicles[i].x, vehicles[i].y, length)) {
                    vehicles[i].active = false;
                    continue;
                }
            } else {
                vehicles[i].x = GRID_WIDTH - length;
                if (!is_space_clear(vehicles[i].x, vehicles[i].y, length)) {
                    vehicles[i].active = false;
                    continue;
                }
            }
            break;
        }
    }
}

void update_vehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) {
            if (vehicles[i].y == 1) {
                vehicles[i].x += 1;
            } else {
                vehicles[i].x -= 1;
            }

            if (vehicles[i].x < 0 || vehicles[i].x + vehicles[i].length > GRID_WIDTH) {
                vehicles[i].active = false;
            }
        }
    }
}

void render_vehicles() {
	for (int i = 0; i < MAX_VEHICLES; i++) {
		if (vehicles[i].active) {
			int x = vehicles[i].x;
			int y = vehicles[i].y;
			char* representation = vehicles[i].y == 1 ? vehicles[i].representation : vehicles[i].reversed_representation;
			for (int j = 0; j < vehicles[i].length; j++) {
				if (x + j < GRID_WIDTH && y < GRID_HEIGHT) {
					grid[y][x + j] = representation[j];
				}
			}
		}
	}
}

void reset_vehicles() {
	for (int i = 0; i < MAX_VEHICLES; i++) {
		vehicles[i].active = false;
	}
}

//Game State

void reset_game() {
	// Reset the frog position
	frogX = GRID_WIDTH / 2;
	frogY = GRID_HEIGHT - 1;
	previousFrogY = frogY;

	// Reset the score, timer, and frogs at home count
	score = 0;
	timer = 20;
	
	if (frogsAtHome == 3){
		frogsAtHome = 0;
	}

	reset_vehicles();

	// Clear the game grid
	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			grid[y][x] = ' ';
		}
	}

	// Re-initialize home positions
	for (int x = 0; x < GRID_WIDTH; x++) {
		if (x == 7) {
			grid[0][x] = 'G';
			} else if (x == 9) {
			grid[0][x] = 'O';
			} else if (x == 11) {
			grid[0][x] = 'A';
			} else if (x == 13) {
			grid[0][x] = 'L';
			} else if (x % 2 == 0 || x % 3 == 0) {
			grid[0][x] = '?';
			} else {
			grid[0][x] = ' ';
		}
	}
}

void game_init() {
	reset_game();
	// Clear grid
	for (int y = 1; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			grid[y][x] = ' ';
		}
	}
}

//Scoring

void display_live_score(int score) {
	lcd_goto_position(3, 0);
	lcd_write_int16(score); 
	_delay_ms(100);
}

void display_final_score(int score) {
	lcd_clear_and_home();  
	lcd_goto_position(1, 4);
	lcd_write_string(PSTR("Score: "));
	lcd_write_int16(score);  
	_delay_ms(2000);
}

void display_time(int timer) {
	lcd_goto_position(3, 17);
	lcd_write_int16(timer);
	_delay_ms(100);
}

//Collisions

void handle_collision() {
	lives -= 1;
	lcd_clear_and_home();
	lcd_goto_position(1, 5);
	lcd_write_string(PSTR("SPLAT!"));
	play_tone(freq_E4);
	play_tone(freq_C4);
	_delay_ms(1000); 
	game_init();  
}

void check_collisions() {
	for (int i = 0; i < MAX_VEHICLES; i++) {
		if (vehicles[i].active) {
			int x = vehicles[i].x;
			int y = vehicles[i].y;
			for (int j = 0; j < vehicles[i].length; j++) {
				if (frogX == x + j && frogY == y) {
					handle_collision();  
					return;  
				}
			}
		}
	}

	// Check for victory
	if (frogY == 0 && grid[frogY][frogX] == ' ') {
		// Victory
		lcd_clear_and_home();
		lcd_goto_position(1, 6);
		lcd_write_string(PSTR("Victory!"));
		play_tone(freq_C4);
		play_tone(freq_C4);
		play_tone(freq_C4);
		play_tone(freq_E4);
		play_tone(freq_C4);
		game_init();  // Reset the game
		return;
	}
}

//********************************************************************************************************
//*					Task 1 - Function 2 - Indefinite Frogger											 *
//********************************************************************************************************

void update_game_state(char input) {
    // Spawn vehicles and update their positions
    if (rand() % 3 == 0) {
        spawn_vehicle();
    }
    update_vehicles();

    // Clear grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }
    
    // Re-initialize home positions
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (x % 2 == 0 || x % 3 == 0) {
            grid[0][x] = '?';
        } 
    }
    
    // Update frog position based on input
    int newFrogX = frogX;
    int newFrogY = frogY;
    if (input == '8' && frogY > 0) newFrogY--;  // Up
    if (input == '0' && frogY < GRID_HEIGHT - 1) newFrogY++;  // Down
    if (input == '*' && frogX > 0) newFrogX--; // Left
    if (input == '#' && frogX < GRID_WIDTH - 1) newFrogX++;  // Right
    
    if(keypressed()) play_tone(freq_F4);
    
    // Check if the new position is a wall
    if (grid[newFrogY][newFrogX] == '?') {
        handle_collision();  // Call the function if a wall is hit
        return;  // Exit early from update_game_state
    }

    // If not, update the frog's position
    frogX = newFrogX;
    frogY = newFrogY;
    
    // Check for collisions
    check_collisions();  

    render_vehicles();
    
    // Update grid with new positions
    grid[frogY][frogX] = '&';
}

void render_game() {
    lcd_home();
    // Render home row
    lcd_goto_position(0, 0);  // Set the cursor position at the beginning of the first row
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (x % 2 == 0 || x % 3 == 0) {
            lcd_write_data(0xFF);  // Write block character to LCD
        } else {
            lcd_write_data(' ');  // Write space to LCD
        }
    }
    // Render the rest of the grid
    for (int y = 1; y < GRID_HEIGHT; y++) {
        lcd_goto_position(y, 0);  
        for (int x = 0; x < GRID_WIDTH; x++) {
            lcd_write_data(grid[y][x]);
        }
    }
	// Render the frog last, so it appears on top of other elements
    lcd_goto_position(frogY, frogX);
    lcd_write_data('&'); 
}

void play_frogger(void) {
    seed += 9;
    game_init();
    while(1) {
        char input = get_keypad_input();
        update_game_state(input);
        render_game();
        _delay_ms(100);
        if (input == '1') {  // '1' is the key to exit back to the menu
            seed -= 3;
            lives = 3;
            display_menu();
            return; 
        }
    }
}

//********************************************************************************************************
//*					Task 2 - Function 2 - Scored Frogger					  						     *
//********************************************************************************************************

// Spawn vehicles and update their positions
void update_game_state_scored(char input, int* score) {
    if (rand() % 2 == 0) {  // 10% chance to spawn a new vehicle each frame
        spawn_vehicle();
    }
    update_vehicles();

    // Clear grid but preserve 'X' characters in the scoring row (row 0)
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (y != 0) {  
                grid[y][x] = ' ';  
            } else {
                if (grid[y][x] != 'X') {
                    grid[y][x] = ' ';
                }
            }
        }
    }

    // Re-initialize home positions
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (x % 2 == 0 || x % 3 == 0) {
            grid[0][x] = '?';
        } else {
            grid[3][x] = ' ';
        }
    }

    // Update frog position based on input and play sound
    int newFrogX = frogX;
    int newFrogY = frogY;
    if (input == '8' && frogY > 0) newFrogY--;  
    if (input == '0' && frogY < GRID_HEIGHT - 1) newFrogY++;  
    if (input == '*' && frogX > 0) newFrogX--;  
    if (input == '#' && frogX < GRID_WIDTH - 1) newFrogX++;  
    if(keypressed()) play_tone(freq_F4);

    // Check if the frog has entered a new row
    if (newFrogY != previousFrogY) {
        if (newFrogY != 0) {
            *score += 20;  
        }
        previousFrogY = newFrogY;  
    }

    // Check if the new position is a wall or X on home row
    if (grid[newFrogY][newFrogX] == '?' || (grid[newFrogY][newFrogX] == 'X' && newFrogY == 0)) {
        handle_collision();  
        return;  
    }

    frogX = newFrogX;
    frogY = newFrogY;

    // Check if the frog has reached a goal
    if (frogY == 0 && grid[frogY][frogX] == ' ' ) {
        play_tone(freq_C4);
        play_tone(freq_E4);
        frogsAtHome += 1;
        lives -= 1;
        *score += 10 * (timer + 1); 

		//Update Frog Position if at home
        if(frogsAtHome > 0 && frogsAtHome < 2) {
            frogMarkY = frogY;
            frogMarkX = frogX;		
        }

        if(frogsAtHome > 1) {
            frogMarkY2 = frogY;
            frogMarkX2 = frogX;
        }

        frogX = GRID_WIDTH / 2;
        frogY = GRID_HEIGHT - 1;
        timer = 20;

        if (frogsAtHome == 3) {
            *score += 500;
        }
    }

    check_collisions();  
    render_vehicles();
    display_live_score(*score);
	display_time(timer);
	timer -= 0.33;
    grid[frogY][frogX] = '&';
}

void render_game_scored(int score) {
    lcd_home();					
    lcd_goto_position(0, 0);  
    for (int x = 0; x < GRID_WIDTH; x++) {
        if (x % 2 == 0 || x % 3 == 0) {
            lcd_write_data(0xFF);  
        } else {
            lcd_write_data(' ');  
        }
    }

    for (int y = 1; y < GRID_HEIGHT; y++) {
        lcd_goto_position(y, 0);  
        for (int x = 0; x < GRID_WIDTH; x++) {
            lcd_write_data(grid[y][x]);
        }
    }

    display_live_score(score);
    
	
	//if Frog at home remove Life/Frogs left to get home
    if(frogsAtHome > 0 ) {
        grid[frogMarkY][frogMarkX] = 'X';
        lcd_goto_position(frogMarkY, frogMarkX);
        lcd_write_data('X');
    }

    if(frogsAtHome == 2) {
        grid[frogMarkY2][frogMarkX2] = 'X';
        lcd_goto_position(frogMarkY2, frogMarkX2);
        lcd_write_data('X');
    }

    if(lives > 2) {
        lcd_goto_position(0,19);
        lcd_write_data('X');
    } else {
        lcd_goto_position(0,19);
        lcd_write_data(' ');
    }

    if(lives > 1) {
        lcd_goto_position(1,19);
        lcd_write_data('X');
    } else {
        lcd_goto_position(1,19);
        lcd_write_data(' ');
    }

    if(lives > 0) {
        lcd_goto_position(2,19);
        lcd_write_data('X');
    } else {
        lcd_goto_position(2,19);
        lcd_write_data(' ');
    }

    lcd_goto_position(frogY, frogX);
    lcd_write_data('&');
}

int check_game_over() {
    if (timer <= 0 || lives <= 0) {
        return 1;  
    }
    if (frogsAtHome == 3) {
        return 2;  
    }
    return 0;  
}

void play_scored_frogger(int score) {
    seed += 5;

    for (int i = 3; i > 0; i--) {
        lcd_clear_and_home();
        lcd_goto_position(1, 8);  
        lcd_write_int16(i);
        play_tone(freq_C4);
        _delay_ms(500);
    }
    lcd_clear_and_home();
    lcd_goto_position(1, 8);
    lcd_write_string(PSTR("Go!"));
    play_tone(freq_C5);
    _delay_ms(1000);

    game_init();

    while(1) {
        char input = get_keypad_input();
        update_game_state_scored(input, &score);
        render_game_scored(score);
        int gameStatus = check_game_over();

        if (gameStatus == 1 || input == '1') {
            seed -= 2;
            lcd_clear_and_home();
            lcd_goto_position(1, 6);
            lcd_write_string(PSTR("Game Over!"));
            play_tone(freq_E4);
            play_tone(freq_E4);
            play_tone(freq_D4);
            play_tone(freq_D4);
            play_tone(freq_C4);
            play_tone(freq_C4);
            display_final_score(score);
            lives = 3;
            frogsAtHome = 0;
			display_menu();
            break;
        }
        if (gameStatus == 2) {
            seed += 10;
            lcd_clear_and_home();
            lcd_goto_position(1, 6);
            lcd_write_string(PSTR("You Win!"));
            play_tone(freq_E4);
            play_tone(freq_G4);
            play_tone(freq_A4);
            play_tone(freq_G4);
            play_tone(freq_F4);
            play_tone(freq_E4);
            play_tone(freq_D4);
            play_tone(freq_C4);
            display_final_score(score);
            lives = 3;
            frogsAtHome = 0;
			display_menu();
            break;
        }
    }
}

//********************************************************************************************************
//*					Main																				 *
//********************************************************************************************************

int main(void) {
    // initialise MCU, drivers and middleware
    atmel_start_init();
  
    // Initialise the LCD
    lcd_init();
    lcd_home();
 
    // Initialise keypad, and the SPST switch port
    keypad_init();

    // Wait a moment, then initialise interrupts
    _delay_ms(1);
    init_interrupts();
    cli();  // Disable interrupts again initially
    display_menu();

    while (1) {
        char choice = get_keypad_input();  // Get user selection from keypad
        srand(seed);
        _delay_ms(500);  // Add a small delay to ensure any previous keypad input has been cleared

        switch (choice) {
            case '1':
                play_tones();
                break;
            case '2':
                play_frogger();
                break;
            case '3':
                play_scored_frogger(score);
                break;
            case '4':
                c_scale();
                break;
            default:
                // Invalid choice
                break;
        }
    }
    
    return 0;
}
