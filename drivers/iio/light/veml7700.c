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

static int __init veml7700_init(void)
{
	/* TODO Auto-generated Function Stub */

	PINFO("INIT\n");
		dev_info(&client->dev, "Probing VEML7700");
	printk("######################################### PROBE\n");
	printk(KERN_CRIT "######################################### PROBE\n";

	return 0;
}

static void __exit veml7700_exit(void)
{
	/* TODO Auto-generated Function Stub */

	PINFO("EXIT\n");
		dev_info(&client->dev, "Probing VEML7700");
	printk("######################################### PROBE\n");
	printk(KERN_CRIT "######################################### PROBE\n";

}

module_init(veml7700_init);
module_exit(veml7700_exit);

MODULE_AUTHOR("AK");
MODULE_DESCRIPTION("Vishay veml7700 ambient light sensor driver");
MODULE_LICENSE("GPL");
