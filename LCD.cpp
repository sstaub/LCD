
/* Arduino TextLCD Library, for a 4-bit LCD based on HD44780
 * Copyright (c) 2020, sstaub
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "LCD.h"

LCD::LCD(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
	this->rs = rs;
	this->enable = enable;
	dataPins[0] = d4;
	dataPins[1] = d5;
	dataPins[2] = d6;
	dataPins[3] = d7;
	}

void LCD::begin(uint8_t columns, uint8_t rows, uint8_t dotsize) {
	this->rows = rows;
	this->columns = columns;
	setRowOffsets(0x00, 0x40, 0x00 + columns, 0x40 + columns);  
	displayfunction = LCD_4BIT_MODE | LCD_1_LINE | LCD_5x8DOTS;
	if (rows > 1) {
		displayfunction |= LCD_2_LINE;
	}
	if ((dotsize != 0) && (rows == 1)) {
		displayfunction |= LCD_5x10DOTS;
	}

	pinMode(rs, OUTPUT);
	pinMode(enable, OUTPUT);
	
	for (int i = 0; i < 4; i++) {
		pinMode(dataPins[i], OUTPUT);
	 	} 
	_delay_ms(50);
	digitalWrite(rs, LOW);
	digitalWrite(enable, LOW);

	write4bits(0x03);
	_delay_us(4500);
	write4bits(0x03);
	_delay_us(4500);
	write4bits(0x03); 
	_delay_us(150);
	write4bits(0x02); 
	command(LCD_FUNCTION_SET | displayfunction);  
	displaycontrol = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;  
	display(DISPLAY_ON);
	cls();
	displaymode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
	command(LCD_ENTRY_MODE_SET | displaymode);
	}

void LCD::setRowOffsets(int row0, int row1, int row2, int row3) {
	rowOffsets[0] = row0;
	rowOffsets[1] = row1;
	rowOffsets[2] = row2;
	rowOffsets[3] = row3;
	}

void LCD::cls() {
	command(LCD_CLEAR_DISPLAY);
	_delay_us(2000);
	}

void LCD::home() {
	command(LCD_RETURN_HOME);
	_delay_us(2000);
	}

void LCD::locate(uint8_t column, uint8_t row) {
	const size_t max_rows = sizeof(rowOffsets) / sizeof(*rowOffsets);
	if (row >= max_rows) row = max_rows - 1;
	if (row >= rows) row = rows - 1;
	command(LCD_SET_DDRAM_ADDR | (column + rowOffsets[row]));
	}

void LCD::display(mode_t mode) {
	switch(mode) {
		case DISPLAY_ON :
			displaycontrol |= LCD_DISPLAY_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case DISPLAY_OFF:
			displaycontrol &= ~LCD_DISPLAY_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case CURSOR_ON:
			displaycontrol |= LCD_CURSOR_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case CURSOR_OFF:
			displaycontrol &= ~LCD_CURSOR_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case BLINK_ON:
			displaycontrol |= LCD_BLINK_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case BLINK_OFF:
			displaycontrol &= ~LCD_BLINK_ON;
			command(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case SCROLL_LEFT:
			command(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT);
			break;
		case SCROLL_RIGHT:
			command(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT);
			break;
		case LEFT_TO_RIGHT:
			displaymode |= LCD_ENTRY_LEFT;
			command(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case RIGHT_TO_LEFT:
			displaymode &= ~LCD_ENTRY_LEFT;
			command(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case AUTOSCROLL_ON:
			displaymode |= LCD_ENTRY_SHIFT_INCREMENT;
			command(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case AUTOSCROLL_OFF:
			displaymode &= ~LCD_ENTRY_SHIFT_INCREMENT;
			command(LCD_ENTRY_MODE_SET | displaymode);
			break;
		}
	}

void LCD::create(uint8_t location, uint8_t charmap[]) {
	location &= 0x7;
	command(LCD_SET_CGRAM_ADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
		}
	}

void LCD::character(uint8_t column, uint8_t row, char c) {
	locate(column, row);
	write(c);
	}

void LCD::printf(const char *format, ...) {
	char lcd_buffer[columns + 1];
	va_list args;
	va_start(args, format);
	vsnprintf(lcd_buffer, columns + 1, format, args);
	va_end(args);
	for (char *i = lcd_buffer; *i; i++) {
		write(*i);
		}
	}

void LCD::command(uint8_t value) {
	send(value, LOW);
	}

size_t LCD::write(uint8_t value) {
	send(value, HIGH);
	return 1;
	}

void LCD::send(uint8_t value, uint8_t mode) {
	digitalWrite(rs, mode);
	write4bits(value>>4);
	write4bits(value);
	}

void LCD::pulseEnable(void) {
	digitalWrite(enable, LOW);
	_delay_us(1);
	digitalWrite(enable, HIGH);
	_delay_us(1);
	digitalWrite(enable, LOW);
	_delay_us(40);
	}

void LCD::write4bits(uint8_t value) {
	for (int i = 0; i < 4; i++) {
		digitalWrite(dataPins[i], (value >> i) & 0x01);
		}
	pulseEnable();
	}