#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/i2c-dev.h>

#define LCD_ADDR 0x27

// PCF8574 Pin Definitions for LCD
#define PIN_RS    (1 << 6)  // P6
#define PIN_RW    (1 << 5)  // P5
#define PIN_EN    (1 << 4)  // P4

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
static struct i2c_board_info lcd_board_info = {
    I2C_BOARD_INFO("i2c-lcd", 0x27),
};

// Write a nibble to the LCD through PCF8574
static void lcd_write_nibble(struct i2c_client *client, u8 nibble, u8 rs) 
{
    u8 data;
    
    // Step 1: Set RS first with EN=0
    data = (rs ? PIN_RS : 0);
    i2c_smbus_write_byte(client, data);
    udelay(1);
    
    // Step 2: Set data bits while keeping RS
    data = (nibble & 0x0F) | (rs ? PIN_RS : 0);
    i2c_smbus_write_byte(client, data);
    udelay(1);  // Ensure data is stable
    
    // Step 3: Set EN high - LCD reads data on this edge
    i2c_smbus_write_byte(client, data | PIN_EN);
    udelay(1);  // Min 450ns required
    
    // Step 4: Set EN low
    i2c_smbus_write_byte(client, data);
    
    // Step 5: Wait for command to complete
    udelay(100);  // Commands need > 37us to settle
}

// Write a byte to the LCD (either command or data)
static void lcd_write_byte(struct i2c_client *client, u8 byte, u8 rs)
{
    // Write high nibble first
    lcd_write_nibble(client, byte >> 4, rs);
    
    // Then write low nibble
    lcd_write_nibble(client, byte & 0x0F, rs);
    
    // Extra delay for certain commands
    if (byte == LCD_CLEAR || byte == LCD_HOME)
        msleep(2);  // These commands need longer delay
}

// Write a command to LCD
static void lcd_command(struct i2c_client *client, u8 cmd)
{
    lcd_write_byte(client, cmd, 0);  // RS = 0 for command
}

// Write data to LCD
static void lcd_data(struct i2c_client *client, u8 data)
{
    lcd_write_byte(client, data, 1);  // RS = 1 for data
}

// Function to initialize the LCD
static void lcd_init(struct i2c_client *client)
{
    pr_info("LCD Driver: Starting initialization\n");
    
    // Wait for power up (>15ms)
    msleep(50);

    // Initialize in 4-bit mode
    lcd_write_byte(client, 0x02, 0);  // 4-bit mode
    msleep(5);
    
    // Function Set: 2-line, 4-bit mode, 5x8 dots (0x28)
    lcd_write_byte(client, 0x28, 0);
    msleep(5);
    
    // Display ON, Cursor OFF (0x0C)
    lcd_write_byte(client, 0x0F, 0);
    msleep(5);
    
    // Entry Mode Set: Increment cursor (0x06)
    lcd_write_byte(client, 0x06, 0);
    msleep(5);
    
    // Clear Display (0x01)
    // lcd_write_byte(client, 0x01, 0);
    // msleep(5);
    
    // Set cursor to home position (0x80)
    lcd_write_byte(client, 0x80, 0);
    msleep(5);

    pr_info("LCD Driver: Initialization complete\n");
}

static void lcd_write_string(struct i2c_client *client, const char *str)
{
    while (*str) {
        lcd_write_byte(client, *str++, 1);  // RS=1 for data
        udelay(100);  // Give LCD time to process each char
    }
}

static int lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    pr_info("LCD Driver: Probing device\n");
    
    lcd_client = client;
    
    // // Initialize LCD
    lcd_init(client);
    msleep(10);  // Wait after init
    
    // // Set cursor to first line
    // lcd_write_byte(client, LCD_SET_DDRAM | 0x00, 0);
    // udelay(100);
    
    // // Write first line
    // // lcd_write_string(client, "Driver");
    
    // // Set cursor to second line
    // lcd_write_byte(client, LCD_SET_DDRAM | 0x40, 0);
    // udelay(100);
    
    // // Write second line
    // //lcd_write_string(client, "Loaded");
    
    pr_info("LCD Driver: Display update complete\n");
    return 0;
}

static int lcd_remove(struct i2c_client *client)
{
    pr_info("LCD Driver: Removing device\n");
    return 0;
}

static const struct i2c_device_id lcd_id[] = {
    { "i2c-lcd", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, lcd_id);

static const struct of_device_id lcd_of_match[] = {
    { .compatible = "i2c-lcd" },
    { }
};
MODULE_DEVICE_TABLE(of, lcd_of_match);

static struct i2c_driver lcd_driver = {
    .driver = {
        .name = "i2c-lcd",
        .owner = THIS_MODULE,
        .of_match_table = lcd_of_match,
    },
    .probe = lcd_probe,
    .remove = lcd_remove,
    .id_table = lcd_id,
};

static int __init lcd_driver_init(void)
{
    int ret;
    struct i2c_adapter *adapter;
    
    pr_info("LCD Driver: Starting module initialization\n");
    
    // Get the I2C adapter for bus 1
    adapter = i2c_get_adapter(1);
    if (!adapter) {
        pr_err("LCD Driver: Failed to get I2C adapter\n");
        return -ENODEV;
    }
    
    // Register the board info
    lcd_client = i2c_new_client_device(adapter, &lcd_board_info);
    if (!lcd_client) {
        pr_err("LCD Driver: Failed to create I2C client\n");
        i2c_put_adapter(adapter);
        return -ENODEV;
    }
    
    // Initialize the LCD
    lcd_init(lcd_client);
    
    // Register the driver
    ret = i2c_register_driver(THIS_MODULE, &lcd_driver);
    if (ret == 0) {
        pr_info("LCD Driver: Ho gaya load bkl...\n");
    }
    
    i2c_put_adapter(adapter);
    return ret;
}

static void __exit lcd_driver_exit(void)
{
    if (lcd_client) {
        i2c_unregister_device(lcd_client);
    }
    i2c_del_driver(&lcd_driver);
}

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HARDIK MINOCHA");
MODULE_DESCRIPTION("I2C LCD 16x2 Display Driver");
MODULE_VERSION("1.0"); 
