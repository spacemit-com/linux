// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * Copyright (C) 2023-2025 SpacemiT (Hangzhou) Technology Co. Ltd
 * Copyright (c) 2025 Yixun Lan <dlan@gentoo.org>
 */

#include <linux/io.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio/driver.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/seq_file.h>
#include <linux/module.h>

/* register offset */
#define GPLR		0x00
#define GPDR		0x0c
#define GPSR		0x18
#define GPCR		0x24
#define GRER		0x30
#define GFER		0x3c
#define GEDR		0x48
#define GSDR		0x54
#define GCDR		0x60
#define GSRER		0x6c
#define GCRER		0x78
#define GSFER		0x84
#define GCFER		0x90
#define GAPMASK		0x9c
#define GCPMASK		0xa8

#define K1_BANK_GPIO_NUMBER	(32)

#define to_spacemit_gpio_port(x) container_of((x), struct spacemit_gpio_port, gc)

struct spacemit_gpio;

struct spacemit_gpio_port {
	struct gpio_chip		gc;
	struct spacemit_gpio		*gpio;
	struct fwnode_handle		*fwnode;
	void __iomem			*base;
	int				irq;
	u32				irq_mask;
	u32				irq_rising_edge;
	u32				irq_falling_edge;
	u32				index;
};

struct spacemit_gpio {
	struct	device			*dev;
	struct spacemit_gpio_port	*ports;
	u32				nr_ports;
};

static inline void spacemit_clear_edge_detection(struct spacemit_gpio_port *port, u32 bit)
{
	writel(bit, port->base + GCRER);
	writel(bit, port->base + GCFER);
}

static inline void spacemit_set_edge_detection(struct spacemit_gpio_port *port, u32 bit)
{
	writel(bit & port->irq_rising_edge,  port->base + GSRER);
	writel(bit & port->irq_falling_edge, port->base + GSFER);
}

static inline void spacemit_reset_edge_detection(struct spacemit_gpio_port *port)
{
	writel(0xffffffff, port->base + GCFER);
	writel(0xffffffff, port->base + GCRER);
	writel(0xffffffff, port->base + GAPMASK);
}

static int spacemit_gpio_irq_type(struct irq_data *d, unsigned int type)
{
	struct spacemit_gpio_port *port = irq_data_get_irq_chip_data(d);
	u32 bit = BIT(irqd_to_hwirq(d));

	if (type & IRQ_TYPE_EDGE_RISING) {
		port->irq_rising_edge |= bit;
		writel(bit, port->base + GSRER);
	} else {
		port->irq_rising_edge &= ~bit;
		writel(bit, port->base + GCRER);
	}

	if (type & IRQ_TYPE_EDGE_FALLING) {
		port->irq_falling_edge |= bit;
		writel(bit, port->base + GSFER);
	} else {
		port->irq_falling_edge &= ~bit;
		writel(bit, port->base + GCFER);
	}

	return 0;
}

static irqreturn_t spacemit_gpio_irq_handler(int irq, void *dev_id)
{
	struct spacemit_gpio_port *port = dev_id;
	unsigned long pending;
	u32 n, gedr;

	gedr = readl(port->base + GEDR);
	if (!gedr)
		return IRQ_NONE;

	writel(gedr, port->base + GEDR);
	gedr = gedr & port->irq_mask;

	if (!gedr)
		return IRQ_NONE;

	pending = gedr;

	for_each_set_bit(n, &pending, BITS_PER_LONG)
		handle_nested_irq(irq_find_mapping(port->gc.irq.domain, n));

	return IRQ_HANDLED;
}

static void spacemit_ack_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_port *port = irq_data_get_irq_chip_data(d);

	writel(BIT(irqd_to_hwirq(d)), port->base + GEDR);
}

static void spacemit_mask_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_port *port = irq_data_get_irq_chip_data(d);
	u32 bit = BIT(irqd_to_hwirq(d));

	port->irq_mask &= ~bit;

	spacemit_clear_edge_detection(port, bit);
}

static void spacemit_unmask_muxed_gpio(struct irq_data *d)
{
	struct spacemit_gpio_port *port = irq_data_get_irq_chip_data(d);
	u32 bit = BIT(irqd_to_hwirq(d));

	port->irq_mask |= bit;

	spacemit_set_edge_detection(port, bit);
}

static void spacemit_irq_print_chip(struct irq_data *data, struct seq_file *p)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(data);
	struct spacemit_gpio_port *port = to_spacemit_gpio_port(gc);

	seq_printf(p, "%s-%d", dev_name(gc->parent), port->index);
}

