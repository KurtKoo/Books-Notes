&gpio {
    spi0_pins: spi0_pins {
        brcm,pins = <9 10 11>;
        brcm,function = <4>; /* alt0 */
    };
    [...]
    key_pin: key_pin {
        brcm,pins = <23>;
        brcm,function = <0>; /* Input */
        brcm,pull = <1>; /* Pull down */
    };
    
};
&soc {
    virtgpio: virtgpio {
        compatible = "brcm,bcm2835-virtgpio";
        gpio-controller;
        #gpio-cells = <2>;
        firmware = <&firmware>;
        status = "okay";
    };
    [...]
    int_key {
        compatible = "arrow,intkey";    //与对应platform_driver匹配
        pinctrl-names = "default";
        pinctrl-0 = <&key_pin>;
        gpios = <&gpio 23 0>;   //与gpio23绑定
        interrupts = <23 1>; 
        interrupt-parent = <&gpio>;
    };
};
// 以上为设备树定义

#include <linux/module.h>
#include <linux/platform_device.h> 
#include <linux/interrupt.h> 
#include <linux/gpio/consumer.h>
#include <linux/miscdevice.h>
static char *HELLO_KEYS_NAME = "PB_KEY";
/* Interrupt handler */
static irqreturn_t hello_keys_isr(int irq, void *data)
{
    struct device *dev = data;
    dev_info(dev, "interrupt received. key: %s\n", HELLO_KEYS_NAME);
    return IRQ_HANDLED;
}
static struct miscdevice helloworld_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mydev",
};
static int __init my_probe(struct platform_device *pdev)
{
    int ret_val, irq;
    struct gpio_desc *gpio;
    struct device *dev = &pdev->dev;
    dev_info(dev, "my_probe() function is called.\n");
    /* First method to get the virtual linux IRQ number */
    gpio = devm_gpiod_get(dev, NULL, GPIOD_IN); //获取一个gpio_desc
    if (IS_ERR(gpio)) {
        dev_err(dev, "gpio get failed\n");
        return PTR_ERR(gpio);
    }
    irq = gpiod_to_irq(gpio);   //由于该gpio有中断，获取中断号
    if (irq < 0)
    return irq;
    dev_info(dev, "The IRQ number is: %d\n", irq);
    /* Second method to get the virtual Linux IRQ number */
    irq = platform_get_irq(pdev, 0);    //由于设备树匹配了对应节点，probe函数传入platform_device，也可基于platform方式获取中断号
    if (irq < 0){
        dev_err(dev, "irq is not available\n");
        return -EINVAL;
    }
    dev_info(dev, "IRQ_using_platform_get_irq: %d\n", irq);
    /* Allocate the interrupt line */
    ret_val = devm_request_irq(dev, irq, hello_keys_isr, 
    IRQF_TRIGGER_FALLING, HELLO_KEYS_NAME, dev);    //为device绑定中断和中断服务函数
    if (ret_val) {
        dev_err(dev, "Failed to request interrupt %d, error %d\n", irq, ret_val);
        return ret_val;
    }
    ret_val = misc_register(&helloworld_miscdevice);
    if (ret_val != 0)
    {
        dev_err(dev, "could not register the misc device mydev\n");
        return ret_val;
    }
    dev_info(dev, "mydev: got minor %i\n",helloworld_miscdevice.minor);
    dev_info(dev, "my_probe() function is exited.\n");
    return 0;
}
static int __exit my_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "my_remove() function is called.\n");
    misc_deregister(&helloworld_miscdevice);
    dev_info(&pdev->dev, "my_remove() function is exited.\n");
    return 0;
}
static const struct of_device_id my_of_ids[] = {
    { .compatible = "arrow,intkey"},
    {},
};
MODULE_DEVICE_TABLE(of, my_of_ids);
static struct platform_driver my_platform_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "intkey",
        .of_match_table = my_of_ids,
        .owner = THIS_MODULE,
    }
};
module_platform_driver(my_platform_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("This is a button INT platform driver");

