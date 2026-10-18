#include "pcd_platform_driver_dt_sysfs.h"
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

