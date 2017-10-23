#include <linux/init.h>
#include <linux/module.h>

static int my_init(void)
{
 return  0;
}

static void my_exit(void)
{
 return;
}

static struct veml7700 my_match_table[] = {
     {
             .compatible = "veml77000",
     },
     {0}
};
MODULE_DEVICE_TABLE(of, my_match_table);

static struct platform_driver my_platform_driver = {
        .probe = my_probe,
        .remove = my_remove,
        .driver = {
                .name = "veml7700",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(my_match_table),
        },
};

static int my_probe(struct platform_device *dev)
{
 return 0;
}

module_init(my_init);
module_exit(my_exit);
