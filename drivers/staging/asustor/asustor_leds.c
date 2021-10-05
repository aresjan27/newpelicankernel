#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/leds.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/libata.h>

#define BLINK_DELAY 30
#define LED_TYPE_GPIO "gpio"

#define ASMEDIA_VENDER 0x1b21
#define ASMEDIA_DEVICE_0612 0x0612

static atomic_t has_platform_device_probed = ATOMIC_INIT(false);
// static DEFINE_SPINLOCK(lock_gpio_nosleep);

struct leds_data
{
    struct device_node *node;

    const char *label;
    const char *type;
    unsigned long mapped_diskid;
    const char *default_trigger;
    long gpio_offset;
};

struct leds_platform_data
{
	int num_leds;
	const struct leds_data *leds;
};

LIST_HEAD(asustor_leds_data_cluster);
struct asustor_leds_data {

    unsigned long cb;
    const char *label;
    const char* type;

    unsigned long mapped_diskid;
    union raw_data {
        struct gpio_led data_gpio;
    } data;

    struct led_classdev cdev;
    struct led_trigger *led_trig;
    struct list_head next;
};

#ifdef ASUSTOR_PATCH
/** Critical: The function will issue soft irq. and NOT sleep in sction of irq. **/
void asustor_ledtrig_disk_activity(void *ata_port)
{
    return;
}
EXPORT_SYMBOL(asustor_ledtrig_disk_activity);
#endif

/** Critical: In leds-triggers, the function will issue soft irq, and NOT sleep in sction of irq. **/
static void asustor_leds_brightness_set(struct led_classdev *led_cdev, enum led_brightness brightness)
{
    struct asustor_leds_data *leds_data = container_of(led_cdev, struct asustor_leds_data, cdev);
    int offset = -1;

    if (!leds_data) return;

    if (!strcmp(leds_data->type, LED_TYPE_GPIO))
    {
        offset = leds_data->data.data_gpio.gpio;
        if (gpio_is_valid(offset))
        {
            if (!gpio_cansleep(offset))
            {
                // spin_lock(&lock_gpio_nosleep);
                gpio_set_value(offset, brightness > 0? 1: 0);
                // spin_unlock(&lock_gpio_nosleep);
            }
        }
    }

    return;
}

__attribute__ ((unused))
static int asustor_leds_blink_set(struct led_classdev *cdev, unsigned long *delay_on, unsigned long *delay_off)
{
    int result = 0;
    return result;
}

__attribute__ ((unused))
static void asustor_led_unregister(struct asustor_leds_data *led_data)
{
	led_classdev_unregister(&led_data->cdev);
}

static int asustor_led_register(const struct leds_data *data_node, struct device *parent)
{
    struct asustor_leds_data *entry = NULL;

    if (!data_node) return -ENOENT;

    entry = devm_kzalloc(parent, sizeof(struct asustor_leds_data), GFP_KERNEL);
    if (!entry) return -ENOMEM;

    entry->cb = sizeof(struct asustor_leds_data);
    entry->label = data_node->label;
    entry->type = data_node->type;

    entry->mapped_diskid = data_node->mapped_diskid;
    if (!strcmp(entry->type, LED_TYPE_GPIO))
    {
        entry->data.data_gpio.name = data_node->label;
        entry->data.data_gpio.default_trigger = data_node->default_trigger;
        entry->data.data_gpio.gpio = data_node->gpio_offset;
    }

    entry->cdev.name = data_node->label;
    entry->cdev.default_trigger = data_node->default_trigger;

    #ifdef CONFIG_LEDS_TRIGGERS
    {
        if (0 != strlen(entry->cdev.default_trigger) && 0 != strcmp(entry->cdev.default_trigger, "none"))
            led_trigger_register_simple(entry->cdev.default_trigger, &entry->led_trig);

        if (entry->led_trig) list_add_tail(&entry->next, &asustor_leds_data_cluster);

    }
    #endif

    entry->cdev.brightness_set = asustor_leds_brightness_set;
    entry->cdev.flags |= LED_BLINK_ONESHOT;
    entry->cdev.max_brightness = LED_FULL;

    return led_classdev_register(parent, &entry->cdev);
}