static struct irq_chip spacemit_muxed_gpio_chip = {
	.name		= "k1-gpio-irqchip",
	.irq_ack	= spacemit_ack_muxed_gpio,
	.irq_mask	= spacemit_mask_muxed_gpio,
	.irq_unmask	= spacemit_unmask_muxed_gpio,
	.irq_set_type	= spacemit_gpio_irq_type,
	.irq_print_chip	= spacemit_irq_print_chip,
	.flags		= IRQCHIP_IMMUTABLE,
	GPIOCHIP_IRQ_RESOURCE_HELPERS,
};

static int spacemit_gpio_get_ports(struct device *dev, struct spacemit_gpio *gpio,
				   void __iomem *regs)
{
	struct spacemit_gpio_port *port;
	u32 i = 0, offset;

	gpio->nr_ports = device_get_child_node_count(dev);
	if (gpio->nr_ports == 0)
		return -ENODEV;

	gpio->ports = devm_kcalloc(dev, gpio->nr_ports, sizeof(*gpio->ports), GFP_KERNEL);
	if (!gpio->ports)
		return -ENOMEM;

	device_for_each_child_node_scoped(dev, fwnode)  {
		port		= &gpio->ports[i];
		port->fwnode	= fwnode;
		port->index	= i++;

		if (fwnode_property_read_u32(fwnode, "reg", &offset))
			return -EINVAL;

		port->base	= regs + offset;
		port->irq	= fwnode_irq_get(fwnode, 0);
	}

	return 0;
}

static int spacemit_gpio_add_port(struct spacemit_gpio *gpio, int index)
{
	struct spacemit_gpio_port *port;
	struct device *dev = gpio->dev;
	struct gpio_irq_chip *girq;
	void __iomem *dat, *set, *clr, *dirin, *dirout;
	int ret;

	port = &gpio->ports[index];
	port->gpio = gpio;

	dat	= port->base + GPLR;
	set	= port->base + GPSR;
	clr	= port->base + GPCR;
	dirin	= port->base + GCDR;
	dirout	= port->base + GSDR;

	/* This registers 32 GPIO lines per port */
	ret = bgpio_init(&port->gc, dev, 4, dat, set, clr, dirout, dirin,
			 BGPIOF_UNREADABLE_REG_SET | BGPIOF_UNREADABLE_REG_DIR);
	if (ret)
		return dev_err_probe(dev, ret, "failed to init gpio chip for port\n");

	port->gc.label		= dev_name(dev);
	port->gc.fwnode		= port->fwnode;
	port->gc.request	= gpiochip_generic_request;
	port->gc.free		= gpiochip_generic_free;
	port->gc.ngpio		= K1_BANK_GPIO_NUMBER;
	port->gc.base		= -1;

	girq			= &port->gc.irq;
	girq->threaded		= true;
	girq->handler		= handle_simple_irq;

	gpio_irq_chip_set_chip(girq, &spacemit_muxed_gpio_chip);

	spacemit_reset_edge_detection(port);

	ret = devm_request_threaded_irq(dev, port->irq, NULL,
					spacemit_gpio_irq_handler,
					IRQF_ONESHOT | IRQF_SHARED,
					port->gc.label, port);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to request IRQ\n");

	return devm_gpiochip_add_data(dev, &port->gc, port);
}

static int spacemit_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spacemit_gpio *gpio;
	struct resource *res;
	void __iomem *regs;
	int i, ret;

	gpio = devm_kzalloc(dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	gpio->dev = dev;

	regs = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	ret = spacemit_gpio_get_ports(dev, gpio, regs);
	if (ret)
		return dev_err_probe(dev, ret, "fail to get gpio ports\n");

	for (i = 0; i < gpio->nr_ports; i++) {
		ret = spacemit_gpio_add_port(gpio, i);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct of_device_id spacemit_gpio_dt_ids[] = {
	{ .compatible = "spacemit,k1-gpio" },
	{ /* sentinel */ }
};

static struct platform_driver spacemit_gpio_driver = {
	.probe		= spacemit_gpio_probe,
	.driver		= {
		.name	= "k1-gpio",
		.of_match_table = spacemit_gpio_dt_ids,
	},
};
module_platform_driver(spacemit_gpio_driver);

MODULE_AUTHOR("Yixun Lan <dlan@gentoo.org>");
MODULE_DESCRIPTION("GPIO driver for SpacemiT K1 SoC");
MODULE_LICENSE("GPL");
