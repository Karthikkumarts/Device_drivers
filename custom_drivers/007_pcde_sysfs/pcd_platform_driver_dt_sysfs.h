#ifndef PCD_PLATFORM_DT_SYSFS_H
#define PCD_PLATFORM_DT_SYSFS_H

#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/platform_device.h>
#include"platform.h"
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of.h>
#include<linux/of_device.h>
#include <linux/sysfs.h>

#define MEM_BUFFER_SIZE_1 512
#define MEM_BUFFER_SIZE_2 512
#define MEM_BUFFER_SIZE_3 512
#define MEM_BUFFER_SIZE_4 512

#define NO_OF_DEVICES 4
#undef pr_fmt
#define pr_fmt(fmt) "%s : "fmt,__func__

enum pcdev_name
{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};
struct device_config
{
	int config1;
	int config2;
};

extern struct device_config pcdev_config[];
struct pcdev_private_data
{
	struct pcdev_platform_data pdata;
	char * buff;
	dev_t dev_num;
	/*Cdev variable*/
	struct cdev cdev;
};

struct pcdrv_private_data {
	int total_devices;
	dev_t device_num_base;
	struct class * class_pcd;
	struct device * device_pcd;
};
int pcd_open (struct inode *inode, struct file * filp);
ssize_t pcd_read (struct file * filp, char __user *buff, size_t count, loff_t *f_pos);
loff_t pcd_llseek (struct file *filp , loff_t off, int whence);
ssize_t pcd_write (struct file * filp, const char __user * buff, size_t count, loff_t *f_pos);
int pcd_release (struct inode *inode, struct file * filp);
#endif
