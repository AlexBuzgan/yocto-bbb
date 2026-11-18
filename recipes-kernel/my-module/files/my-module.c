#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define mem_size 1024

dev_t dev = 0;
static struct class *dev_class;
static struct cdev my_cdev;
static uint8_t *kernel_buffer;

// function prototypes
static int		__init my_driver_init(void);
static void		__exit my_driver_exit(void);
static int 		my_open(struct inode *inode, struct file *file);
static int 		my_release(struct inode *inode, struct file *file);
static ssize_t	my_read(struct file *filp, char __user *buf, size_t len, loff_t * off);
static ssize_t	my_write(struct file *filp, const char *buf, size_t len, loff_t * off);

// file operations structure

static struct file_operations fops = 
{
	.owner		= THIS_MODULE,
	.read		= my_read,
	.write		= my_write,
	.open		= my_open,
	.release	= my_release,
};

// called when we open the device file
static int my_open(struct inode *inode, struct file *file)
{
	pr_info("My device file opened...\n");
	return 0;
}

// called when we close the device file
static int my_release(struct inode *inode, struct file *file)
{
	pr_info("My device file closed...\n");
	return 0;
}

// called when we read the device file
static ssize_t	my_read(struct file *filp, char __user *buf, size_t len, loff_t * off)
{
	// copy data from kernel space to user space
	ssize_t ret = simple_read_from_buffer(buf, len, off, kernel_buffer, mem_size);
	
	pr_info("Read done, %zd bytes!!!\n", ret);

	return ret;
}

// called when we write the device file
static ssize_t	my_write(struct file *filp, const char *buf, size_t len, loff_t * off)
{
	// copy data from user space to kernel space
	ssize_t ret = simple_write_to_buffer(kernel_buffer, mem_size, off, buf, len);

	pr_info("Write done, %zd bytes!!!\n", ret);

	return len;
}

// module init function
static int __init my_driver_init(void)
{
	struct device *device;
	int ret;

	// create physical memory
	if((kernel_buffer = kzalloc(mem_size, GFP_KERNEL)) == 0)
	{
		pr_info("Cannot allocate memory in the kernel\n");
		return -ENOMEM;
	}

	// allocating major number
	ret = alloc_chrdev_region(&dev, 0, 1, "my_Dev");
	if(ret < 0)
	{
		pr_info("Cannot allocate major number\n");
		goto r_buf;
	}
	pr_info("Major = %d		Minor = %d \n", MAJOR(dev), MINOR(dev));

	// create cdev structure
	cdev_init(&my_cdev, &fops);

	// add character device to the system
	ret = cdev_add(&my_cdev, dev, 1);
	if(ret < 0)
	{
		pr_info("Cannot add the device to the system\n");
		goto r_region;
	}

	// create struct class
	if(IS_ERR(dev_class = class_create("my_class")))
	{
		pr_info("Cannot create the struct class\n");
		ret = PTR_ERR(dev_class);
		goto r_cdev;
	}

	// create device
	if(IS_ERR(device = device_create(dev_class, NULL, dev, NULL, "my_device")))
	{
		pr_info("Cannot create the device\n");
		ret = PTR_ERR(device);
		goto r_class;
	}

	strscpy(kernel_buffer, "Hello Yocto!", mem_size);
	pr_info("Device driver insert...done!\n");
	return 0;

r_class:
	class_destroy(dev_class);
r_cdev:
	cdev_del(&my_cdev);
r_region:
	unregister_chrdev_region(dev, 1);
r_buf:
	kfree(kernel_buffer);
	return ret;
}

// module exit function
static void __exit my_driver_exit(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	kfree(kernel_buffer);
	pr_info("Device driver remove...done!\n");
}

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Linux Device Driver");
MODULE_VERSION("1.0");
