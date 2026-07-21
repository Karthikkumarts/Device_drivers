#include<linux/module.h>

static int __init my_kernel_init(void )
{
	pr_info("hello world this is karthik\n");
	return 0;
}	

static void __exit my_kernel_exit(void)
{
	pr_info("Good bye world\n");
}

module_init(my_kernel_init);
module_exit(my_kernel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KARTHIK KUMAR TS");
MODULE_DESCRIPTION("Printing a simple message for the first time\n");
MODULE_INFO(board,"beagle bone black rev D");
