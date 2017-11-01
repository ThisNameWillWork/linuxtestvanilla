
#define DRIVER_NAME "veml7700"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/i2c.h>
#include<linux/init.h>
#include<linux/kdev_t.h>
#include<linux/module.h>
#include<linux/slab.h>
#include<linux/types.h>
#include<linux/uaccess.h>
