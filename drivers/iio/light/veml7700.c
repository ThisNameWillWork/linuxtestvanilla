/*
===============================================================================
Driver Name    :     veml7700
Author         :     ANDRE KILIAN
License        :     GPL
Description    :     LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include"veml7700.h"
//#include "chardev.h"

#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <asm/uaccess.h>   /* for get_user and put_user */

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define veml7700_DRV_NAME "veml7700"
#define DEBUG 1

#define COMMAND_ALS_SM 		0x00   /**/
#define COMMAND_ALS_WH      0x01   /**/
#define COMMAND_ALS_WL      0x02   /**/
#define COMMAND_PSM         0x03   /**/
#define COMMAND_PSM_EN      0x03   /**/
#define COMMAND_ALS         0x04   /**/
#define COMMAND_WHITE       0x05   /**/
#define COMMAND_ALS_IF_L    0x06   /**/
#define COMMAND_ALS_IF_H    0x06   /**/

#define VEML7700_N_MINORS 1
#define VEML7700_FIRST_MINOR 0
#define VEML7700_NODE_NAME "veml7700"
#define VEML7700_BUFF_SIZE 1024

uint16_t gain;
static uint16_t ZERO = 0;  
static uint16_t ALS_GAIN_x1 = 0x00;  // x 1
static uint16_t ALS_GAIN_x2 = 0x01;  // x 2
static uint16_t ALS_GAIN_d8 = 0x02;  // x 1/8
static uint16_t ALS_GAIN_d4 = 0x03;  // x 1/4

uint16_t inttime;
static uint16_t ALS_INTEGRATION_25ms = 0xc;
static uint16_t ALS_INTEGRATION_50ms = 0x8;
static uint16_t ALS_INTEGRATION_100ms = 0x0;
static uint16_t ALS_INTEGRATION_200ms = 0x1;
static uint16_t ALS_INTEGRATION_400ms = 0x2;
static uint16_t ALS_INTEGRATION_800ms = 0x3;

static uint16_t ALS_PERSISTENCE_1 = 0x0;
static uint16_t ALS_PERSISTENCE_2 = 0x1;
static uint16_t ALS_PERSISTENCE_4 = 0x2;
static uint16_t ALS_PERSISTENCE_8 = 0x3;

static uint16_t ALS_POWER_MODE_1 = 0x0;
static uint16_t ALS_POWER_MODE_2 = 0x1;
static uint16_t ALS_POWER_MODE_3 = 0x2;
static uint16_t ALS_POWER_MODE_4 = 0x3;

static uint16_t STATUS_OK = 0;
static uint16_t STATUS_ERROR = 0xff;

  //########################################################################

#define I2C_ADDRESS 0x10

#define COMMAND_ALS_SM 0x00
#define ALS_SM_MASK 0x1800
#define ALS_SM_SHIFT 11

#define COMMAND_ALS_IT 0x00
#define ALS_IT_MASK 0x03c0
#define ALS_IT_SHIFT 6

#define COMMAND_ALS_PERS 0x00
#define ALS_PERS_MASK 0x0030
#define ALS_PERS_SHIFT 4

#define COMMAND_ALS_INT_EN 0x00
#define ALS_INT_EN_MASK 0x0002
#define ALS_INT_EN_SHIFT 1

#define COMMAND_ALS_SD 0x00
#define ALS_SD_MASK 0x0001
#define ALS_SD_SHIFT 0

#define COMMAND_ALS_WH 0x01
#define COMMAND_ALS_WL 0x02

#define COMMAND_PSM 0x03
#define PSM_MASK 0x0006
#define PSM_SHIFT 1

#define COMMAND_PSM_EN 0x03
#define PSM_EN_MASK 0x0001
#define PSM_EN_SHIFT 0

#define COMMAND_ALS 0x04
#define COMMAND_WHITE 0x05

#define COMMAND_ALS_IF_L 0x06
#define ALS_IF_L_MASK 0x8000
#define ALS_IF_L_SHIFT 15

#define COMMAND_ALS_IF_H 0x06
#define ALS_IF_H_MASK 0x4000
#define ALS_IF_H_SHIFT 14

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDRE KILIAN");

static int veml7700_major=0;

static dev_t dev_num;

static struct class *veml7700_class;

static struct i2c_private *priv;

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
	printk(KERN_DEBUG "-------------------------------------------------------------\n");
	printk(KERN_DEBUG "VEML7700 ######################################### OPEN\n");
	return 0;
}

