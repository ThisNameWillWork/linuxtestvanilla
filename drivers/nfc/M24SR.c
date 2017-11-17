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

#define I2C_ADDRESS 0x2d

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

static int m24sr_probe(struct i2c_client *client ,
							const struct i2c_device_id *id)
{
	printk(KERN_DEBUG "M24SR ######################################### PROBE");
	dev_t curr_dev;
	unsigned int minor = atomic_inc_return(&dev_cnt);
	int ret = 0;
	char rset_cmd[] = { 0x00,0x05, 0xF9, 0x04, 0x00, 0xC3, 0xE5 };
	char cmd1[] 	= { 0x00,0x02, 0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00, 0x35, 0xC0 };
	char cmd2[] 	= { 0x00,0x03, 0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03, 0xD2, 0xAF };

	int count = sizeof(rset_cmd);

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


	ret = i2c_master_send(priv->client, rset_cmd, count);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, cmd1, count);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

	ret = i2c_master_send(priv->client, cmd2, count);
		printk(KERN_DEBUG "M24SR ######################################### RET: %d\n",ret);

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

///**
//  * @brief  Initialize the command and response structure
//  * @param  None
//  * @retval None
//  */
//static void M24SR_InitStructure ( void )
//{
//	/* build the command */
//	Command.Header.CLA = 0x00;
//	Command.Header.INS = 0x00;
//	/* copy the offset */
//	Command.Header.P1 = 0x00 ;
//	Command.Header.P2 = 0x00 ;
//	/* copy the number of byte of the data field */
//	Command.Body.LC = 0x00 ;
//	/* copy the number of byte to read */
//	Command.Body.LE = 0x00 ;
//	Command.Body.pData = DataBuffer;
//
//  // 	/* initializes the response structure*/
//  // 	Response.pData = DataBuffer;
//  // 	Response.SW1 = 0x00;
//  // 	Response.SW2 = 0x00;
//}

/**
  * @brief  This function initialize the M24SR device
  * @retval None
  */
//void M24SR_Init( void )
//{
//	M24SR_I2CInit();
//	// M24SR_GPOInit(); // Empty function
//
//	M24SR_InitStructure();
//
//#if defined (I2C_GPO_SYNCHRO_ALLOWED) || defined (I2C_GPO_INTERRUPT_ALLOWED)
//	if( M24SR_KillSession() == M24SR_ACTION_COMPLETED)
//	{
//    M24SR_ManageI2CGPO(I2C_ANSWER_READY);
//	  M24SR_Deselect ();
//	}
//#endif /* I2C_GPO_SYNCHRO_ALLOWED */
//}
//
///**
//  * @brief  This fonction initialize the M24SR
//	* @param	CCBuffer : pointer on the buffer to store CC file
//	* @param	size : number of byte of data to read
//  * @retval SUCCESS : Initalization done
//	* @retval ERROR : Not able to Initialize.
//  */
//static uint16_t M24SR_Initialization ( uint8_t* CCBuffer, uint8_t size )
//{
//	uint16_t status = ERROR;
//	uint16_t trials = 5; /* wait 1sec, driver is configured to let 200ms for command to complete */
//                       /* which is enough for all commands except GetSession if RF session is already opened */
//                       /* Smartphone generaly release the session within the second, but customer can modify this value */
//
//	/* Perform HW initialization */
//	M24SR_Init();
//
//	/* Read CC file */
//	while( status != M24SR_ACTION_COMPLETED && trials)
//	{
//		printk("Calling M24SR_GetSession(), attempts remaining %d\n", trials);
//		status = M24SR_GetSession();
//		printk("M24SR_GetSession() returned: %d\n", status);
//		trials--;
//	}
//	if (status != M24SR_ACTION_COMPLETED)
//	{
//		printf("M24SR_GetSession() failed\n");
//		return ERROR;
//	}
//	/*===================================*/
//	/* Select the NFC type 4 application */
//	/*===================================*/
//	errorchk( M24SR_SelectApplication() );
//
//	/*==================*/
//	/* select a CC file */
//	/*==================*/
//	errorchk (M24SR_SelectCCfile() );
//
//	/* read the first 15 bytes of the CC file */
//	if( M24SR_ReadData ( 0x0000 , 0x0F , CCBuffer )== M24SR_ACTION_COMPLETED)
//	{
//		NDEF_FileID = (uint16_t) ((CCBuffer[0x09]<<8) | CCBuffer[0x0A]);
//		errorchk( M24SR_Deselect () );
//		return 0;
//	}
//	else
//		errorchk( M24SR_Deselect () );
//
//	Error:
//		printf("Error in M24SR_Initialization\n");
//			return ERROR;
//
//}
