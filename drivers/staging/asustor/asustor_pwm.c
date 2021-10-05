#include <linux/syscalls.h> /* SYSCALL_DEFINEx */
#include <linux/pwm.h>

enum asustor_pwmctl_cmd_type {
    PWMCTL_GET_STATE = 0,
    PWMCTL_SET_STATE,
    PWMCTL_ENABLE,
    PWMCTL_DISABLE,
    PWMCTL_ADJUST,
    PWMCTL_MAX_CMD,
};

struct asustor_pwmctl_args
{
    const char* label;
    unsigned int pwm;
    struct pwm_state st;
};

static int asustor_pwm_ctrl(unsigned int cmd, unsigned long args)
{
    int result = 0;
    struct pwm_device *dev = NULL;
    struct pwm_state st;
    struct asustor_pwmctl_args __user *argp = (struct asustor_pwmctl_args __user *)args;

    if (NULL == argp) return -EINVAL;

    dev = pwm_request(argp->pwm, argp->label);
    if (NULL != dev)
    {
        switch ((enum asustor_pwmctl_cmd_type)cmd)
        {
        case PWMCTL_GET_STATE:
            pwm_get_state(dev, &st);
            if (copy_to_user(&(argp->st), &st, sizeof(struct pwm_state))) {
                result = -ENOMEM; }
            break;
        case PWMCTL_SET_STATE:
            if (copy_from_user(&st, &(argp->st), sizeof(struct pwm_state))) {
                result = -ENOMEM; break; }
            result = pwm_apply_state(dev, &st);
            break;
        case PWMCTL_ENABLE:
            if (dev->chip->ops->enable)
                result = dev->chip->ops->enable(dev->chip, dev);
            break;
        case PWMCTL_DISABLE:
            if (dev->chip->ops->disable)
                dev->chip->ops->disable(dev->chip, dev);
            break;
        case PWMCTL_ADJUST:
            result = pwm_adjust_config(dev);
            break;
        default:
            result = -EINVAL;
        }
        pwm_free(dev);
    }

    return result;
}

SYSCALL_DEFINE2(asustor_pwmctl, unsigned int, cmd, unsigned long, args)
{
    return asustor_pwm_ctrl(cmd, args);
}