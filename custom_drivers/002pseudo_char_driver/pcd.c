#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>

#define DEV_MEM_BUFFER 512

#undef pr_fmt
#define pr_fmt(fmt) "%s : "fmt,__func__

/* this is our pseudo device where we will read write the memory*/
char device_buffer[DEV_MEM_BUFFER];


/*this holds the device number(major and minor) */
dev_t device_number;

/*Cdev variable*/
struct cdev pcd_cdev;

loff_t pcd_llseek (struct file *filp , loff_t off, int whence)
{
	loff_t temp =0;

	switch(whence)
	{
		case SEEK_SET:
			if((off > DEV_MEM_BUFFER) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;

		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > DEV_MEM_BUFFER) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = DEV_MEM_BUFFER + off;
			if((temp > DEV_MEM_BUFFER) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;

	}
	return filp->f_pos;
}

ssize_t pcd_read (struct file * filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested %zu bytes\n",count);
	pr_info("current file pos %lld\n",*f_pos);

	/*adjust the count */
	if((*f_pos + count) > DEV_MEM_BUFFER)
		count = DEV_MEM_BUFFER - *f_pos;
	
	if(copy_to_user(buff,&device_buffer[*f_pos],count))
		return -EFAULT;


	*f_pos += count;
	pr_info("number of bytes successfully read : %zu \n",count);
	pr_info("updated file pos %lld\n",*f_pos);
	return count;

}
ssize_t pcd_write (struct file * filp, const char __user * buff, size_t count, loff_t *f_pos)
{
	pr_info("write requested %zu bytes\n",count);
	pr_info("current file pos %lld\n",*f_pos);

	/*adjust the count */
	if((*f_pos + count) > DEV_MEM_BUFFER)
		count = DEV_MEM_BUFFER - *f_pos;

	if(!count)
	{
		pr_info("NO memory to alloacte\n");
		return -ENOMEM;
	}
	
	if(copy_from_user(&device_buffer[*f_pos],buff,count))
		return -EFAULT;

	*f_pos += count;
	pr_info("number of bytes successfully read : %zu \n",count);
	pr_info("updated file pos %lld\n",*f_pos);
	return count;
}

int pcd_open (struct inode *inode, struct file * filp)
{
	pr_info("open was successful\n");
	return 0;
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

struct class * class_pcd;
struct device * device_pcd;

static int __init pcd_driver_init(void)
{
	int ret =0;
	pr_info("started init function");
	/*1. Dynamically allocate a device number*/
	ret = alloc_chrdev_region(&device_number,0,1,"pcd");
	if(ret < 0)
	{
		pr_info("alloc_char_dev failed\n");
		goto out;
	}

	pr_info("the device number <major>:<minor> is %d:%d\n",MAJOR(device_number),MINOR(device_number));
	/*2. Initialize cdev structure with fops*/
	cdev_init(&pcd_cdev , &pcd_fops);
	pcd_cdev.owner = THIS_MODULE;

	/*3. Register a device ( cdev structure ) with VFS */
	ret = cdev_add(&pcd_cdev,device_number,1);
	if(ret < 0)
	{
		pr_info("cdev_add failed\n");
		goto unreg_chardev;
	}


	/*4. create device class under /sys/class */
	class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(class_pcd))
	{
		pr_info("class_create failed\n" );
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}

	/*5. populate the sysfs with device number */
	device_pcd = device_create(class_pcd,NULL,device_number,NULL,"pcd");
	if(IS_ERR(device_pcd))
	{
		pr_info("device_create failed\n" );
		ret = PTR_ERR(device_pcd);
		goto class_destroy;
	}

	return 0;
class_destroy:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chardev:
	unregister_chrdev_region(device_number,1);
out:
	return ret;

}

static void __exit pcd_driver_exit(void)
{
	device_destroy(class_pcd,device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number,1);

	pr_info("done by unloaded the module\n");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KARTHIK KUMAR");
MODULE_DESCRIPTION("To understand the char device driver");
MODULE_INFO(board,"beagle bone");
