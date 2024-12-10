#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "hardware/timer.h"


/*
GPIO 4 (pin 6)-> SDA on LCD bridge board
   GPIO 5 (pin 7)-> SCL on LCD bridge board
   3.3v (pin 36) -> VCC on LCD bridge board
   GND (pin 38)  -> GND on LCD bridge board


*/
   

void i2c_write_byte(uint8_t val);
void lcd_toggle_enable(uint8_t val);

void lcd_send_byte(uint8_t val, int mode);

void lcd_clear(void);

void lcd_set_cursor(int line, int position);

static void lcd_char(char val);

void lcd_string(char *s);

void lcd_init(uint16_t SDA, uint16_t SCL);

void Barrel(int pos);
