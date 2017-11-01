/*
 * veml7700.c - Support for Vishay veml7700 combined ambient light and
 * proximity sensor
 *
 * Copyright 2012 Peter Meerwald <pmeerw@pmeerw.net>
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * IIO driver for veml7700 (7-bit I2C slave address 0x10)
 *
 * TODO:
 *   allow to adjust IR current
 *   proximity threshold and event handling
 */

#include "chardev.h"
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define veml7700_DRV_NAME "veml7700"
#define DEBUG 1
#define SUCCESS 0

#define COMMAND_ALS_SM      0x00   /**/ 
#define COMMAND_ALS_WH      0x01   /**/ 
#define COMMAND_ALS_WL      0x02   /**/ 
#define COMMAND_PSM        	0x03   /**/ 
#define COMMAND_PSM_EN      0x03   /**/ 
#define COMMAND_ALS        	0x04   /**/ 
#define COMMAND_WHITE      	0x05   /**/ 
#define COMMAND_ALS_IF_L    0x06   /**/ 
#define COMMAND_ALS_IF_H    0x06   /**/ 


#define SUCCESS 0

static const struct i2c_device_id veml7700_id[] = {
	{ "veml7700", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, veml7700_id);

static int veml7700_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{

#ifdef DEBUG
	printk(KERN_DEBUG "VEML7700 ######################################### PROBE\n");
	dev_info(&client->dev, "Probing VEML7700");


	int e = i2c_smbus_read_byte_data(client, COMMAND_ALS);
	printk(KERN_DEBUG "VEML7700 ######################################### VAL: %d\n",e);

	e = i2c_smbus_read_byte_data(client, COMMAND_WHITE);
	printk(KERN_DEBUG "VEML7700 ######################################### VAL: %d\n",e);
#endif

	return 0;
}

static int veml7700_remove(struct i2c_client *client)
{
	return 0;
}

/* 
 * This is called whenever a process attempts to open the device file 
 */
static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "device_open(%p)\n", file);
#endif

	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif

	module_put(THIS_MODULE);
	return 0;
}

/* 
 * This function is called when somebody tries to
 * write into our device file. 
 */
static ssize_t device_write(struct file *file,
	     const char __user * buffer, size_t length, loff_t * offset)
{
	int i = 0;

#ifdef DEBUG
	printk(KERN_INFO "device_write(%p,%s,%d)", file, buffer, length);
#endif

	/* 
	 * Again, return the number of input characters used 
	 */
	return i;
}

/* 
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file,	/* see include/linux/fs.h   */
			   char __user * buffer,	/* buffer to be
							 * filled with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	/* 
	 * Number of bytes actually written to the buffer 
	 */
	int bytes_read = 99;
	return bytes_read;
}

static int veml7700_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	printk(KERN_DEBUG "VEML7700 ######################################### READ RAW\n");
	return 0;
}

/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
int device_ioctl(struct inode *inode,	/* see include/linux/fs.h */
		 struct file *file,	/* ditto */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)
{
	int i;
	char *temp;
	char ch;

	/* 
	 * Switch according to the ioctl called 
	 */
	switch (ioctl_num) {
	case IOCTL_SET_MSG:
		/* 
		 * Receive a pointer to a message (in user space) and set that
		 * to be the device's message.  Get the parameter given to 
		 * ioctl by the process. 
		 */
		temp = (char *)ioctl_param;

		/* 
		 * Find the length of the message 
		 */
		get_user(ch, temp);
		for (i = 0; ch && i < BUF_LEN; i++, temp++)
			get_user(ch, temp);

		device_write(file, (char *)ioctl_param, i, 0);
		break;

	case IOCTL_GET_MSG:
		/* 
		 * Give the current message to the calling process - 
		 * the parameter we got is a pointer, fill it. 
		 */
		i = device_read(file, (char *)ioctl_param, 99, 0);

		/* 
		 * Put a zero at the end of the buffer, so it will be 
		 * properly terminated 
		 */
		put_user('\0', (char *)ioctl_param + i);
		break;

	case IOCTL_GET_NTH_BYTE:
		/* 
		 * This ioctl is both input (ioctl_param) and 
		 * output (the return value of this function) 
		 */
		return Message[ioctl_param];
		break;
	}

	return SUCCESS;
}

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. 
 */
struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,	/* a.k.a. close */
};

static const struct iio_info veml7700_info = {
	.read_raw = veml7700_read_raw,
	.driver_module = THIS_MODULE,
};

static struct i2c_driver veml7700_driver = {
	.driver = {
		.name   = veml7700_DRV_NAME,
		.owner  = THIS_MODULE,
	},
	.probe  = veml7700_probe,
	.remove = veml7700_remove,
	.id_table = veml7700_id,
};
module_i2c_driver(veml7700_driver);

MODULE_AUTHOR("AK");
MODULE_DESCRIPTION("Vishay veml7700 ambient light sensor driver");
MODULE_LICENSE("GPL");
