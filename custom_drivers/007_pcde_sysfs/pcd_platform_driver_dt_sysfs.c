#include "pcd_platform_driver_dt_sysfs.h" 

struct device_config pcdev_config[] = {
	[PCDEVA1X] = {.config1 = 60 , .config2 = 20},
	[PCDEVB1X] = {.config1 = 70 , .config2 = 21},
	[PCDEVC1X] = {.config1 = 80 , .config2 = 22}
};
struct pcdrv_private_data pcdrv_data;

/*file operation of the driver*/
struct file_operations pcd_fops =
{
	.open = pcd_open,
	.release = pcd_release,
	.llseek = pcd_llseek,
	.read = pcd_read,
	.write = pcd_write
};
ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char *buf)
{
	/*get access to the device private data */
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	int ret = sprintf(buf,"%d\n",dev_data->pdata.size);
	pr_info("show_serial_number successfull");
	return ret;
	
}
ssize_t show_serial_number(struct device *dev, struct device_attribute *attr, char *buf)
{
	/*get access to the device private data */
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	int ret = sprintf(buf,"%s\n",dev_data->pdata.serial_number);
	pr_info("show_serial_number successfull");
	return ret;
}
ssize_t store_max_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long result;
	int ret;
	ret = kstrtol(buf,10,&result);
	if(ret)
		return ret;
	
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	dev_data->pdata.size = result;
	dev_data->buff = krealloc(dev_data->buff,dev_data->pdata.size,GFP_KERNEL);
	return count;

}

/* create 2 variable of struct device_attribute */
static DEVICE_ATTR(max_size,S_IRUGO|S_IWUSR, show_max_size ,store_max_size);
static DEVICE_ATTR(serial_number , S_IRUGO , show_serial_number , NULL);

struct attribute * pcd_attrs[] =
{
	&dev_attr_max_size.attr,
	&dev_attr_serial_number.attr
};

struct attribute_group pcd_attr_group =
{
	.attrs = pcd_attrs
};
int pcd_sysfs_create_file(struct device *pcd_dev)
{
	int ret ;
#if 0
	ret = sysfs_create_file(&pcd_dev->kobj,& dev_attr_max_size.attr);
	ret = sysfs_create_file(&pcd_dev->kobj,& dev_attr_serial_number.attr);
#endif
	ret =  sysfs_create_group(&pcd_dev->kobj , &pcd_attr_group);
	return ret;
}
struct pcdev_platform_data * pcdev_get_data_from_dt( struct device *dev)
{
	struct device_node * dev_node = dev->of_node;
	struct pcdev_platform_data *pdata;

	if(!dev_node)
	{
		return NULL;
	}

	pdata = devm_kzalloc(dev,sizeof(*pdata),GFP_KERNEL);
	if(!pdata)
	{
		dev_info(dev,"cannot allocate the memory\n");
		return ERR_PTR(-ENOMEM);
	}

