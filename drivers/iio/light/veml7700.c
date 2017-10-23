/*
===============================================================================
Driver Name		:		veml7700
Author			:		ANDRE KILIAN
License			:		GPL
Description		:		LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include"veml7700.h"
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANDRE KILIAN");


#define COMMAND_ALS_SM			0x00 	/**/
#define COMMAND_ALS_WH			0x01 	/**/
#define COMMAND_ALS_WL			0x02 	/**/
#define COMMAND_PSM				0x03 	/**/
#define COMMAND_PSM_EN			0x03 	/**/
#define COMMAND_ALS				0x04 	/**/
#define COMMAND_WHITE			0x05 	/**/
#define COMMAND_ALS_IF_L		0x06 	/**/
#define COMMAND_ALS_IF_H		0x06 	/**/


/* Bit masks for COMMAND register */
//#define VEML7700_AL_RDY		0x40 /* ALS data ready? */
//#define VEML7700_PS_RDY		0x20 /* proximity data ready? */
//#define VEML7700_AL_OD		0x10 /* start on-demand ALS measurement */
//#define VEML7700_PS_OD		0x08 /* start on-demand proximity measurement */


static const struct i2c_device_id veml7700_id[] = {
	{ "veml7700", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, veml7700_id);

static struct i2c_driver veml7700_driver = {
	.driver = {
		.name	= "veml7700",
	},

	.id_table	= veml7700_id,
	.probe		= veml7700_probe,
	.remove		= veml7700_remove,
}

static int veml770_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	printk(KERN_DEBUG "PROBE: %s:%i\n", __FILE__, __LINE_&_);
	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if(res == NULL){
		printf("TEST DRIVER NULL");
	}


	return 0;
}

static int vcnl4000_remove(struct i2c_client *client)
{
	printk(KERN_DEBUG "REMOVE: %s:%i\n", __FILE__, __LINE_&_);
	return 0;
}

static int veml7700_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	printk(KERN_DEBUG "READ RAW: %s:%i\n", __FILE__, __LINE_&_);

	int tries = 20;
	int ret;

	ret = i2c_smbus_write_byte_data(data->client, COMMAND_ALS,mask);

	if (ret < 0)
			return ret;

	return 0;
}

static const struct iio_info veml7700_info = {
	.read_raw = veml7700_read_raw,
	.driver_module = THIS_MODULE,
};

static int __init veml7700_init(void)
{
	/* TODO Auto-generated Function Stub */
	printk(KERN_DEBUG "Here I am: %s:%i\n", __FILE__, __LINE_&_);
	PINFO("INIT\n");

	return i2c_add_driver(&veml7700_driver);
}

static void __exit veml7700_exit(void)
{	
	/* TODO Auto-generated Function Stub */

	PINFO("EXIT\n");

}

module_i2c_driver(veml7700_driver);

