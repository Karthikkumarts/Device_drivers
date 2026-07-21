#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>

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


struct pcdev_private_data
{
	char * buff;
	unsigned int size;
	const char *serial_number;
	int perm;
	/*Cdev variable*/
	struct cdev cdev;
};

struct pcdrv_private_data {
	int total_devices;
	dev_t device_number;
	struct class * class_pcd;
	struct device * device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data =  {

	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buff = device_buffer_pcdev1,
			.size = MEM_BUFFER_SIZE_1,
			.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY, //RDONLY
		},
		[1] = {
			.buff = device_buffer_pcdev2,
			.size = MEM_BUFFER_SIZE_2,
			.serial_number = "PCDEV2XYZ123",
			.perm = WRONLY, //WRONLY
		},
		[2] = {
			.buff = device_buffer_pcdev3,
			.size = MEM_BUFFER_SIZE_3,
			.serial_number = "PCDEV3XYZ123",
			.perm = RDWR, //RDWR
		},
		[3] = {
			.buff = device_buffer_pcdev4,
			.size = MEM_BUFFER_SIZE_4,
			.serial_number = "PCDEV4XYZ123",
			.perm = RDWR, //RDWR
		}
	}
};

loff_t pcd_llseek (struct file *filp , loff_t off, int whence)
{
	loff_t temp =0;
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	int max_size = pcdev_data->size;

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
	int max_size = pcdev_data->size;
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
	int max_size = pcdev_data->size;
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
static int pcd_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    pr_info("pcd_uevent called\n");

    return add_uevent_var(env, "DEVMODE=%#o", 0666);
}
int pcd_open (struct inode *inode, struct file * filp)
{
	int ret , minor_number;
	struct pcdev_private_data *pcdev_data;

	/* find out which device file open was attempted  by user space */
	minor_number = MINOR(inode->i_rdev);
	pr_info("minor number : %d\n",minor_number);
pcdrv_data.class_pcd->dev_uevent = pcd_uevent;
	/*get device's private data*/
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	filp->private_data = pcdev_data;

	/*check permission*/
	pr_info("perm : %x , f_mode : %d",pcdev_data->perm,filp->f_mode);
	ret = check_permission(pcdev_data->perm , filp->f_mode);
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


static int __init pcd_driver_init(void)
{
	int ret =0;
	int i;
	pr_info("started init function");
	/*1. Dynamically allocate a device number*/
	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,4,"pcd");
	if(ret < 0)
	{
		pr_info("alloc_char_dev failed\n");
		goto out;
	}

	/*4. create device class under /sys/class */
	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_n_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_info("class_create failed\n" );
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chardev;
	}

	for(i = 0 ; i < NO_OF_DEVICES ; i++)
	{
		// device number is type dev_t not array to access in using index[i] ,rather it is  range of consecutive device numbers
		pr_info("the device number <major>:<minor> is %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));
		/*2. Initialize cdev structure with fops*/
		cdev_init(&pcdrv_data.pcdev_data[i].cdev , &pcd_fops);
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;

		/*3. Register a device ( cdev structure ) with VFS */
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);
		if(ret < 0)
		{
			pr_info("cdev_add failed\n");
			goto cdev_del;
		}

		/*5. populate the sysfs with device number */
		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number + i,NULL,"pcdev-%d",i+1);
		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_info("device_create failed\n" );
			ret = PTR_ERR(pcdrv_data.device_pcd);
			goto class_destroy;
		}
	}

	return 0;
cdev_del:
class_destroy:
	for( ; i > 0 ; i--)
	{
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chardev:
	unregister_chrdev_region(pcdrv_data.device_number,1);
out:
	return ret;
}

static void __exit pcd_driver_exit(void)
{
	int i = 0;
	for( i = 0; i < NO_OF_DEVICES ; i++)
	{
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);

	unregister_chrdev_region(pcdrv_data.device_number,1);
	pr_info("module unloaded successfully\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KARTHIK KUMAR");
MODULE_DESCRIPTION("A pseudo character driver which handles n devices");
MODULE_INFO(board,"beagle bone");
