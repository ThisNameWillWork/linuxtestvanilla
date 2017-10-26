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

struct veml7700_data {
	struct i2c_client *client;
};

static const struct i2c_device_id veml7700_id[] = {
	{ "veml7700", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, veml7700_id);

static const struct of_device_id veml7700_of_match[] = {
	{ .compatible = "veml7700" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, veml7700_of_match);

static const struct iio_chan_spec veml7700_channels[] = {
	{
		.type = IIO_LIGHT,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_SCALE),
	}, {
		.type = IIO_PROXIMITY,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	}
};

static int veml7700_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{

	return 0;
}

static const struct iio_info veml7700_info = {
	.read_raw = veml7700_read_raw,
	.driver_module = THIS_MODULE,
};

static int veml7700_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	dev_info(&client->dev, "Probing VEML7700");
	printk("######################################### PROBE\n");
	printk(KERN_CRIT "######################################### PROBE\n";
	return 0;
}

static int veml7700_remove(struct i2c_client *client)
{
	printk("######################################### REMOVE\n");
	printk(KERN_CRIT "######################################### REMOVE\n";
	return 0;
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
