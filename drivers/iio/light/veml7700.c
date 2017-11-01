/*
===============================================================================
Driver Name    :     veml7700
Author         :     ANDRE KILIAN
License        :     GPL
Description    :     LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include"veml7700.h"
#include "chardev.h"

#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <asm/uaccess.h>   /* for get_user and put_user */

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define veml7700_DRV_NAME "veml7700"
#define DEBUG 1

#define COMMAND_ALS_SM      0x00   /**/ 
#define COMMAND_ALS_WH      0x01   /**/ 
#define COMMAND_ALS_WL      0x02   /**/ 
#define COMMAND_PSM           0x03   /**/ 
#define COMMAND_PSM_EN      0x03   /**/ 
#define COMMAND_ALS           0x04   /**/ 
#define COMMAND_WHITE         0x05   /**/ 
#define COMMAND_ALS_IF_L    0x06   /**/ 
#define COMMAND_ALS_IF_H    0x06   /**/ 

#define VEML7700_N_MINORS 1
#define VEML7700_FIRST_MINOR 0
#define VEML7700_NODE_NAME "veml7700"
#define VEML7700_BUFF_SIZE 1024
#define SUCCESS 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDRE KILIAN");

static int veml7700_major=0;

static dev_t dev_num;

static struct class *veml7700_class;

typedef struct i2c_private {

	struct i2c_client *client;

	char buff[VEML7700_BUFF_SIZE];

	int irq;
	struct cdev cdev;

	struct device *veml7700_device;

} veml7700_private;

/* TODO Fill the I2C device table */

static const struct i2c_device_id veml7700_i2c_driver_ids[]={
	{ "veml7700", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c,veml7700_i2c_driver_ids);

atomic_t dev_cnt = ATOMIC_INIT(VEML7700_FIRST_MINOR - 1);


static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "VEML7700 ######################################### OPEN\n");
	return 0;
}

static ssize_t device_read(struct file *file,   /* see include/linux/fs.h   */
				char __user * buffer,   /* buffer to be
							 * filled with data */
				size_t length, /* length of the buffer     */
				loff_t * offset)
{
	int bytes_read = 99;
	printk(KERN_DEBUG "VEML7700 ######################################### READ\n");
	return bytes_read;
}

static ssize_t device_write(struct file *file,
		  const char __user * buffer, size_t length, loff_t * offset)
{
	int i = 0;
	printk(KERN_DEBUG "VEML7700 ######################################### WRITE\n");
	return i;
}

static int device_release(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "VEML7700 ######################################### RELEASE\n");
	return 0;
}

int device_unlocked_ioctl(struct inode *inode,  /* see include/linux/fs.h */
		 struct file *file,  /* ditto */
		 unsigned int ioctl_num,   /* number and param for ioctl */
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
		printk(KERN_DEBUG "VEML7700 ######################################### SETMSG\n");
		break;

	case IOCTL_GET_MSG:
		/*
		 * Give the current message to the calling process -
		 * the parameter we got is a pointer, fill it.
		 */
		printk(KERN_DEBUG "VEML7700 ######################################### GETMSG\n");
		break;

	case IOCTL_GET_NTH_BYTE:
		/*
		 * This ioctl is both input (ioctl_param) and
		 * output (the return value of this function)
		 */
		printk(KERN_DEBUG "VEML7700 ######################################### NTHBYTE\n");
		break;
	}

	return SUCCESS;
}

static const struct file_operations veml7700_fops= {
	.owner            = THIS_MODULE,
	.read     = device_read,//
	.write    = device_write,//
	.unlocked_ioctl    = device_unlocked_ioctl,//
	.open     = device_open,//
	.release = device_release, /* a.k.a. close */
};

static int veml7700_probe(struct i2c_client *client ,
							const struct i2c_device_id *id)
{
	dev_t curr_dev;
	unsigned int minor = atomic_inc_return(&dev_cnt);
	struct i2c_private *priv;

	PINFO("In i2c probe() function\n");

	if (minor == VEML7700_N_MINORS + VEML7700_FIRST_MINOR)
		return -EAGAIN;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "Unable to allocate memory for private data\n");
		return -ENOMEM;
	}

	priv->client = client;
	priv->irq = client->irq;   
	i2c_set_clientdata(client, priv);

	cdev_init(&priv->cdev , &veml7700_fops);

	curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + minor);
	cdev_add(&priv->cdev, curr_dev,1);

	priv->veml7700_device  = 
				device_create(veml7700_class , NULL ,curr_dev ,
					priv ,VEML7700_NODE_NAME"%d",VEML7700_FIRST_MINOR);
	if(!priv->veml7700_device) {
		PERR("device creation failed\n");
		return -1;
	}

	int e = i2c_smbus_read_byte_data(client, COMMAND_ALS);
	printk(KERN_DEBUG "VEML7700 ######################################### ALS VAL: %d\n",e);

	e = 0;

	e = i2c_smbus_read_byte_data(client, COMMAND_WHITE);
	printk(KERN_DEBUG "VEML7700 ######################################### WHITE VAL: %d\n",e);

	return 0;
}

static int veml7700_remove(struct i2c_client *client )
{
	struct i2c_private *priv = i2c_get_clientdata(client);
	PINFO("In i2c remove() function\n");

	device_destroy(veml7700_class ,priv->cdev.dev);

	cdev_del(&priv->cdev);

	kfree(priv);

	return 0;
}

static struct i2c_driver veml7700_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner   = THIS_MODULE,
	 },
	 .probe     = veml7700_probe,
	 .remove    = veml7700_remove,
	 .id_table  = veml7700_i2c_driver_ids,
};

static int __init veml7700_init(void)
{
	/* TODO Auto-generated Function Stub */

	int res;

	res = alloc_chrdev_region(&dev_num,VEML7700_FIRST_MINOR,VEML7700_N_MINORS ,DRIVER_NAME);
	if(res) {
		PERR("register device no failed\n");
		return -1;
	}
	veml7700_major = MAJOR(dev_num);

	veml7700_class = class_create(THIS_MODULE , DRIVER_NAME);
	if(!veml7700_class) {
		PERR("class creation failed\n");
		return -1;
	}

	res = i2c_add_driver(&veml7700_i2c_driver);
	if(res) {
		PERR("Adding driver to i2c core failed\n");
		return res;
	}

	PINFO("INIT\n");

	return 0;
}

static void __exit veml7700_exit(void)
{  
	/* TODO Auto-generated Function Stub */

	PINFO("EXIT\n");

	i2c_del_driver(&veml7700_i2c_driver);

	class_destroy(veml7700_class);

	unregister_chrdev_region(dev_num ,VEML7700_N_MINORS);

}

module_init(veml7700_init);
module_exit(veml7700_exit);

