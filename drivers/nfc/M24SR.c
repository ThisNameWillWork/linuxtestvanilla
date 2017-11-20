/*
===============================================================================
Driver Name    :     m24sr
Author         :
License        :     GPL
Description    :     LINUX DEVICE DRIVER PROJECT
===============================================================================
*/


//#include "chardev.h"

//##############################
#include"m24sr.h"

/* Init NDEF_FileID with bad value in case Init failed */
static uint16_t NDEF_FileID = 0xDEAD;
//##############################


#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <asm/uaccess.h>   /* for get_user and put_user */
#include <linux/nfc.h>
#include <linux/netdevice.h>
#include <net/nfc/nfc.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define m24sr_DRV_NAME "m24sr16"
#define m24sr_NODE_NAME "m24sr16"
#define m24sr_N_MINORS 1
#define m24sr_FIRST_MINOR 0
#define DEBUG 1


//########################################################################

#define I2C_ADDRESS 0x56

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDRE KILIAN");

static int m24sr_major=0;

static dev_t dev_num;

static struct class *m24sr_class;

static struct i2c_private *priv;

typedef struct i2c_private {

	struct i2c_client *client;

	int irq;
	struct cdev cdev;

	struct device *m24sr_device;

} m24sr_private;

/* TODO Fill the I2C device table */

static const struct i2c_device_id m24sr_i2c_driver_ids[]={
	{ "m24sr16", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c,m24sr_i2c_driver_ids);

atomic_t dev_cnt = ATOMIC_INIT(m24sr_FIRST_MINOR - 1);


static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "-------------------------------------------------------------\n");
	printk(KERN_DEBUG "m24sr ######################################### OPEN\n");
	return 0;
}

static ssize_t device_read(struct file *file,   /* see include/linux/fs.h   */
				char __user * buffer,   /* buffer to be
							 * filled with data */
				size_t length, /* length of the buffer     */
				loff_t * offset)
{
	int bytes_read,ret;
	ret 		= 0;
	return ret;
}

static ssize_t device_write(struct file *file,
		  const char __user * buffer, size_t length, loff_t * offset)
{
	int i = 0;
	printk(KERN_DEBUG "m24sr ######################################### WRITE\n");
	return i;
}

static int device_release(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "m24sr ######################################### RELEASE\n");
	return 0;
}

static const struct file_operations m24sr_fops= {
	.owner    = THIS_MODULE,
	.read     = device_read,//
	.write    = device_write,//
	.open     = device_open,//
	.release  = device_release, /* a.k.a. close */
};

/**
  * @brief  This function sends the GetSession command to the M24SR device
	* @retval M24SR_ACTION_COMPLETED : the function is succesful.
  * @retval Status (SW1&SW2) : if operation does not complete.
  */
uint16_t M24SR_GetSession ( void )
{
	uint8_t Buffer = M24SR_OPENSESSION;
	int16_t 	status;

	//errchk(M24SR_SendI2Ccommand ( 0x01 , &Buffer ));
	status = i2c_master_send(priv->client, Buffer, 0x01);
	printk(KERN_DEBUG "M24SR ######################################### GETSESS: %d\n",status);


	return M24SR_ACTION_COMPLETED;
}

static int m24sr_probe(struct i2c_client *client ,
							const struct i2c_device_id *id)
{
	printk(KERN_DEBUG "M24SR ######################################### PROBE");
	dev_t curr_dev;
	unsigned int minor = atomic_inc_return(&dev_cnt);
	int ret = 0;
	char rset_cmd[] = { 0x00,0x05, 0xF9, 0x04, 0x00, 0xC3, 0xE5 };
	char getSession[] = { M24SR_OPENSESSION };
	char cmd1[] 	= { 0x00,0x26, 0x00, 0x00, 0x00, 0x06};
	char cmd2[] 	= { 0x00,0x03, 0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03, 0xD2, 0xAF };
	char cmd3[] 	= { 0x00,0xa4, 0x04, 0x00, 0x07, 0xd2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
	//0x00 A4 04 00 07 D2 76 00 00 85 01 01 00

	int count = sizeof(rset_cmd);
	int sCount = sizeof(getSession);
	int count1 = sizeof(cmd1);
	int count2 = sizeof(cmd2);
	int count3 = sizeof(cmd3);

	PINFO("In i2c probe() function\n");

	if (minor == m24sr_N_MINORS + m24sr_FIRST_MINOR)
		return -EAGAIN;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "Unable to allocate memory for private data\n");
		return -ENOMEM;
	}

	priv->client = client;
	priv->irq = client->irq;
	i2c_set_clientdata(client, priv);

	cdev_init(&priv->cdev , &m24sr_fops);

	curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + minor);
	cdev_add(&priv->cdev, curr_dev,1);

	priv->m24sr_device  =
				device_create(m24sr_class , NULL ,curr_dev ,
					priv ,m24sr_NODE_NAME"%d",m24sr_FIRST_MINOR);
	if(!priv->m24sr_device) {
		PERR("device creation failed\n");
		return -1;
	}

	//INIT pn544_hci_i2c_platform_init


	ret = i2c_master_send(priv->client, getSession, sCount);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, rset_cmd, count);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, cmd1, count1);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, cmd2, count2);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, cmd3, count3);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	//M24SR_GetSession();

	return 0;
}

static int m24sr_remove(struct i2c_client *client )
{
	struct i2c_private *priv = i2c_get_clientdata(client);
	PINFO("In i2c remove() function\n");

	device_destroy(m24sr_class ,priv->cdev.dev);

	cdev_del(&priv->cdev);

	kfree(priv);

	return 0;
}

static struct i2c_driver m24sr_i2c_driver = {
	.driver = {
		.name 	= m24sr_DRV_NAME,
		.owner  = THIS_MODULE,
	 },
	 .probe     = m24sr_probe,
	 .remove    = m24sr_remove,
	 .id_table  = m24sr_i2c_driver_ids,
};

static int __init m24sr_init(void)
{
	/* TODO Auto-generated Function Stub */

	int res;

	res = alloc_chrdev_region(&dev_num,m24sr_FIRST_MINOR,m24sr_N_MINORS ,DRIVER_NAME);
	if(res) {
		PERR("register device no failed\n");
		return -1;
	}
	m24sr_major = MAJOR(dev_num);

	m24sr_class = class_create(THIS_MODULE , DRIVER_NAME);
	if(!m24sr_class) {
		PERR("class creation failed\n");
		return -1;
	}

	res = i2c_add_driver(&m24sr_i2c_driver);
	if(res) {
		PERR("Adding driver to i2c core failed\n");
		return res;
	}
	PINFO("INIT\n");

	return 0;
}

static void __exit m24sr_exit(void)
{
	/* TODO Auto-generated Function Stub */

	PINFO("EXIT\n");

	i2c_del_driver(&m24sr_i2c_driver);

	class_destroy(m24sr_class);

	unregister_chrdev_region(dev_num ,m24sr_N_MINORS);

}

module_init(m24sr_init);
module_exit(m24sr_exit);

