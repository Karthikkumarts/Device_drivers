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
struct pcdev_platform_data pcdev_data[2] = {
	[0] = {.size = 512 , .perm = RDWR , .serial_number = "PCDEVABC1111"},
	[1] = {.size = 1024 , .perm = RDWR , .serial_number = "PCDEVABC2222"}
};
/* 2. create 2 platfrom device */

struct platform_device platform_pcdev1 = {
	.name = "pseudo-char-device",
	.id =0,
	.dev = 
	{
		.platform_data = &pcdev_data[0] ,
		.release = pcdev_release
	}
};

struct platform_device platform_pcdev2 = {
	.name = "pseudo-char-device",
	.id =1,
	.dev = 
	{
		.platform_data = &pcdev_data[1] ,
		.release = pcdev_release
	}
};

static int __init pcdev_platform_init(void)
{
	//register platform device
	platform_device_register(&platform_pcdev1);
	platform_device_register(&platform_pcdev2);
	pr_info("Device setup module loaded\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev1);
	platform_device_unregister(&platform_pcdev2);
	pr_info("Device setup module unloaded\n ");

}
module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("karthik");
MODULE_DESCRIPTION("A kernel module to add platform devices");

