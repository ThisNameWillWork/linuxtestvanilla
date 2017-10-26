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
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <linux/module.h>

#define veml7700_DRV_NAME "veml7700"


static const struct i2c_device_id veml7700_id[] = {
	{ "veml7700", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, veml7700_id);

static int veml7700_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	printk("######################################### PROBE\n");
	dev_info(&client->dev, "Probing VEML7700");
	return 0;
}


static int veml7700_remove(struct i2c_client *client)
{
	return 0;
}



static int __init veml7700_init(void)
{
	/* TODO Auto-generated Function Stub */
	//dev_info(&client->dev, "Probing VEML7700");
	printk("######################################### PROBE\n");
	printk(KERN_CRIT "######################################### PROBE\n";

	return 0;
}

static void __exit veml7700_exit(void)
{
	/* TODO Auto-generated Function Stub */

	PINFO("EXIT\n");
	//dev_info(&client->dev, "Probing VEML7700");
	printk("######################################### PROBE\n");
	printk(KERN_CRIT "######################################### PROBE\n";

}

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

module_init(veml7700_init);
module_exit(veml7700_exit);

MODULE_AUTHOR("AK");
MODULE_DESCRIPTION("Vishay veml7700 ambient light sensor driver");
MODULE_LICENSE("GPL");
