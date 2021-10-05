#include <linux/syscalls.h> /* SYSCALL_DEFINEx */
#include <linux/gpio.h>

enum asustor_gpioctl_cmd_type {
    GPIOCTL_DIRECTION_INPUT = 0,
    GPIOCTL_DIRECTION_OUTPUT,
    GPIOCTL_GET_VALUE,
    GPIOCTL_SET_VALUE,
    GPIOCTL_MAX_CMD,
};

struct asustor_gpioctl_args
{
    const char *label;
    unsigned offset;
    int value;
};

static int asustor_gpio_ctrl(unsigned int cmd, unsigned long args)
{
    int result = 0;
    struct asustor_gpioctl_args __user *argp = (struct asustor_gpioctl_args __user *)args;

    if (NULL == argp) return -EINVAL;

    if(gpio_is_valid(argp->offset))
    {
        result = gpio_request(argp->offset, argp->label);
        if (0 == result)
        {
            switch ((enum asustor_gpioctl_cmd_type)cmd)
            {
            case GPIOCTL_DIRECTION_INPUT:
                result = gpio_direction_input(argp->offset);
                break;
            case GPIOCTL_DIRECTION_OUTPUT:
                result = gpio_direction_output(argp->offset, argp->value);
                break;
            case GPIOCTL_GET_VALUE:
                result = gpio_get_value(argp->offset);
                break;
            case GPIOCTL_SET_VALUE:
                gpio_set_value(argp->offset, argp->value);
                result = 0;
                break;
            default:
                result = -EINVAL;
            }
            gpio_free(argp->offset);
        }
    }
    return result;
}

SYSCALL_DEFINE2(asustor_gpioctl, unsigned int, cmd, unsigned long, args)
{
    return asustor_gpio_ctrl(cmd, args);
}


