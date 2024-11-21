#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

/* Meta Information  */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kacper Powolny & Adam Wiktor");
MODULE_DESCRIPTION("I lost my humanity and sanity on that laboratory...");

/* Default interval in seconds for CPU usage checks */
static struct timer_list cpu_usage_timer;
static int interval = 10;
module_param(interval, int, 0644);
MODULE_PARM_DESC(interval, "Interval in seconds between CPU usage checks");

/* Variables to store the previous values of total and indle CPU time  */
static unsigned long prev_total = 0;
static unsigned long prev_idle = 0;

/* Function to calculate CPU usage  */
static void calculate_cpu_usage(struct timer_list *timer) {
    struct file *file;
    char buf[128];
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    unsigned long total, active;
    unsigned long cpu_usage;
    loff_t pos = 0;
    int ret;

    /* Opening the /proc/stat file which contains CPU usage information  */
    file = filp_open("/proc/stat", O_RDONLY, 0);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Failed to open /proc/stat, error: %ld\n", PTR_ERR(file));
        return;
    }

    /* Reading the contents of /proc/stat  */
    ret = kernel_read(file, buf, sizeof(buf) - 1, &pos);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read /proc/stat, error: %d\n", ret);
        filp_close(file, NULL);
        return;
    }

    /* Closing the /proc/stat file  */
    filp_close(file, NULL);

    /* Parsing the data from /proc/stat  */
    if (sscanf(buf, "cpu  %lu %lu %lu %lu %lu %lu %lu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 7) {

        total = user + nice + system + idle + iowait + irq + softirq;
        active = user + nice + system;
	
	/* In case we habe previous values for total and idle, we calculate  */
        if (prev_total != 0) {
            unsigned long delta_total = total - prev_total;
            unsigned long delta_idle = idle - prev_idle;

	/* In case total CPU has changed, we calculate new usage  */
            if (delta_total > 0) {
                cpu_usage = 100 * (delta_total - delta_idle) / delta_total;
                printk(KERN_INFO "CPU Usage: %lu%% [Total: %lu, Active: %lu, Idle: %lu]\n", cpu_usage, total, active, idle);
            } else {
                printk(KERN_INFO "CPU Usage: Unable to calculate, no data.\n");
            }
        }
	
	/* Saving current total and idle values for next iterations  */
        prev_total = total;
        prev_idle = idle;
    } else {
        printk(KERN_ERR "Failed to parse /proc/stat\n");
    }
	
    /* Triggering the timer again after specified interval  */
    mod_timer(&cpu_usage_timer, jiffies + interval * HZ);
}

/* Module initialization function  */
static int __init cpu_usage_init(void) {
    printk(KERN_INFO "CPU Monitor Kernel Module Loaded\n");

    /* Setting the timer to periodically trigger the CPU Usage calculation function  */
    timer_setup(&cpu_usage_timer, calculate_cpu_usage, 0);
    mod_timer(&cpu_usage_timer, jiffies + interval * HZ);

    return 0;
}

/* Module exit function */
static void __exit cpu_usage_exit(void) {
    printk(KERN_INFO "CPU Monitor Kernel Module Unloaded\n");

    del_timer(&cpu_usage_timer);
}

module_init(cpu_usage_init);
module_exit(cpu_usage_exit);
