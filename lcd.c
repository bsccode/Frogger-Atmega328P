// lcd.c
// for NerdKits
// mrobbins@mit.edu
//
// PIN DEFINITIONS:
//
//
// PD2-5 -- LCD DB4-7 (pins 11-14) (was PB3-6)
// PD6 -- LCD E (pin 6) (was PA5)
// PD7 -- LCD RS (pin 4) (was PA4)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdio.h>

#include "lcd.h"

// lcd_set_type_data()
void lcd_set_type_data() {
  PORTC |= (1<<PC5);
}

// lcd_set_type_command()
void lcd_set_type_command() {
  PORTC &= ~(1<<PC5);
}

// lcd_write_nibble(...)
void lcd_write_nibble(char c) {

  // NOTE: only 2 or 3 work in the _delays here.

  // set data
  PORTC &= ~(0x0f);
  PORTC |= (c&0x0f) ;

  // E high
  PORTC |= (1<<PC4);
   _delay_us(1);
  // E low
  PORTC &= ~(1<<PC4);
   _delay_us(1);
  
}

void lcd_write_byte(char c) {
  lcd_write_nibble( (c >> 4) & 0x0f );
  lcd_write_nibble( c & 0x0f );
   _delay_us(80*LCD_DELAY);
}

void lcd_clear_and_home() {
  lcd_set_type_command();
  lcd_write_byte(0x01);
   _delay_ms(50*LCD_DELAY);
  lcd_write_byte(0x02);
   _delay_ms(50*LCD_DELAY);
}

void lcd_home() {
  lcd_set_type_command();
  lcd_write_byte(0x02);
   _delay_ms(50*LCD_DELAY);
}

void lcd_write_data(char c) {
  lcd_set_type_data();
  lcd_write_byte(c);
}

// lcd_write_int16
void lcd_write_int16(int16_t in) {
  uint8_t started = 0;

  uint16_t pow = 10000;

  if(in < 0) {
    lcd_write_data('-');
    in = -in;
  }

  while(pow >= 1) {
    if(in / pow > 0 || started || pow==1) {
      lcd_write_data((uint8_t) (in/pow) + '0');
      started = 1;
      in = in % pow;
    }

    pow = pow / 10;
  }

}


// lcd_write_int16_centi
// assumes that its measured in centi-whatevers
void lcd_write_int16_centi(int16_t in) {
  uint8_t started = 0;

  uint16_t pow = 10000;

  if(in < 0) {
    lcd_write_data('-');
    in = -in;
  }

  while(pow >= 1) {
    if(in / pow > 0 || started || pow==1) {
      lcd_write_data((uint8_t) (in/pow) + '0');
      started = 1;
      in = in % pow;
    }

    if(pow == 100) {
      if(!started) {
        lcd_write_data('0');
      }
      lcd_write_data('.');
      started = 1;
    }

    pow = pow / 10;
  }

}

void lcd_write_string(const char *x) {
  // assumes x is in program memory
  while(pgm_read_byte(x) != 0x00)
    lcd_write_data(pgm_read_byte(x++));
}

void lcd_goto_position(uint8_t row, uint8_t col) {
  lcd_set_type_command();

  // 20x4 LCD: offsets 0, 0x40, 20, 0x40+20
  uint8_t row_offset = 0;
  switch(row) {
    case 0: row_offset = 0; break;
    case 1: row_offset = 0x40; break;
    case 2: row_offset = 20; break;
    case 3: row_offset = 0x40+20; break;
  }
  
  
  lcd_write_byte(0x80 | (row_offset + col));
}

void lcd_line_one()   { lcd_goto_position(0, 0); }
void lcd_line_two()   { lcd_goto_position(1, 0); }
void lcd_line_three() { lcd_goto_position(2, 0); }
void lcd_line_four()  { lcd_goto_position(3, 0); }

// lcd_init()
void lcd_init() {
  // set pin driver directions
  // (output on PD7,PD6, and PD3-6)
  DDRC |= 0x3f;

  // wait 100msec
   _delay_ms(100*LCD_DELAY);
  lcd_set_type_command();
  
  // do reset
  lcd_write_nibble(0x03);
   _delay_ms(6*LCD_DELAY);
  lcd_write_nibble(0x03);
   _delay_us(250*LCD_DELAY);
  lcd_write_nibble(0x03);
   _delay_us(250*LCD_DELAY);
  
  // write 0010 (data length 4 bits)
  lcd_write_nibble(0x02);
  // set to 2 lines, font 5x8
  lcd_write_byte(0x28);
  // disable LCD
  //lcd_write_byte(0x08);
  // enable LCD
  lcd_write_byte(0x0c);
  // clear display
  lcd_write_byte(0x01);
   _delay_ms(5*LCD_DELAY);
  // enable LCD
  lcd_write_byte(0x0c);
  // set entry mode
  lcd_write_byte(0x06);

  // set cursor/display shift
  lcd_write_byte(0x14);

  // clear and home
  lcd_clear_and_home();
}

int lcd_putchar(char c, FILE *stream) {
  lcd_write_data(c);
  return 0;
}
