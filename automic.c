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

#define DEVICE_CNT              1
#define DEVICE_NAME             "automic"

struct automic_dev_struct {
        dev_t devid;
        int major;
        int minor;
        struct cdev *cdev;
        struct class *class;
        struct device *device;
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

        cdev_init(automic_dev.cdev, &ops);
        ret = cdev_add(automic_dev.cdev, automic_dev.devid, DEVICE_CNT);
        if (ret < 0) {
                printk("cdev add error!\n");
                goto fail_cdev_add;
        }

        automic_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
        if (IS_ERR(automic_dev.class)) {
                printk("class create error!\n");
                goto fail_class_create;
        }
        automic_dev.device = device_create(automic_dev.class, NULL, 
                                           automic_dev.devid, NULL, DEVICE_NAME);
        if (IS_ERR(automic_dev.class)) {
                printk("device create error!\n");
                goto fail_device_create;
        }

        goto success;

fail_device_create:
        class_destroy(automic_dev.class);
fail_class_create:
        cdev_del(automic_dev.cdev);
fail_cdev_add:
        unregister_chrdev_region(automic_dev.devid, DEVICE_CNT);
fail_region:
success:
        return ret;
}

static void __exit automic_exit(void)
{
        device_destroy(automic_dev.class, automic_dev.devid);
        class_destroy(automic_dev.class);
        cdev_del(automic_dev.cdev);
        unregister_chrdev_region(automic_dev.devid, DEVICE_CNT);
}

module_init(automic_init);
module_exit(automic_exit);
MODULE_AUTHOR("wanglei");
MODULE_LICENSE("GPL");
