#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>

// static struct gpio_desc *led0 = NULL;
// static struct gpio_desc *led1 = NULL;
// static struct gpio_desc *led2 = NULL;
// static struct gpio_desc *led3 = NULL;
static struct kobject *led_kobj;

#define NUM_LEDS 4

struct led_attr {
    int led_state;
    struct kobj_attribute attr;
    struct gpio_desc *led;
};

static ssize_t led_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/* Write current LED state to user buffer */
	// return sprintf(buf, "%d\n", led0_state);

    struct led_attr *led_attribute = container_of(attr, struct led_attr, attr);
    return sprintf(buf, "%d\n", led_attribute->led_state);
}

static ssize_t led_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    // Read input and update LED state, user space can update LED state
	// sscanf(buf, "%du", &led0_state);
	// if (led0_state == 1 || led0_state == 0) {
	// 	gpiod_set_value(led0, led0_state);
	// }
	// return count;

    struct led_attr *led_attribute = container_of(attr, struct led_attr, attr);

    sscanf(buf, "%du", &led_attribute->led_state);
    if (led_attribute->led_state == 1 || led_attribute->led_state == 0) {
		gpiod_set_value(led_attribute->led, led_attribute->led_state);
	}

    return count;
}

/* Create the file with name, "led0_state", specify read and write callbacks */
// static struct kobj_attribute led0_state_attr = __ATTR(led0_state, 0660, my_module_show, my_module_store);
// static struct kobj_attribute led2_state_attr = __ATTR(led1_state, 0660, my_module_show, my_module_store);
// static struct kobj_attribute led3_state_attr = __ATTR(led2_state, 0660, my_module_show, my_module_store);
// static struct kobj_attribute led4_state_attr = __ATTR(led3_state, 0660, my_module_show, my_module_store);
static struct led_attr led_attrs[NUM_LEDS] = {
    [0] = { .attr = __ATTR(led0_state, 0660, led_read, led_write) },
    [1] = { .attr = __ATTR(led1_state, 0660, led_read, led_write) },
    [2] = { .attr = __ATTR(led2_state, 0660, led_read, led_write) },
    [3] = { .attr = __ATTR(led3_state, 0660, led_read, led_write) }
};


static int my_module_probe(struct platform_device *pdev)
{
	int err, i;
    const char *label0;
    const char *label1;
    const char *label2;
    const char *label3;

	struct device *dev = &pdev->dev;

	// Fetch all user LEDs
	struct fwnode_handle *fwnode_btn0 = device_get_named_child_node(dev, "led2");
	struct fwnode_handle *fwnode_btn1 = device_get_named_child_node(dev, "led3");
	struct fwnode_handle *fwnode_btn2 = device_get_named_child_node(dev, "led4");
	struct fwnode_handle *fwnode_btn3 = device_get_named_child_node(dev, "led5");

	printk("LED: Running device probe\n");

	// Fetch labels from am335x-bone-common.dtsi
	err = fwnode_property_read_string(fwnode_btn0, "label", &label0);
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }

	err = fwnode_property_read_string(fwnode_btn1, "label", &label1);
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }

	err = fwnode_property_read_string(fwnode_btn2, "label", &label2);
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }

	err = fwnode_property_read_string(fwnode_btn3, "label", &label3);
    if (err) {
        printk("Failed to read label property\n");
        return -ENODATA;
    }
    printk("Labels are: %s, %s, %s, %s\n", label0, label1, label2, label3);

    // Set up LEDs as GPIO output devices
	led_attrs[0].led = devm_fwnode_gpiod_get(dev, fwnode_btn0, NULL, GPIOD_OUT_HIGH, label0);
	led_attrs[1].led = devm_fwnode_gpiod_get(dev, fwnode_btn1, NULL, GPIOD_OUT_HIGH, label1);
	led_attrs[2].led = devm_fwnode_gpiod_get(dev, fwnode_btn2, NULL, GPIOD_OUT_HIGH, label2);
	led_attrs[3].led = devm_fwnode_gpiod_get(dev, fwnode_btn3, NULL, GPIOD_OUT_HIGH, label3);
	if(IS_ERR(led_attrs[0].led) ||
        IS_ERR(led_attrs[1].led) ||
        IS_ERR(led_attrs[2].led) ||
        IS_ERR(led_attrs[3].led)) {

        printk("Error, could not set up GPIOs\n");
		return -EIO;
	}
	printk("LEDs set up as GPIO devices\n");

	/* store current LEDs state to HIGH */
	// led0_state = 1;
	// led1_state = 1;
	// led2_state = 1;
	// led3_state = 1;

    // Create sysfs entry
    led_kobj = kobject_create_and_add("my_led_sysfs", NULL);
    if(!led_kobj) {
        printk("Failed to create sysfs entry\n");
        return -ENOMEM;
    }
    // Add attributes, LEDs being in off state
    for (i = 0; i < NUM_LEDS; i++) {
        led_attrs[i].led_state = 0;
        if (sysfs_create_file(led_kobj, &led_attrs[i].attr.attr)) {
            printk(KERN_ERR "Failed to create led%d_state in /sys/kernel/led\n", i);
        }
    }

    return 0;
}

static int my_module_remove(struct platform_device *pdev)
{
    int i;

    printk("LED: Running device remove\n");

    // Turn off LED
    for (i = 0; i < NUM_LEDS; i++) {
        gpiod_set_value(led_attrs[i].led, 0);
        led_attrs[i].led_state = 0;

        sysfs_remove_file(led_kobj, &led_attrs[i].attr.attr);
    }

    kobject_put(led_kobj);
    return 0;
}

static struct of_device_id my_driver_of_match[] = {
	{ .compatible = "my-gpio-leds", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_driver_of_match);

static struct platform_driver my_led_driver = {
	.probe = my_module_probe,
	.remove = my_module_remove,
	.driver = {
		.name = "my_led_driver",
		.of_match_table = my_driver_of_match,
	}
};

static int __init my_module_init(void)
{
	printk("Hello World, from BBB!!\n");
	platform_driver_register(&my_led_driver);
	return 0;
}

static void __exit my_module_exit(void)
{
	printk("Goodbye Cruel World! BBB out!\n");
	platform_driver_unregister(&my_led_driver);
}

// module_platform_driver(my_led_driver);
module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");
