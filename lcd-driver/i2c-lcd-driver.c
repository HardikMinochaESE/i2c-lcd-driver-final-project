#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define LCD_ADDR 0x27
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_CMD 0x00
#define LCD_DATA 0x01

// LCD Commands
#define LCD_CLEAR 0x01
#define LCD_HOME 0x02
#define LCD_ENTRY_MODE 0x04
#define LCD_DISPLAY_CTRL 0x08
#define LCD_CURSOR_SHIFT 0x10
#define LCD_FUNCTION_SET 0x20
#define LCD_SET_CGRAM 0x40
#define LCD_SET_DDRAM 0x80

// Entry mode options
#define LCD_ENTRY_RIGHT 0x00
#define LCD_ENTRY_LEFT 0x02
#define LCD_ENTRY_SHIFT_INC 0x01
#define LCD_ENTRY_SHIFT_DEC 0x00

// Display control options
#define LCD_DISPLAY_ON 0x04
#define LCD_DISPLAY_OFF 0x00
#define LCD_CURSOR_ON 0x02
#define LCD_CURSOR_OFF 0x00
#define LCD_BLINK_ON 0x01
#define LCD_BLINK_OFF 0x00

// Function set options
#define LCD_8BIT_MODE 0x10
#define LCD_4BIT_MODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

static struct i2c_client *lcd_client;

// Function to write a nibble to the LCD
static void lcd_write_nibble(struct i2c_client *client, u8 nibble, u8 mode)
{
    u8 data = nibble | mode | LCD_BACKLIGHT;
    i2c_smbus_write_byte(client, data | LCD_ENABLE);
    udelay(1);
    i2c_smbus_write_byte(client, data & ~LCD_ENABLE);
    udelay(50);
}

// Function to write a byte to the LCD
static void lcd_write_byte(struct i2c_client *client, u8 byte, u8 mode)
{
    lcd_write_nibble(client, byte >> 4, mode);
    lcd_write_nibble(client, byte & 0x0F, mode);
}

// Function to initialize the LCD
static void lcd_init(struct i2c_client *client)
{
    // Wait for LCD to power up
    msleep(50);

    // Initialize LCD in 4-bit mode
    lcd_write_nibble(client, 0x03, LCD_CMD);
    msleep(5);
    lcd_write_nibble(client, 0x03, LCD_CMD);
    udelay(150);
    lcd_write_nibble(client, 0x03, LCD_CMD);
    lcd_write_nibble(client, 0x02, LCD_CMD);

    // Set 4-bit mode, 2 lines, 5x8 dots
    lcd_write_byte(client, LCD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS, LCD_CMD);
    
    // Turn on display with cursor blinking
    lcd_write_byte(client, LCD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_ON | LCD_BLINK_ON, LCD_CMD);
    
    // Clear display
    lcd_write_byte(client, LCD_CLEAR, LCD_CMD);
    msleep(2);
    
    // Set entry mode
    lcd_write_byte(client, LCD_ENTRY_MODE | LCD_ENTRY_LEFT, LCD_CMD);
}

static int lcd_probe(struct i2c_client *client)
{
    pr_info("LCD Driver: Probing device\n");
    
    lcd_client = client;
    lcd_init(client);
    
    pr_info("LCD Driver: Initialization complete\n");
    return 0;
}

static void lcd_remove(struct i2c_client *client)
{
    pr_info("LCD Driver: Removing device\n");
}

static const struct i2c_device_id lcd_id[] = {
    { "i2c-lcd", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, lcd_id);

static struct i2c_driver lcd_driver = {
    .driver = {
        .name = "i2c-lcd",
        .owner = THIS_MODULE,
    },
    .probe = lcd_probe,
    .remove = lcd_remove,
    .id_table = lcd_id,
};

module_i2c_driver(lcd_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("I2C LCD 16x2 Display Driver");
MODULE_VERSION("1.0"); 