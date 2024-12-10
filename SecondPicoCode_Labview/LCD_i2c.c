#include "LCD_i2c.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;
const int LINEA2 = 0xC0;
const int LINEA3 = 0x94;
const int LINEA4 = 0xD4;
// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;
// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;
// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;
// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;
// flag for backlight control
const int LCD_BACKLIGHT = 0x08;
const int LCD_ENABLE_BIT = 0x04;
// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;
// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      4
#define MAX_CHARS      16

/// @brief Envia el dato hacia el modulo i2c del LCD
/// @param val comando
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c1, addr, &val, 1, false);
#endif
}
/// @brief Activa el pin "EN" del display LCD, crea un pulso de 500uS
/// @param val comando
void lcd_toggle_enable(uint8_t val) {
#define DELAY_US 500
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT); // Envia el dato hacia el modulo i2c + el pin de EN en 1
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);// Envia el dato hacia el modulo i2c + el pin de EN en 0
    sleep_us(DELAY_US);
}

/// @brief Envia un byte en dos secciones
/// @param val Operacion
/// @param mode Determina el tipo de dato que se va a enviar al display lcd
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

   //i2c_write_byte(high);
    lcd_toggle_enable(high);
  // i2c_write_byte(low);
    lcd_toggle_enable(low);
}
/// @brief Limpia la pantalla LCD
/// @param  
void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

/// @brief Mueve el cursor a una posicion determinada
/// @param line Linea o fila
/// @param position Posicion en la linea
void lcd_set_cursor(int line, int position) {
    int val;
    if (line == 0) {
        val = 0x80 + position;  // Primera línea
    } else if (line == 1) {
        val = 0xC0 + position;  // Segunda línea
    } else if (line == 2) {
        val = 0x94 + position;  // Tercera línea (para un LCD de 4 líneas)
    } else if (line == 3) {
        val = 0xD4 + position;  // Cuarta línea (para un LCD de 4 líneas)
    } else {
        val = 0x80;  // O algún valor por defecto
    }
    lcd_send_byte(val, LCD_COMMAND);
}
/// @brief Envia un caracter a la pantalla
/// @param val Caracter
static void inline lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}
/// @brief Envia un string a la pantalla
/// @param s String
void lcd_string(char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

/// @brief Inicializa la pantalla LCD, el spi se configura por defecto a 100Khz y utiliza el spi1 de la Raspberry Pi Pico
/// @param SDA Pin para el SDA, Asociado al spi1
/// @param SCL Pin para el SCL, Asociado al spi1
void lcd_init(uint16_t SDA,uint16_t SCL) {

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
    // Make the I2C pins available to picotool
   // bi_decl(bi_2pins_with_func(SDA, SCL, GPIO_FUNC_I2C));
    bi_decl(bi_2pins_with_func(14, 15, GPIO_FUNC_I2C));
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);
    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();

}
/// @brief Intento de mover los caracteres pero no funciona. :(
/// @param pos 
void Barrel(int pos){
    lcd_send_byte(LCD_MOVERIGHT + pos,LCD_COMMAND);

}