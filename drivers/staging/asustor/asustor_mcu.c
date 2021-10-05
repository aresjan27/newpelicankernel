#include <linux/init.h>
#include <linux/pm.h>
#include <linux/serial.h>
#include <linux/termios.h>
#include <linux/poll.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/reboot.h>

#define SERIAL_PORT_PATTERN "/dev/ttyS%d"

#define MCU_NAME "[MCU::PIC16F1829]"
#define MCU_TRY_PROBE_COUNT 45
#define MCU_TRY_PROBE_PERIOD 1500 // millisecond
#define MCU_INITIAL_DELAY 3*1000 // millisecond
#define MCU_PORT 1
// #define MCU_VERSION_MAJOR 1
// #define MCU_VERSION_MINOR 11
#define MCU_COMMAND_SIZE 3
#define MCU_RECIVE_SIZE 3

#define MCU_COMMAND_GET_VERSION 0x410000 // 0x41, 0x00, 0x00
#define MCU_COMMAND_POWER_OFF 0x100101 // 0x10, 0x01, 0x01
#define MCU_COMMAND_RESTART 0x100102 // 0x10, 0x01, 0x02
#define MCU_COMMAND_GET_POWER_STATE 0x110000 // 0x11, 0x00, 0x00

extern void (*asustor_pm_restart)(enum reboot_mode reboot_mode, const char *cmd);

struct mcu_data
{
    unsigned char main_class;
    unsigned char sub_class;
    unsigned char data;
};

static void serial_poll_wait(struct file* f, int timeout)
{
    struct poll_wqueues table;
	ktime_t start, now;

	start = ktime_get();
	poll_initwait(&table);
	while (1) {
		long elapsed;
		int mask;

		mask = f->f_op->poll(f, &table.pt);
		if (mask & (POLLRDNORM | POLLRDBAND | POLLIN |
			    POLLHUP | POLLERR)) {
			break;
		}
		now = ktime_get();
		elapsed = ktime_us_delta(now, start);
		if (elapsed > timeout)
			break;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(((timeout - elapsed) * HZ) / 10000);
	}
	poll_freewait(&table);
}

static int serial_recv(struct file* f, void* buff, int buff_size, int timeout)
{
	int result = 0, retries = 0, length = 0;
    unsigned char ch = 0;
    loff_t offset = 0;
    mm_segment_t oldfs;

    if (!IS_ERR(f) && f && NULL != buff)
    {
        oldfs = get_fs();
		set_fs(KERNEL_DS);
        while (1)
        {
            buff_size--;
            if (f->f_op->poll)
            {
                serial_poll_wait(f, timeout);
                if (1 == __vfs_read(f, (__force char __user *)&ch, 1, &offset))
                {
                    *((unsigned char*)(buff + length)) = ch;
                    length++;
                }
            }
            else
            {
                while (1)
                {
                    retries++;
                    if (retries >= timeout)
                        break;

                    if (1 == __vfs_read(f, (__force char __user *)&ch, 1, &offset))
                    {
                        *((unsigned char*)(buff + length)) = ch;
                        length++;
                        break;
                    }
                    usleep_range(100, 1000);
                }
            }

            if (0 >= buff_size)
                break;
        }
        set_fs(oldfs);
        result = length;
    }
    return result;
}

static int serial_send(struct file* f, void* data, int count)
{
    int result = 0;
    mm_segment_t oldfs;
    loff_t offset = 0;

    if (!IS_ERR(f) && f)
    {
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        result = __vfs_write(f, (__force const char __user *)data, count, &offset);
        set_fs(oldfs);
    }
    return result;
}

static void serial_close(struct file* f)
{
    if (!IS_ERR(f) && f) filp_close(f, NULL);
}