static int asustor_leds_remove(struct platform_device *pdev)
{
    int result = 0;
    return result;
}

static int check_asmedia_device(void)
{
    return (NULL != pci_get_device(ASMEDIA_VENDER, ASMEDIA_DEVICE_0612, NULL));
}

static struct leds_data* get_next_node_from_dt(struct platform_device *pdev, struct leds_data *prev)
{
    struct device_node *leds = NULL, *leds_cluster = NULL, *led_node = NULL;

    static struct leds_data dt_data;
    struct leds_data *leds_data_gotten = NULL;

    const char *property_str = NULL;
    int result = 0, property_int = 0;

    if (!pdev) return NULL;

    leds = dev_of_node(&pdev->dev);

    leds_cluster = of_get_next_available_child(leds, NULL);
    while (leds_cluster) {

        if (!strcmp(leds_cluster->name, "diskx4_leds") && !check_asmedia_device())
            goto next_cluster;
        else if (!strcmp(leds_cluster->name, "diskx2_leds") && check_asmedia_device())
            goto next_cluster;

        led_node = of_get_next_available_child(leds_cluster, prev? prev->node: NULL);
        if (led_node) {
            leds_data_gotten = &dt_data;
            if (leds_data_gotten)
            {
                leds_data_gotten->node = led_node;
                if (led_node->name) leds_data_gotten->label = led_node->name;
                if (led_node->type) leds_data_gotten->type = led_node->type;
                if (!strcmp(leds_data_gotten->type, LED_TYPE_GPIO))
                {
                    result = of_property_read_u32(led_node, "offset", &property_int);
                    leds_data_gotten->gpio_offset = (!result? property_int: -1);
                }
                result = of_property_read_string(led_node, "default_trigger", &property_str);
                leds_data_gotten->default_trigger = (!result)? property_str: "none";
                result = of_property_read_u32(led_node, "mapped_diskid", &property_int);
                leds_data_gotten->mapped_diskid = (!result)? property_int: -1;
            }
        }
next_cluster:
        leds_cluster = of_get_next_available_child(leds, leds_cluster);
    }

    return leds_data_gotten;
}

static int asustor_leds_probe(struct platform_device *pdev)
{
    struct leds_platform_data *pdata = NULL;
    struct leds_data *leds_data = NULL;
    int result = 0, i;

    if (dev_of_node(&pdev->dev))
    {
        leds_data = get_next_node_from_dt(pdev, NULL);
        while (leds_data)
        {
            result = asustor_led_register((const struct leds_data *)leds_data, &pdev->dev);
            if (result) goto err;

            leds_data = get_next_node_from_dt(pdev, leds_data);
        }
        atomic_set(&has_platform_device_probed, true);
    }
    else if (NULL != (pdata = dev_get_platdata(&pdev->dev)))
    {
        for (i = 0; i < pdata->num_leds; i++)
        {
            result = asustor_led_register(&pdata->leds[i], &pdev->dev);
            if (result) goto err;
        }
        atomic_set(&has_platform_device_probed, true);
    }

    return 0;

err:
    return result;
}

#ifdef CONFIG_OF
static struct of_device_id asustor_leds_match_table[] =
{
    { .name = "leds", .compatible = "asustor,as33-leds", },
    { .name = "leds", .compatible = "asustor,as11-leds", },
	{},
};
MODULE_DEVICE_TABLE(of, asustor_leds_match_table);
#else
#define asustor_leds_match_table NULL
#endif // CONFIG_OF

static struct platform_driver asustor_leds_driver = {
	.probe		= asustor_leds_probe,
	.remove		= asustor_leds_remove,
	.driver		= {
        .name = "asustor_leds_driver",
        .owner = THIS_MODULE,
        .of_match_table = asustor_leds_match_table,
    },
};
module_platform_driver(asustor_leds_driver);

__attribute__ ((unused))
static void __exit asustor_leds_exit(void)
{
	platform_driver_unregister(&asustor_leds_driver);
	return;
}

__attribute__ ((unused))
static int __init asustor_leds_init(void)
{
    return platform_driver_register(&asustor_leds_driver);
}


