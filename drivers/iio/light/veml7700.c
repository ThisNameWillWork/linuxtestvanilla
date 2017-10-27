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


#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define veml7700_DRV_NAME "veml7700"
#define COMMAND_ALS_SM      0x00   /**/ 
#define COMMAND_ALS_WH      0x01   /**/ 
#define COMMAND_ALS_WL      0x02   /**/ 
#define COMMAND_PSM        	0x03   /**/ 
#define COMMAND_PSM_EN      0x03   /**/ 
#define COMMAND_ALS        	0x04   /**/ 
#define COMMAND_WHITE      	0x05   /**/ 
#define COMMAND_ALS_IF_L    0x06   /**/ 
#define COMMAND_ALS_IF_H    0x06   /**/ 

static const struct i2c_device_id veml7700_id[] = {
	{ "veml7700", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, veml7700_id);

static int veml7700_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	printk(KERN_DEBUG "VEML7700 ######################################### PROBE\n");
	dev_info(&client->dev, "Probing VEML7700");

	int e = i2c_smbus_read_byte_data(client, COMMAND_ALS);
	printk(KERN_DEBUG "VEML7700 ######################################### VAL: %d\n",e);

	e = i2c_smbus_read_byte_data(client, COMMAND_WHITE);
	printk(KERN_DEBUG "VEML7700 ######################################### VAL: %d\n",e);

	return 0;
}


static int veml7700_remove(struct i2c_client *client)
{
	return 0;
}


static int veml7700_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	printk(KERN_DEBUG "VEML7700 ######################################### READ RAW\n");
	return 0;
}

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