static int serial_open(struct file** f, int port)
{
    int result = 0;
    char serial_port[20];

    sprintf(serial_port, SERIAL_PORT_PATTERN, port);
    *f = filp_open(serial_port, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
    if (IS_ERR(*f)) {
        result = (int)PTR_ERR(*f);
    }
    return result;
}

#if 0
void __asustor_mcu_restart(void)
{
	int bNormalReboot = false;
	mm_segment_t oldfs;
	struct file  *pfTerm = NULL, *pfFile = NULL;
	unsigned char ryszReboot[3], ryszMcuReboot[7];

	ryszReboot[0] = 0x10;
	ryszReboot[1] = 0x01;
	ryszReboot[2] = 0x02;
	ryszMcuReboot[0] = 0xDE;
	ryszMcuReboot[1] = 0x01;
	ryszMcuReboot[2] = 0x00;
	ryszMcuReboot[3] = 0x00;
	ryszMcuReboot[4] = 0x06;
	ryszMcuReboot[5] = 0x00;
	ryszMcuReboot[6] = ryszMcuReboot[0] + ryszMcuReboot[1] + ryszMcuReboot[2] + ryszMcuReboot[3] + ryszMcuReboot[4] + ryszMcuReboot[5];
	// check if normal reboot or mcu update reboot
	pfFile = filp_open("/etc/.mcu.hex", O_RDONLY, 0);
	if(IS_ERR(pfFile))
		bNormalReboot = true;

	pfTerm = filp_open("/dev/ttyS1", O_RDWR | O_NOCf | O_NONBLOCK, 0);

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	pfTerm->f_pos=0;
	if (bNormalReboot)
	{
		printk("mcu reboot\n");
		pfTerm->f_op->write(pfTerm,ryszReboot, sizeof(ryszReboot), &pfTerm->f_pos);
		mdelay(10);
		// mcu ack
		pfTerm->f_op->read(pfTerm, ryszReboot, sizeof(ryszReboot), &(pfTerm->f_pos));
	}
	else
	{
		printk("mcu update reboot\n");
		pfTerm->f_op->write(pfTerm, ryszMcuReboot, sizeof(ryszMcuReboot), &pfTerm->f_pos);
		mdelay(2);
		// mcu ack
		pfTerm->f_op->read(pfTerm, ryszMcuReboot, sizeof(ryszMcuReboot), &(pfTerm->f_pos));
	}
	set_fs(oldfs);

	mdelay(1000);

}
EXPORT_SYMBOL_GPL(as_mcu_restart);
#endif


static int asustor_mcu_send_command(u32 send_commnad, struct mcu_data* recv_data)
{
    int result = 0;
    struct file *tty;

    struct mcu_data send_data, ack_data;

    send_data.main_class = (unsigned char)((send_commnad >> 16) & 0xff);
    send_data.sub_class = (unsigned char)((send_commnad >> 8) & 0xff);
    send_data.data = (unsigned char)(send_commnad & 0xff);

    result = serial_open(&tty, MCU_PORT);
    if (0 == result)
    {
        result = serial_send(tty, (void*)&send_data, MCU_COMMAND_SIZE);
        if (MCU_COMMAND_SIZE == result)
        {
            // mdelay(10);
            result = serial_recv(tty, (void*)&ack_data, MCU_COMMAND_SIZE, 1000);
            if (MCU_COMMAND_SIZE == result)
            {
                if ((send_data.main_class & 0xF0) + 0x0A == ack_data.main_class)
                    result = 0;
            }

            if (NULL != recv_data && 1 == (send_data.main_class & 0x0f) && 0 == result)
            {
                // mdelay(10);
                result = serial_recv(tty, recv_data, MCU_COMMAND_SIZE, 1000);
                if (result == MCU_COMMAND_SIZE)
                    result = 0;
            }
        }
        serial_close(tty);
    }

    return result;
}

#if 0
static void asustor_mcu_power_resume(unsigned long is_enable)
{
    struct mcu_data recv_data;
    u32 send_commnad;

    if (!asustor_mcu_send_command(MCU_COMMAND_GET_POWER_STATE, &recv_data))
    {
        send_commnad = 0x100000 + (recv_data.data & 0x3F) + ((is_enable)? 0x2 << 6: 0x0 << 6);
        asustor_mcu_send_command(send_commnad, NULL);
    }
}
#endif

static void asustor_mcu_power_off(void)
{
    printk(KERN_INFO "%s Power off\n", MCU_NAME);
    // asustor_mcu_power_resume(false);
    mdelay(10);
    asustor_mcu_send_command(MCU_COMMAND_POWER_OFF, NULL);
}

static void asustor_mcu_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	printk("%s Restart\n", MCU_NAME);
    asustor_mcu_send_command(MCU_COMMAND_RESTART, NULL);
}

static void asustor_mcu_register(void)
{
    printk(KERN_INFO "%s device has found\n", MCU_NAME);

    pm_power_off = asustor_mcu_power_off;
    asustor_pm_restart = asustor_mcu_restart;
    // asustor_mcu_power_resume(true);

    return;
}

static struct task_struct *probe_tsk;

static int asustor_mcu_probe(void)
{
    struct mcu_data recv_data;

    if (0 != asustor_mcu_send_command(MCU_COMMAND_GET_VERSION, &recv_data))
        return -ENOENT;

    return 0;
}

static int probe_mcu_worker(void *data)
{
    int retry = 0;

    msleep_interruptible(MCU_INITIAL_DELAY);

    while (!kthread_should_stop())
    {
        if (MCU_TRY_PROBE_COUNT <= retry)
            kthread_stop(probe_tsk);

        if (0 == asustor_mcu_probe())
        {
            asustor_mcu_register();

            kthread_stop(probe_tsk);
        }

        msleep_interruptible(MCU_TRY_PROBE_PERIOD);
        retry++;
    }
    return 0;
}

static int __init asustor_mcu_init(void)
{
    probe_tsk = kthread_run(probe_mcu_worker, NULL, "probe_mcu_worker");
    return 0;
}
late_initcall(asustor_mcu_init);