	if(of_property_read_string(dev_node, "org,serial_number",&pdata->serial_number))
	{
		dev_info(dev,"Missing serial number\n");
			return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node, "org,perm",&pdata->perm))
	{
		dev_info(dev,"Missing permission\n");
			return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node, "org,size",&pdata->size))
	{
		dev_info(dev,"Missing size\n");
			return ERR_PTR(-EINVAL);
	}

	return pdata;
}
int pcd_platform_driver_probe(struct platform_device * pcdev)
{
	int ret;
	pr_info("A matching device detected\n");
	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data * pdata; //instead of taking extra structure for platfrom_data , first allocate the memory for dev_data and then directly take the dev_get_platdata to dev_data->pdata.

	const struct of_device_id * match;
    int driver_data;    
	/* 1. Get the platform data */

	// start TYPE 1
	pdata = pcdev_get_data_from_dt(&pcdev->dev); //get the data  from device tree node
	if(IS_ERR(pdata))
	{
		return -EINVAL;
	}
	if(!pdata)
	{
		//manual [device_setup]
		//pdata = pcdev->dev.platform_data;
		pdata = (struct pcdev_platform_data *)dev_get_platdata(&pcdev->dev); //returs platform data
		if(!pdata)
		{
			pr_info("NO platform data available\n");
			return -EINVAL;
		}
		driver_data = pcdev->id_entry->driver_data;
	}
	else
	{
		driver_data = (int)of_device_get_match_data(&pcdev->dev);
		//match = of_match_device(pcdev->dev.driver->of_match_table,&pcdev->dev);
		//driver_data = (int)match->data;
	}
	//END TYPE 1

		//Type 2
	/*match = of_match_device(of_match_ptr(org_pcdev_dt_match),dev); // by using this of_match_ptr we can determine the matching is from device tree or not
	if(match)
	{
		pdata = pcdev_get_data_from_dt(&pcdev->dev); //get the data  from device tree node
		if(IS_ERR(pdata))
		{
			return PTR_ERR(-EINVAL);
		}	
		driver_data = (int)match->data;
	}
	else
	{
		pdata = (struct pcdev_platform_data *)dev_get_platdata(&pcdev->dev); //returs platform data
		driver_data = pcdev->id_entry->driver_data;
	}
	if(!pdata)
	{
		dev_info(&pcdev->dev."No platform data available");
		return -EINVAL;
	}*/
	//END TYPE 2

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

	pr_info("config item 1 : %d\n",pcdev_config[driver_data].config1);
	pr_info("config item 2 : %d\n",pcdev_config[driver_data].config2);

	/* 3. Dynamically allocate the memory for device buffer using size information from the platform data */

	dev_data->buff = kzalloc(dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data->buff){
		pr_info("cannot allocate memory for buffer\n");
		return -ENOMEM;
	}

	/*4. Get the device number */
	dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

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
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,&pcdev->dev,dev_data->dev_num,NULL,"pcdev-%d",pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("device creation failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
	        cdev_del(&dev_data->cdev);
	}

	dev_set_drvdata(&pcdev->dev,dev_data);
	pcdrv_data.total_devices++;

	ret = pcd_sysfs_create_file(pcdrv_data.device_pcd);
	if(ret)
	{
		device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);
		return ret;
	}
	dev_info(&pcdev->dev,"Probe was successfull\n");
	return 0;
}

/*void for the host  and int for beagle bone as kernel version are differnet */
//int pcd_platform_driver_remove(struct platform_device * pcdev)
void  pcd_platform_driver_remove(struct platform_device * pcdev)
{
#if 1
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pcdev->dev); 
	/* 1. Remove  a device which was created by device_create */
	device_destroy(pcdrv_data.class_pcd , dev_data->dev_num);

	/*2. remove cdev entry */
	cdev_del(&dev_data->cdev);

	/*3  Free the memory held by the device */
	kfree(dev_data->buff);
	kfree(dev_data);
	
	pcdrv_data.total_devices--;
#endif
	dev_info(&pcdev->dev,"A device removed\n");
//	return 0;
}

struct platform_device_id pcdev_ids[] = {
	[0] = {.name = "pcdev-A1x",.driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x",.driver_data=PCDEVB1X},
	[2] = {.name = "pcdev-C1x",.driver_data=PCDEVC1X}
};

struct of_device_id org_pcdev_dt_match[] =
{
	{.compatible = "pcdev-A1x",.data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x",.data=(void *)PCDEVB1X},
	{.compatible = "pcdev-C1x",.data=(void *)PCDEVC1X},
	{.compatible = "pcdev-D1x",.data=(void *)PCDEVD1X},
	{}

};
struct platform_driver pcd_platform_driver = {

	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdev_ids,
	.driver = {
        	.name = "pcdev",
		.of_match_table = org_pcdev_dt_match
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
	//for BBB
	//pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
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