static ssize_t device_read(struct file *file,   /* see include/linux/fs.h   */
				char __user * buffer,   /* buffer to be
							 * filled with data */
				size_t length, /* length of the buffer     */
				loff_t * offset)
{
	int bytes_read,ret;
	float factor1,factor2,retf;
	bytes_read	= 99;
	ret 		= 0;
	factor1 	= 0.f;
	factor2 	= 0.f;

	printk(KERN_DEBUG "VEML7700 ######################################### READ\n");

	ret = i2c_smbus_read_word_data(priv->client, COMMAND_ALS);
	printk(KERN_DEBUG "VEML7700 ######################################### RET: %d\n",ret);



	  // if(gain==0x00)
	  //   factor1 = 1.f;
	  // else if(gain==0x01)
	  //   factor1 = 0.5f;
	  // else if(gain==0x02)
	  //   factor1 = 8.f;
	  // else if(gain==0x03)
	  //   factor1 = 4.f;
	  // else
	  //   factor1 = 1.f;


	  // if(inttime==0xc)
	  //   factor2 = 0.2304f;
	  // else if(inttime==0x8)
	  //   factor2 = 0.1152f;
	  // else if(inttime==0x0)
	  //   factor2 = 0.0576f;
	  // else if(inttime==0x1)
	  //   factor2 = 0.0288f;
	  // else if(inttime==0x2)
	  //   factor2 = 0.0144f;
	  // else if(inttime==0x3)
	  //   factor2 = 0.0072f;
	  // else
	  //   factor2 = 0.2304f;

	

	//printk(KERN_DEBUG "VEML7700 ######################################### LUX: %d * %f * %f = \n",ret,factor1,factor2);
	//float buf = (float) ret;
	//printk(KERN_DEBUG "VEML7700 ######################################### BUF: %f\n",buf);
	//ret = (int) ((float)ret * 0.0288f);
	//printk(KERN_DEBUG "VEML7700 ######################################### RET: %d\n",ret);

	return ret;
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

static const struct file_operations veml7700_fops= {
	.owner            = THIS_MODULE,
	.read     = device_read,//
	.write    = device_write,//
	.open     = device_open,//
	.release  = device_release, /* a.k.a. close */
};

static int veml7700_probe(struct i2c_client *client ,
							const struct i2c_device_id *id)
{
	dev_t curr_dev;
	unsigned int minor = atomic_inc_return(&dev_cnt);

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

	// int e = i2c_smbus_read_byte_data(client, COMMAND_ALS);
	// printk(KERN_DEBUG "VEML7700 ######################################### ALS VAL: %d\n",e);

	// e = 0;

	// e = i2c_smbus_read_byte_data(client, COMMAND_WHITE);
	// printk(KERN_DEBUG "VEML7700 ######################################### WHITE VAL: %d\n",e);

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

	int res,x;
	char register_cache[4];
	x = 99;

	//Configuration VEML7700
	gain = ALS_GAIN_x2;
	inttime = ALS_INTEGRATION_100ms;

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

  // write initial state to VEML7700
  register_cache[0] = ( gain<<ALS_SM_SHIFT |
                        inttime<<ALS_IT_SHIFT |
                        ALS_PERSISTENCE_1<<ALS_PERS_SHIFT |
                        ZERO<<ALS_INT_EN_SHIFT |
                        ZERO<<ALS_SD_SHIFT);
  register_cache[1] = 0x0000;
  register_cache[2] = 0xffff;
  register_cache[3] = ( ALS_POWER_MODE_3 << PSM_SHIFT | ZERO << PSM_EN_SHIFT );



  x = i2c_smbus_write_byte_data(priv->client, 0x00, register_cache[0]);
  printk(KERN_DEBUG "VEML7700 ######################################### CONF0: %d\n",x);
  x = i2c_smbus_write_byte_data(priv->client, 0x01, register_cache[1]);
  printk(KERN_DEBUG "VEML7700 ######################################### CONF1: %d\n",x);
  x = i2c_smbus_write_byte_data(priv->client, 0x02, register_cache[2]);
  printk(KERN_DEBUG "VEML7700 ######################################### CONF2: %d\n",x);
  x = i2c_smbus_write_byte_data(priv->client, 0x03, register_cache[3]);
  printk(KERN_DEBUG "VEML7700 ######################################### CONF3: %d\n",x);


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

