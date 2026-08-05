#include<linux/module.h>
#include<linux/platform_device.h>
#include"platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : "fmt,__func__

void pcdev_release (struct device *dev)
{
	pr_info("Device released succesfully\n");
}
/* 1 create 2. platform data */
struct pcdev_platform_data pcdev_data[3] = {
	[0] = {.size = 512 , .perm = RDWR , .serial_number = "PCDEVABC1111"},
	[1] = {.size = 1024 , .perm = RDWR , .serial_number = "PCDEVABC2222"},
	[2] = {.size = 1024 , .perm = RDWR , .serial_number = "PCDEVABC3333"}
};
/* 2. create 2 platfrom device */

struct platform_device platform_pcdev1 = {
	.name = "pcdev-A1x",
	.id =0,
	.dev = 
	{
		.platform_data = &pcdev_data[0] ,
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev2 = {
	.name = "pcdev-B1x",
	.id =1,
	.dev = 
	{
		.platform_data = &pcdev_data[1] ,
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev3 = {
	.name = "pcdev-C1x",
	.id =2,
	.dev = 
	{
		.platform_data = &pcdev_data[2] ,
		.release = pcdev_release
	}
};
struct platform_device *platform_pcdevs[] = 
{
	&platform_pcdev1,
	&platform_pcdev2,
	&platform_pcdev3
};
static int __init pcdev_platform_init(void)
{
	//register platform device
	//platform_device_register(&platform_pcdev1);
	//platform_device_register(&platform_pcdev2);
int ret;	
	/*instead of initializing one by one */
	ret = platform_add_devices(platform_pcdevs,ARRAY_SIZE(platform_pcdevs));
	if (ret) {
        pr_err("Failed to register platform devices: %d\n", ret);
        return ret;
    	}

	pr_info("Device setup module loaded\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev1);
	platform_device_unregister(&platform_pcdev2);
	platform_device_unregister(&platform_pcdev3);
	pr_info("Device setup module unloaded\n");
}
module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("karthik");
MODULE_DESCRIPTION("A kernel module to add platform devices");

