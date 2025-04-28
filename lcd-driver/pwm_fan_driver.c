#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pwm.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>

/*

    REFERENCES AND ACKNOWLEDGEMENTS:

    This driver was created by following the tutorial at:
    https://www.youtube.com/watch?v=Zy_z_9-7734

    The tutorial was helpful in understanding how to create a character device driver
    and how to use sysfs to control the fan speed.

    https://github.com/Johannes4Linux/Linux_Driver_Tutorial_legacy/blob/main/06_pwm_driver/pwm_driver.c
* 
*/

/* Variables for pwm */
struct pwm_device *pwm0 = NULL;
u32 pwm_period = 40000; // 25 kHz frequency (1/25000 = 0.00004 seconds = 40000 nanoseconds)
u32 pwm_duty_cycle = 20000; // 50% duty cycle

/* Sysfs attributes */
static struct class *lcd_class;
static struct device *lcd_device;
static char fan_speed = '5'; // Default to 50% speed

/* Sysfs show function */
static ssize_t pwm_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%c\n", fan_speed);
}

/* Sysfs store function */
static ssize_t pwm_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32 duty_cycle;
    
    if (count != 1) {
        printk("Invalid input length\n");
        return -EINVAL;
    }

    if (buf[0] >= '0' && buf[0] <= '9') {
        fan_speed = buf[0];
        duty_cycle = (fan_speed - '0') * (pwm_period / 10);
        pwm_config(pwm0, duty_cycle, pwm_period);
        return count;
    }

    printk("Invalid value - Use 0-9 for fan speed control\n");
    return -EINVAL;
}

/* Define sysfs attribute */
static DEVICE_ATTR(pwm_duty_cycle, 0664, pwm_duty_cycle_show, pwm_duty_cycle_store);

/**
 * @brief This function is called when the module is loaded into the kernel
 */
static int __init ModuleInit(void)
{
    int ret;
    printk("PWM Fan Driver - Module Init\n");

    /* Get the existing LCD class */
    lcd_class = class_find_by_name(NULL, "LCD162");
    if (!lcd_class) {
        printk("LCD class not found\n");
        return -ENODEV;
    }

    /* Find the LCD device */
    lcd_device = class_find_device_by_name(lcd_class, "lcd_device");
    if (!lcd_device) {
        printk("LCD device not found\n");
        return -ENODEV;
    }

    /* Create sysfs attribute */
    ret = device_create_file(lcd_device, &dev_attr_pwm_duty_cycle);
    if (ret) {
        printk("Failed to create sysfs attribute\n");
        return ret;
    }

    /* Request PWM0 */
    pwm0 = pwm_request(0, "pwm-fan");
    if (pwm0 == NULL) {
        printk("Could not get PWM0!\n");
        device_remove_file(lcd_device, &dev_attr_pwm_duty_cycle);
        return -ENODEV;
    }

    /* Configure and enable PWM with default speed */
    pwm_config(pwm0, pwm_duty_cycle, pwm_period);
    pwm_enable(pwm0);

    return 0;
}

/**
 * @brief This function is called when the module is removed from the kernel
 */
static void __exit ModuleExit(void)
{
    pwm_disable(pwm0);
    pwm_free(pwm0);
    device_remove_file(lcd_device, &dev_attr_pwm_duty_cycle);
    printk("PWM Fan Driver - Module Exit\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hardik Minocha");
MODULE_DESCRIPTION("A PWM driver for controlling fan speed");