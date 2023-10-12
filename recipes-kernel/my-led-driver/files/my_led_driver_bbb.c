#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>

static struct kobject *led_kobj;

#define NUM_LEDS 4

struct led_attr {
    int led_state;
    struct kobj_attribute attr;
    struct gpio_desc *led;
};

struct led_node {
    struct fwnode_handle *fwnode_btn;
    const char *name;
    const char *label;
};

static ssize_t led_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/* Write current LED state to user buffer */
    struct led_attr *led_attribute = container_of(attr, struct led_attr, attr);
    return sprintf(buf, "%d\n", led_attribute->led_state);
}

static ssize_t led_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct led_attr *led_attribute = container_of(attr, struct led_attr, attr);

    sscanf(buf, "%du", &led_attribute->led_state);
    if (led_attribute->led_state == 1 || led_attribute->led_state == 0) {
		gpiod_set_value(led_attribute->led, led_attribute->led_state);
	}

    return count;
}

/* Create the file with name, "led0_state", specify read and write callbacks */
static struct led_attr led_attrs[NUM_LEDS] = {
    [0] = { .attr = __ATTR(led0_state, 0660, led_read, led_write) },
    [1] = { .attr = __ATTR(led1_state, 0660, led_read, led_write) },
    [2] = { .attr = __ATTR(led2_state, 0660, led_read, led_write) },
    [3] = { .attr = __ATTR(led3_state, 0660, led_read, led_write) }
};

static int my_module_probe(struct platform_device *pdev)
{
	int err, i;
    char name[5] = {0};

    struct led_node led_nodes[NUM_LEDS];
	struct device *dev = &pdev->dev;

    /* Create sysfs entry */
    led_kobj = kobject_create_and_add("my_led_sysfs", NULL);
    if(!led_kobj) {
        printk("Failed to create sysfs entry\n");
        return -ENOMEM;
    }

    /* Fetch user LED info from am335x-bone-common.dtsi */
    for (i=0; i < NUM_LEDS; i++) {
        sprintf(name, "led%d", i+2);
        printk("Getting node named %s\n", name);
        led_nodes[i].fwnode_btn = device_get_named_child_node(dev, name);
        if (!led_nodes[i].fwnode_btn) {
            printk("Failed to get node named %s\n", name);
            return -ENODEV;
        }

        /* Fetch label */
        err = fwnode_property_read_string(led_nodes[i].fwnode_btn, "label", &led_nodes[i].label);
        if (err) {
            printk("Failed to read label property\n");
            return -ENODATA;
        }
        printk("Fetched LED label %s\n", led_nodes[i].label);

        /* Set up LEDs as GPIO output devices */
	    led_attrs[i].led = devm_fwnode_gpiod_get(dev, led_nodes[i].fwnode_btn, NULL, GPIOD_OUT_HIGH, led_nodes[i].label);
        if(IS_ERR(led_attrs[i].led)) {
            printk("Error, could not set up GPIO %s\n", led_nodes[i].label);
            return -EIO;
        }
        printk("LED %s set up as GPIO device\n", name);

        led_attrs[i].led_state = 1;
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
        printk("Turn off LED %d, remove sysfs\n", i);
        gpiod_set_value(led_attrs[i].led, 0);
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

// static int __init my_module_init(void)
// {
// 	printk("Hello World, from BBB!!\n");
// 	platform_driver_register(&my_led_driver);
// 	return 0;
// }

// static void __exit my_module_exit(void)
// {
// 	printk("Goodbye Cruel World! BBB out!\n");
// 	platform_driver_unregister(&my_led_driver);
// }

module_platform_driver(my_led_driver);
// module_init(my_module_init);
// module_exit(my_module_exit);
MODULE_LICENSE("GPL");
