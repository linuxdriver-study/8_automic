#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/ide.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#define DEVICE_CNT            1
#define DEVICE_NAME             "automic"

struct automic_dev_struct {
        dev_t devid;
        int major;
        int minor;
        struct cdev *cdev;
};
static struct automic_dev_struct automic_dev;

struct file_operations ops = {
        //.open = ;
};

static int __init automic_init(void)
{
        int ret = 0;
        if (automic_dev.major) {
                automic_dev.devid = MKDEV(automic_dev.major, automic_dev.minor);
                ret = register_chrdev_region(automic_dev.devid, DEVICE_CNT, DEVICE_NAME);
        } else {
                ret = alloc_chrdev_region(&automic_dev.devid, 0, DEVICE_CNT, DEVICE_NAME);
                automic_dev.major = MAJOR(automic_dev.devid);
                automic_dev.minor = MINOR(automic_dev.devid);
        }
        if (ret < 0) {
                printk("chrdev_region error!\n");
                goto fail_region;
        }
        printk("major:%d minor:%d\n", automic_dev.major, automic_dev.minor);

        cdev_init(automic_dev.cdev, );

fail_region:
        return ret;
}

static void __exit automic_exit(void)
{
        unregister_chrdev_region(automic_dev.devid, DEVICE_CNT);
}

module_init(automic_init);
module_exit(automic_exit);
MODULE_AUTHOR("wanglei");
MODULE_LICENSE("GPL");
