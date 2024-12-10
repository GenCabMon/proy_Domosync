#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "hardware/timer.h"

void i2c_write_byte(uint8_t val);
void lcd_toggle_enable(uint8_t val);
void lcd_send_byte(uint8_t val, int mode);
void lcd_clear(void);
void lcd_set_cursor(int line, int position);
static void lcd_char(char val);
void lcd_string(char *s);
void lcd_init(uint16_t SDA, uint16_t SCL);

#endif // LCD_I2C_H
