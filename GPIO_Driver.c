#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>   // For copy_from_user, copy_to_user
#include <asm/io.h>          // For ioremap

#define UART_MAX_USER_SIZE 256  // Max buffer size for user-space input

// Define UART memory addresses based on Raspberry Pi model
#define BCM2837_UART_ADDRESS  0x3F201000  // UART base address for RPi 3
#define BCM2711_UART_ADDRESS  0xFE201000  // UART base address for RPi 4

static struct proc_dir_entry *uart_proc = NULL;  // Entry in /proc filesystem

static char uart_buffer[UART_MAX_USER_SIZE+1] = {0}; // Buffer to store input data

static volatile unsigned int *uart_registers = NULL; // Pointer to UART registers

// UART Register Offsets
#define UART_DR   0x00  // Data Register (Read/Write)
#define UART_FR   0x18  // Flag Register (Check TX/RX status)
#define UART_IBRD 0x24  // Integer Baud Rate Divisor
#define UART_FBRD 0x28  // Fractional Baud Rate Divisor
#define UART_LCRH 0x2C  // Line Control Register
#define UART_CR   0x30  // Control Register
#define UART_IMSC 0x38  // Interrupt Mask Set/Clear Register

// Function to send a character over UART
static void uart_send_char(char c)
{
    while (*(uart_registers + (UART_FR / 4)) & (1 << 5));  // Wait until TX buffer is not full
    *(uart_registers + (UART_DR / 4)) = c;  // Write character to Data Register
}

// Function to receive a character over UART
static char uart_receive_char(void)
{
    while (*(uart_registers + (UART_FR / 4)) & (1 << 4));  // Wait until RX buffer has data
    return *(uart_registers + (UART_DR / 4));  // Read received character
}

// Function to write data to UART via /proc file
static ssize_t uart_write(struct file *file, const char __user *user, size_t size, loff_t *off)
{
    if (size > UART_MAX_USER_SIZE) size = UART_MAX_USER_SIZE;

    memset(uart_buffer, 0x0, sizeof(uart_buffer));

    if (copy_from_user(uart_buffer, user, size))
        return -EFAULT;

    printk("UART Write: %s\n", uart_buffer);

    // Send each character to UART
    for (size_t i = 0; i < size; i++)
    {
        uart_send_char(uart_buffer[i]);
    }

    return size;
}

// Function to read data from UART via /proc file
static ssize_t uart_read(struct file *file, char __user *user, size_t size, loff_t *off)
{
    char recv_buffer[2] = {0}; // Buffer to store received character
    recv_buffer[0] = uart_receive_char(); // Read one character

    if (copy_to_user(user, recv_buffer, 1))
        return -EFAULT;

    return 1; // Return number of bytes read
}

// Define file operations for /proc interface
static const struct proc_ops uart_proc_fops = {
    .proc_read = uart_read,
    .proc_write = uart_write,
};

// Module initialization function
static int __init uart_driver_init(void)
{
    printk("UART Driver: Initializing...\n");

    // Map UART memory region
    uart_registers = (unsigned int *)ioremap(BCM2837_UART_ADDRESS, PAGE_SIZE);
    if (!uart_registers)
    {
        printk("UART Driver: Failed to map UART memory\n");
        return -ENOMEM;
    }

    printk("UART Driver: Successfully mapped UART memory\n");

    // Create a /proc entry
    uart_proc = proc_create("lll-uart", 0666, NULL, &uart_proc_fops);
    if (!uart_proc)
    {
        iounmap(uart_registers);
        printk("UART Driver: Failed to create /proc entry\n");
        return -ENOMEM;
    }

    printk("UART Driver: /proc/lll-uart created successfully\n");

    return 0;
}

// Module exit function
static void __exit uart_driver_exit(void)
{
    printk("UART Driver: Exiting...\n");

    proc_remove(uart_proc); // Remove /proc entry
    iounmap(uart_registers); // Unmap memory

    printk("UART Driver: Successfully removed\n");
}

module_init(uart_driver_init);
module_exit(uart_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Low Level Learning");
MODULE_DESCRIPTION("UART driver for Raspberry Pi");
MODULE_VERSION("1.0");
