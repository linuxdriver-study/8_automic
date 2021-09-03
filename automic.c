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
#include <linux/types.h>

#define LED_ON                  1
#define LED_OFF                 0
#define DEVICE_CNT              1
#define DEVICE_NAME             "automic"

struct automic_dev_struct {
        dev_t devid;
        int major;
        int minor;
        struct cdev cdev;
        struct class *class;
        struct device *device;
        struct device_node *nd;
        int led_gpio;
        atomic_t lock;
};
static struct automic_dev_struct automic_dev = {
        .lock = ATOMIC_INIT(1),
};

static int led_open(struct inode *nd, struct file *file);
static ssize_t led_write(struct file *file,
                          const char __user *user,
                          size_t size,
                          loff_t *loff);
static int led_release(struct inode *nd, struct file *file);
 
struct file_operations ops = {
        .owner = THIS_MODULE,
        .open = led_open,
        .write = led_write,
        .release = led_release,
};

static int led_open(struct inode *nd, struct file *file)
{
        int ret = 0;
        file->private_data = &automic_dev;
        /* 通过原子变量检查是否别的应用在使用此驱动 */
        if (!atomic_dec_and_test(&automic_dev.lock)) {
                atomic_inc(&automic_dev.lock);
                ret = -EBUSY;
        }
        return ret;
}

static ssize_t led_write(struct file *file,
                          const char __user *user,
                          size_t size,
                          loff_t *loff)
{
        struct automic_dev_struct *dev = file->private_data;
        u8 buf[1] = {0};
        int ret = 0;

        ret = copy_from_user(buf, user, 1);
        if (ret != 0) {
                ret = -EFAULT;
                goto error;
        }

        if (buf[0] != LED_OFF && buf[0] != LED_ON) {
                ret = -EFAULT;
                goto error;
        }

        if (buf[0] == LED_OFF) {
                gpio_set_value(dev->led_gpio, 1);
        } else if (buf[0] == LED_ON) {
                gpio_set_value(dev->led_gpio, 0);
        }

error:
        return ret;
}

static int led_release(struct inode *nd, struct file *file)
{
        file->private_data = NULL;
        /* 关闭时释放原子变量 */
        atomic_inc(&automic_dev.lock);
        return 0;
}
 

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

        cdev_init(&automic_dev.cdev, &ops);
        ret = cdev_add(&automic_dev.cdev, automic_dev.devid, DEVICE_CNT);
        if (ret < 0) {
                printk("cdev add error!\n");
                goto fail_cdev_add;
        }

        automic_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
        if (IS_ERR(automic_dev.class)) {
                printk("class create error!\n");
                ret = -EFAULT;
                goto fail_class_create;
        }
        automic_dev.device = device_create(automic_dev.class, NULL, 
                                           automic_dev.devid, NULL,
                                           DEVICE_NAME);
        if (IS_ERR(automic_dev.device)) {
                printk("device create error!\n");
                ret = -EFAULT;
                goto fail_device_create;
        }

        automic_dev.nd = of_find_node_by_path("/gpioled");
        if (automic_dev.nd == NULL) {
                printk("find node by path!\n");
                ret = -EFAULT;
                goto fail_find_node;
        }
        automic_dev.led_gpio = of_get_named_gpio(automic_dev.nd, "led-gpios", 0);
        if (automic_dev.led_gpio < 0) {
                printk("get named error!\n");
                ret = -EFAULT;
                goto fail_get_named;
        }
        printk("led_gpio num: %d\n", automic_dev.led_gpio);
        ret = gpio_request(automic_dev.led_gpio, "led_gpio");
        if (ret != 0) {
                printk("gpio request error!\n");
                goto fail_gpio_request;
        }
        ret = gpio_direction_output(automic_dev.led_gpio, 1);
        if (ret != 0) {
                printk("gpio dir output set error!\n");
                goto fail_gpio_dir;
        }
        gpio_set_value(automic_dev.led_gpio, 1);

        //atomic_set(&automic_dev.lock, 1); /* 初始化原子变量为1 */

        goto success;

fail_gpio_dir:
        gpio_free(automic_dev.led_gpio);
fail_gpio_request:
fail_get_named:
fail_find_node:
        device_destroy(automic_dev.class, automic_dev.devid);
fail_device_create:
        class_destroy(automic_dev.class);
fail_class_create:
        cdev_del(&automic_dev.cdev);
fail_cdev_add:
        unregister_chrdev_region(automic_dev.devid, DEVICE_CNT);
fail_region:
success:
        return ret;
}

static void __exit automic_exit(void)
{
        gpio_set_value(automic_dev.led_gpio, 1);
        gpio_free(automic_dev.led_gpio);
        device_destroy(automic_dev.class, automic_dev.devid);
        class_destroy(automic_dev.class);
        cdev_del(&automic_dev.cdev);
        unregister_chrdev_region(automic_dev.devid, DEVICE_CNT);
}

module_init(automic_init);
module_exit(automic_exit);
MODULE_AUTHOR("wanglei");
MODULE_LICENSE("GPL");
