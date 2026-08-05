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
#define MEM_BUFFER_SIZE_1 512
#define MEM_BUFFER_SIZE_2 512
#define MEM_BUFFER_SIZE_3 512
#define MEM_BUFFER_SIZE_4 512

#define NO_OF_DEVICES 4
#undef pr_fmt
#define pr_fmt(fmt) "%s : "fmt,__func__

#define RDONLY 0x01
#define WRONLY 0X10
#define RDWR 0x11
/* this is our pseudo device where we will read write the memory*/
char device_buffer_pcdev1[MEM_BUFFER_SIZE_1];
char device_buffer_pcdev2[MEM_BUFFER_SIZE_2];
char device_buffer_pcdev3[MEM_BUFFER_SIZE_3];
char device_buffer_pcdev4[MEM_BUFFER_SIZE_4];
enum pcdev_name
{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X
};
struct device_config
{
	int config1;
	int config2;
};
struct device_config pcdev_config[] = {
	[PCDEVA1X] = {.config1 = 60 , .config2 = 20},
	[PCDEVB1X] = {.config1 = 70 , .config2 = 21},
	[PCDEVC1X] = {.config1 = 80 , .config2 = 22}
};

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
struct pcdrv_private_data pcdrv_data;

loff_t pcd_llseek (struct file *filp , loff_t off, int whence)
{
	loff_t temp =0;
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	int max_size = pcdev_data->pdata.size;

	switch(whence)
	{
		case SEEK_SET:
			if((off > max_size) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;

		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + off;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;

	}
	return filp->f_pos;
}

ssize_t pcd_read (struct file * filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("read requested %zu bytes\n",count);
	pr_info("current file pos %lld\n",*f_pos);

	/*adjust the count */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	if(copy_to_user(buff,&pcdev_data->buff+*f_pos,count))
		return -EFAULT;


	*f_pos += count;
	pr_info("number of bytes successfully read : %zu \n",count);
	pr_info("updated file pos %lld\n",*f_pos);
	return count;
}
ssize_t pcd_write (struct file * filp, const char __user * buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("write requested %zu bytes\n",count);
	pr_info("current file pos %lld\n",*f_pos);

	/*adjust the count */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	if(!count)
	{
		pr_info("NO memory to alloacte\n");
		return -ENOMEM;
	}

	if(copy_from_user(&pcdev_data->buff+*f_pos,buff,count))
		return -EFAULT;

	*f_pos += count;
	pr_info("number of bytes successfully wrote : %zu \n",count);
	pr_info("updated file pos %lld\n",*f_pos);
	return count;
}

int check_permission(int dev_perm , int acc_mode)
{
	if(dev_perm == RDWR)
		return 0;
	if((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;
	if((dev_perm == WRONLY) && (!(acc_mode & FMODE_READ) && (acc_mode & FMODE_WRITE)))
		return 0;

	return -EPERM;
}
int pcd_open (struct inode *inode, struct file * filp)
{
	int ret , minor_number;
	struct pcdev_private_data *pcdev_data;

	/* find out which device file open was attempted  by user space */
	minor_number = MINOR(inode->i_rdev);
	pr_info("minor number : %d\n",minor_number);
	/*get device's private data*/
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	filp->private_data = pcdev_data;

	/*check permission*/
	pr_info("perm : %x , f_mode : %d",pcdev_data->pdata.perm,filp->f_mode);
	ret = check_permission(pcdev_data->pdata.perm , filp->f_mode);
	(!ret)?pr_info("open was successfull\n"):pr_info("open was not successfull\n");
	return ret;
}

int pcd_release (struct inode *inode, struct file * filp)
{
	pr_info("release was successful\n");
	return 0;
}

/*file operation of the driver*/
struct file_operations pcd_fops =
{
	.open = pcd_open,
	.release = pcd_release,
	.llseek = pcd_llseek,
	.read = pcd_read,
	.write = pcd_write
};

int pcd_platform_driver_probe(struct platform_device * pcdev)
{
	int ret;
	pr_info("A matching device detected\n");

	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data * pdata; //instead of taking extra structure for platfrom_data , first allocate the memory for dev_data and then directly take the dev_get_platdata to dev_data->pdata.
        
	/* 1. Get the platform data */
	
	//pdata = pcdev->dev.platform_data;
	pdata = (struct pcdev_platform_data *)dev_get_platdata(&pcdev->dev);
	if(!pdata)
	{
		pr_info("NO platform data available\n");
		return -EINVAL;
	}
	/* 2.Dynamically allocate memory for device private data */

	dev_data = kzalloc(sizeof(*dev_data),GFP_KERNEL);
	if(!dev_data){
		pr_info("cannot allocate memory\n");
		return -ENOMEM;
	}
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial number : %s\n",dev_data->pdata.serial_number);
	pr_info("Device permission : %d\n",dev_data->pdata.perm);
	pr_info("Device size : %d\n",dev_data->pdata.size);

	pr_info("config item 1 : %d\n",pcdev_config[pcdev->id_entry->driver_data].config1);
	pr_info("config item 2 : %d\n",pcdev_config[pcdev->id_entry->driver_data].config2);

	/* 3. Dynamically allocate the memory for device buffer using size information from the platform data */

	dev_data->buff = kzalloc(dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data->buff){
		pr_info("cannot allocate memory for buffer\n");
		return -ENOMEM;
	}

	/*4. Get the device number */
	dev_data->dev_num = pcdrv_data.device_num_base + pcdev->id;

	/* 5. Do cdev init and cdev add */
	cdev_init(&dev_data->cdev,&pcd_fops);
	
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret < 0)
	{
		pr_err("Cdev add failed\n");
		return ret;
	}
	dev_data->cdev.owner = THIS_MODULE;

	/* 6. create  device file for the detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,dev_data->dev_num,NULL,"pcdev-%d",pcdev->id);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("device creation failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
	        cdev_del(&dev_data->cdev);
	}

	dev_set_drvdata(&pcdev->dev,dev_data);
	pcdrv_data.total_devices++;
	pr_info("Probe was successfull\n");
	return 0;
}

/*void for the host  and int for beagle bone as kernel version are differnet */
//int  pcd_platform_driver_remove(struct platform_device * pcdev)
void  pcd_platform_driver_remove(struct platform_device * pcdev)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pcdev->dev); 
	/* 1. Remove  a device which was created by device_create */
	device_destroy(pcdrv_data.class_pcd , dev_data->dev_num);

	/*2. remove cdev entry */
	cdev_del(&dev_data->cdev);

	/*3  Free the memory held by the device */
	kfree(dev_data->buff);
	kfree(dev_data);
	
	pcdrv_data.total_devices--;
	pr_info("A device removed\n");
}

struct platform_device_id pcdev_ids[] = {
	[0] = {.name = "pcdev-A1x",.driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x",.driver_data=PCDEVB1X},
	[2] = {.name = "pcdev-C1x",.driver_data=PCDEVC1X}
};
struct platform_driver pcd_platform_driver = {

	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdev_ids,
	.driver = {
        	.name = "pcdev",
    },
};
//.driver will no longer be required for device and driver matching
#define MAX_DEVICE 10
static int __init pcd_platform_driver_init(void)
{
	/* 1. Dynamically allocate the device number for MAX_DEVICE */
	int ret , i ;
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base , 0 , MAX_DEVICE,"pcdevs");
	if(ret < 0)
	{
		pr_err("Allc chrdev failed\n");
		return ret;
	}
	
	/* 2. create the class 	under /sys/class */
	//beagle-bone
	//pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
	
	//for latest kernel 
	pcdrv_data.class_pcd = class_create("pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICE);
		return ret;
	}

	/* 3.  Register platform driver */
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver loaded\n");
	return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
	platform_driver_unregister(&pcd_platform_driver);
	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICE);
	pr_info("pcd platform driver unloaded\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KARTHIK KUMAR");
MODULE_DESCRIPTION("A pseudo character driver which handles n devices");
MODULE_INFO(board,"beagle bone");
