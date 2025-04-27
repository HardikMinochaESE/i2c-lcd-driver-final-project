#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/i2c-dev.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/device.h>

#define LCD_ADDR 0x27

// PCF8574 Pin Definitions for LCD
#define PIN_RS      (1 << 0)  // P0 - Register Select
#define PIN_RW      (1 << 1)  // P1 - Read/Write
#define PIN_EN      (1 << 2)  // P2 - Enable
#define PIN_BL      (1 << 3)  // P3 - Backlight
#define PIN_D4      (1 << 4)  // P4 - Data 4
#define PIN_D5      (1 << 5)  // P5 - Data 5
#define PIN_D6      (1 << 6)  // P6 - Data 6
#define PIN_D7      (1 << 7)  // P7 - Data 7

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
    I2C_BOARD_INFO("i2c-lcd", LCD_ADDR),
};

static struct timer_list update_timer;
static struct class *lcd_class;
static struct device *lcd_device;
static int current_temp = 0;

// Function to update LCD display
static void update_lcd_display(void)
{
    int whole, decimal;
    
    // Split into whole and decimal parts
    whole = current_temp / 1000;
    decimal = current_temp % 1000;

    // Set cursor to second line
    lcd_command(lcd_client, LCD_SET_DDRAM | 0x40);
    msleep(1);

    // Write temperature with format XX.XXX Deg C
    lcd_data(lcd_client, (whole / 10) + '0');
    lcd_data(lcd_client, (whole % 10) + '0');
    lcd_data(lcd_client, '.');
    lcd_data(lcd_client, ((decimal / 100) % 10) + '0');
    lcd_data(lcd_client, ((decimal / 10) % 10) + '0');
    lcd_data(lcd_client, (decimal % 10) + '0');
    lcd_write_string(lcd_client, " Deg C");
}

// Sysfs attribute show/store functions
static ssize_t temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", current_temp);
}

static ssize_t temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    int temp;

    ret = kstrtoint(buf, 10, &temp);
    if (ret < 0)
        return ret;

    current_temp = temp;
    
    // Update LCD display immediately when new value is written
    if (lcd_client) {
        update_lcd_display();
    }
    
    return count;
}

static DEVICE_ATTR(lcd_data_feed, 0664, temp_show, temp_store);

// Write a nibble to the LCD through PCF8574
static void lcd_write_nibble(struct i2c_client *client, u8 nibble, u8 rs) 
{
    u8 data;
    
    // Prepare data with backlight ON
    data = PIN_BL;  // Keep backlight on
    
    // Set RS and R/W (R/W is always 0 for write)
    if (rs) data |= PIN_RS;
    
    // Map the nibble to D4-D7 pins
    if (nibble & 0x01) data |= PIN_D4;
    if (nibble & 0x02) data |= PIN_D5;
    if (nibble & 0x04) data |= PIN_D6;
    if (nibble & 0x08) data |= PIN_D7;
    
    // Write with EN=0
    i2c_smbus_write_byte(client, data);
    udelay(1);
    
    // Write with EN=1
    i2c_smbus_write_byte(client, data | PIN_EN);
    udelay(1);  // Min 450ns required
    
    // Write with EN=0
    i2c_smbus_write_byte(client, data);
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

// Write string to LCD
static void lcd_write_string(struct i2c_client *client, const char *str)
{
    while (*str) {
        lcd_data(client, *str++);
        udelay(100);  // Give LCD time to process each char
    }
}

// Function to initialize the LCD
static void lcd_init(struct i2c_client *client)
{
    pr_info("LCD Driver: Starting initialization\n");
    
    // Wait for power up (>15ms)
    msleep(50);

    // Initialize in 4-bit mode
    lcd_command(client, 0x02);  // 4-bit mode
    msleep(5);
    
    // Function Set: 2-line, 4-bit mode, 5x8 dots (0x28)
    lcd_command(client, 0x28);
    msleep(5);
    
    // Display ON, Cursor Blinking (0x0F)
    lcd_command(client, 0x0F);
    msleep(5);
    
    // Entry Mode Set: Increment cursor (0x06)
    lcd_command(client, 0x06);
    msleep(5);
    
    // Clear Display (0x01)
    lcd_command(client, 0x01);
    msleep(5);
    
    // Set cursor to home position (0x80)
    lcd_command(client, 0x80);
    msleep(5);

    pr_info("LCD Driver: Initialization complete\n");
}

static int lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    pr_info("LCD Driver: Probing device\n");
    
    lcd_client = client;
    
    // Initialize LCD
    lcd_init(client);
    msleep(10);
    
    // Write header on first line
    lcd_command(client, LCD_SET_DDRAM | 0x00);
    msleep(1);
    lcd_write_string(client, "CPU Temp Monitor");
    
    // Initialize second line
    lcd_command(client, LCD_SET_DDRAM | 0x40);
    msleep(1);
    lcd_write_string(client, "00.000 Deg C");
    
    // Create class
    lcd_class = class_create(THIS_MODULE, "lcd_class");
    if (IS_ERR(lcd_class)) {
        pr_err("LCD Driver: Failed to create class\n");
        return PTR_ERR(lcd_class);
    }

    // Create device
    lcd_device = device_create(lcd_class, NULL, MKDEV(0, 0), NULL, "lcd_dev");
    if (IS_ERR(lcd_device)) {
        pr_err("LCD Driver: Failed to create device\n");
        class_destroy(lcd_class);
        return PTR_ERR(lcd_device);
    }

    // Create sysfs attribute
    ret = device_create_file(lcd_device, &dev_attr_lcd_data_feed);
    if (ret) {
        pr_err("LCD Driver: Failed to create sysfs file\n");
        device_destroy(lcd_class, MKDEV(0, 0));
        class_destroy(lcd_class);
        return ret;
    }

    pr_info("LCD Driver: Initialization complete\n");
    return 0;
}

static int lcd_remove(struct i2c_client *client)
{
    pr_info("LCD Driver: Removing device\n");
    
    // Clean up sysfs and class
    if (lcd_device) {
        device_remove_file(lcd_device, &dev_attr_lcd_data_feed);
        device_destroy(lcd_class, MKDEV(0, 0));
    }
    if (lcd_class) {
        class_destroy(lcd_class);
    }
    
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
        pr_info("LCD Driver: Successfully loaded\n");
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
    pr_info("LCD Driver: Module unloaded\n");
}

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HARDIK MINOCHA");
MODULE_DESCRIPTION("I2C LCD 16x2 Display Driver");
MODULE_VERSION("1.0"); 
