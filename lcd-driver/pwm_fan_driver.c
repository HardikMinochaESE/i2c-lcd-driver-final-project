#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pwm.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/of.h>

/*
    REFERENCES AND ACKNOWLEDGEMENTS:

    This driver was created by following the tutorial at:
    https://www.youtube.com/watch?v=Zy_z_9-7734

    The tutorial was helpful in understanding how to create a character device driver
    and how to initialize the PWM driver.

    https://github.com/Johannes4Linux/Linux_Driver_Tutorial_legacy/blob/main/06_pwm_driver/pwm_driver.c
*/

/* Variables for pwm */
struct pwm_device *pwm0 = NULL;
u32 pwm_period = 1000000; // 1kHz frequency (1ms period)
u32 pwm_duty_cycle = 500000; // 50% duty cycle

/* Sysfs attributes */
static struct class *custom_pwm_class;
static struct device *pwm_device;
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
    int speed;
    
    if (count == 0) {
        pr_err("PWM Driver: Empty input\n");
        return -EINVAL;
    }

    // Try to convert input to integer first
    if (kstrtoint(buf, 10, &speed) == 0) {
        // Valid integer input
        if (speed >= 0 && speed <= 9) {
            fan_speed = speed + '0';
            duty_cycle = (speed * pwm_period) / 10; // Convert 0-9 to 0-90% duty cycle
            if (pwm0) {
                pwm_config(pwm0, duty_cycle, pwm_period);
            }
            return count;
        }
    }
    // If not a valid integer, try single character
    else if (count == 1 && buf[0] >= '0' && buf[0] <= '9') {
        fan_speed = buf[0];
        duty_cycle = ((fan_speed - '0') * pwm_period) / 10;
        if (pwm0) {
            pwm_config(pwm0, duty_cycle, pwm_period);
        }
        return count;
    }

    pr_err("PWM Driver: Invalid value - Use 0-9 for fan speed control\n");
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

    /* Create PWM class */
    custom_pwm_class = class_create(THIS_MODULE, "custom_pwm");
    if (IS_ERR(custom_pwm_class)) {
        pr_err("PWM Driver: Failed to create sysfs class\n");
        return PTR_ERR(custom_pwm_class);
    }

    /* Create PWM device */
    pwm_device = device_create(custom_pwm_class, NULL, MKDEV(0, 0), NULL, "pwm_fan");
    if (IS_ERR(pwm_device)) {
        pr_err("PWM Driver: Failed to create sysfs device\n");
        class_destroy(custom_pwm_class);
        return PTR_ERR(pwm_device);
    }

    /* Create sysfs attribute */
    ret = device_create_file(pwm_device, &dev_attr_pwm_duty_cycle);
    if (ret) {
        pr_err("PWM Driver: Failed to create sysfs file\n");
        device_destroy(custom_pwm_class, MKDEV(0, 0));
        class_destroy(custom_pwm_class);
        return ret;
    }

    /* Try to get PWM device - try different PWM numbers */
    pwm0 = pwm_request(0, "pwm-fan");
    if (IS_ERR(pwm0)) {
        pr_err("PWM Driver: Failed to get PWM0, trying PWM1\n");
        pwm0 = pwm_request(1, "pwm-fan");
        if (IS_ERR(pwm0)) {
            pr_err("PWM Driver: Failed to get PWM1, trying PWM2\n");
            pwm0 = pwm_request(2, "pwm-fan");
            if (IS_ERR(pwm0)) {
                pr_err("PWM Driver: Failed to get any PWM device. Make sure PWM is enabled in device tree\n");
                device_remove_file(pwm_device, &dev_attr_pwm_duty_cycle);
                device_destroy(custom_pwm_class, MKDEV(0, 0));
                class_destroy(custom_pwm_class);
                return PTR_ERR(pwm0);
            } else {
                printk("PWM Driver: Successfully allocated PWM2\n");
            }
        } else {
            printk("PWM Driver: Successfully allocated PWM1\n");
        }
    } else {
        printk("PWM Driver: Successfully allocated PWM0\n");
    }

    /* Configure and enable PWM with default speed */
    ret = pwm_config(pwm0, pwm_duty_cycle, pwm_period);
    if (ret) {
        pr_err("PWM Driver: Failed to configure PWM\n");
        pwm_free(pwm0);
        device_remove_file(pwm_device, &dev_attr_pwm_duty_cycle);
        device_destroy(custom_pwm_class, MKDEV(0, 0));
        class_destroy(custom_pwm_class);
        return ret;
    }

    ret = pwm_enable(pwm0);
    if (ret) {
        pr_err("PWM Driver: Failed to enable PWM\n");
        pwm_free(pwm0);
        device_remove_file(pwm_device, &dev_attr_pwm_duty_cycle);
        device_destroy(custom_pwm_class, MKDEV(0, 0));
        class_destroy(custom_pwm_class);
        return ret;
    }

    printk("PWM Driver: Successfully initialized PWM device\n");
    return 0;
}

/**
 * @brief This function is called when the module is removed from the kernel
 */
static void __exit ModuleExit(void)
{
    if (pwm0) {
        pwm_disable(pwm0);
        pwm_free(pwm0);
    }
    device_remove_file(pwm_device, &dev_attr_pwm_duty_cycle);
    device_destroy(custom_pwm_class, MKDEV(0, 0));
    class_destroy(custom_pwm_class);
    printk("PWM Fan Driver - Module Exit\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hardik Minocha");
MODULE_DESCRIPTION("A PWM driver for controlling fan speed");