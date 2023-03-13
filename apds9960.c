#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/of_device.h>
#include <linux/mod_devicetable.h>

#include <linux/i2c.h>
#include <linux/wait.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>


#define APDS9960_DEVICENAME		"apds9960"
#define APDS9960_COMPATIBLE		"globallogic, apds9960"
#define APDS9960_DEVICE_ID		(0xAB)

#define APDS9960_REG_SIZE		(8)
#define APDS9960_REG_BYTE		(8)

#define APDS9960_REG_RANGE_RW1_START	(0x00)
#define APDS9960_REG_RANGE_RW1_END	(0x90)
#define APDS9960_REG_RANGE_RW2_START	(0x9D)
#define APDS9960_REG_RANGE_RW2_END	(0xAB)

#define APDS9960_REG_RANGE_R1_START	(0x92)
#define APDS9960_REG_RANGE_R1_END	(0x9C)
#define APDS9960_REG_RANGE_R2_START	(0xAE)
#define APDS9960_REG_RANGE_R2_END	(0xAF)
#define APDS9960_REG_RANGE_R3_START	(0xFC)
#define APDS9960_REG_RANGE_R3_END	(0xFF)

#define APDS9960_REG_RANGE_W_START	(0xE4)
#define APDS9960_REG_RANGE_W_END	(0xE7)
#define APDS9960_REG_MAX		(0xFF)

			/* APDS9960 Register Set */
#define APDS9960_RAM_START	(0x00)
#define APDS9960_RAM_END	(0x7F)
#define APDS9960_REG_ENABLE	(0x80) /* enable states and interrupts */
#define APDS9960_REG_ATIME	(0x81) /* adc integration time */
#define APDS9960_REG_WTIME	(0x83) /* wait time (non-gesture) */

#define APDS9960_REG_AILTL	(0x84) /* ALS int low threshold low byte*/
#define APDS9960_REG_AILTH	(0x85) /* ALS int low threshold high byte*/
#define APDS9960_REG_AIHTL	(0x86) /* ALS int hight threshold low byte */
#define APDS9960_REG_AIHTH	(0x87) /* ALS int hight threshold hight byte*/

#define APDS9960_REG_PILT	(0x89) /* proximity int low threshold */
#define APDS9960_REG_PIHT	(0x8B) /* proximity int hight threshold */
#define APDS9960_REG_PERS	(0x8C) /* int persistence filters non-gesture*/

#define APDS9960_REG_CONFIG1	(0x8D) /* configuration register one */
#define APDS9960_REG_CONFIG2	(0x90) /* configuration register two */
#define APDS9960_REG_CONFIG3	(0x9F) /* configuration register three */

#define APDS9960_REG_PPULSE	(0x8E) /* proximity pulse count and lenght */
#define APDS9960_REG_CONTROL	(0x8F) /* gain control */

#define APDS9960_REG_ID		(0x92) /* Device ID */
#define APDS9960_REG_STATUS	(0x93) /* Device status */
#define APDS9960_REG_CDATAL	(0x94) /* lowbyte of clear channel data */
#define APDS9960_REG_CDATAH	(0x95) /* highbyte of clear channel data */
#define APDS9960_REG_RDATAL	(0x96) /* lowbyte of red channel data */
#define APDS9960_REG_RDATAH	(0x97) /* highbyte of red channel data */
#define APDS9960_REG_GDATAL	(0x98) /* lowbyte of green channel data */
#define APDS9960_REG_GDATAH	(0x99) /* highbyte of green channel data */
#define APDS9960_REG_BDATAL	(0x9A) /* lowbyte of blue channel data */
#define APDS9960_REG_BDATAH	(0x9B) /* highbyte of blue channel data */
#define APDS9960_PDATA		(0x9C) /* proximity data */

#define APDS9960_REG_POFFSET_UR	(0x9D) /* prox offset for UP and RIGHT VD */
#define APDS9960_REG_POFFSET_DL (0x9E) /* prox offset for DOWN and LEFT VD */
#define APDS9960_REG_GPENTH	(0xA0) /* gesture proximity enter threshold */
#define APDS9960_REG_GEXTH	(0xA1) /* gesture exit threshold */

#define APDS9960_REG_GCONF1	(0xA2) /* gesture configuration one */
#define APDS9960_REG_GCONF2	(0xA3) /* gesture configuration two */
#define APDS9960_REG_GCONF3	(0xAA) /* gesture configuration three */
#define APDS9960_REG_GCONF4	(0xAB) /* gestre configuration four */

#define APDS9960_REG_GOFFSET_U	(0xA4) /* gesture UP offset register */
#define APDS9960_REG_GOFFSET_D	(0xA5) /* gesture DOWN offset register */
#define APDS9960_REG_GOFFSET_L	(0xA7) /* gesture LEFT offset register */
#define APDS9960_REG_GOFFSET_R	(0xA9) /* gesture RIGHT offset register */
#define APDS9960_REG_GPULSE	(0xA6) /* gesture pulse count and lenght */
#define APDS9960_REG_GFLVL	(0xAE) /* gesture FIFO level */
#define APDS9960_REG_GSTATUS	(0xAF) /* gesture status */

		/* only write registers, special i2c address accessing */
#define APDS9960_REG_IFORCE	(0xE4) /* force interrupt */
#define APDS9960_REG_PICLEAR	(0xE5) /* proximity interrupt clear */
#define APDS9960_REG_CICLEAR	(0xE6) /* ALS clear channel interrupts clear */
#define APDS9960_REG_AICLEAR	(0xE7) /* all non-gesture interrupts clear */

#define APDS9960_REG_GFIFO_U	(0xFC)	/* gesture FIFO UP value */
#define APDS9960_REG_GFIFO_D	(0xFD)	/* gesture FIFO DOWN value */
#define APDS9960_REG_GFIFO_L	(0xFE)	/* gesture FIFO LEFT value */
#define APDS9960_REG_GFIFO_R	(0xFF)	/* gesture FIFO RIGHT value */

/*		DEVICE TREE-OVERLAY DESCRIPTION
{
	compatible = "brcm, bcm2835";
	part_number = "globallogic-gestrure-apds9960";
	version = "A1";

	fragment@0 {
		target = <&i2c1>;
		__overlay__ {
			#address-cells = <1>;
			#size-cells = <0>;
			#interrupt-cells = <2>;
			status = "okay";

			apds9960: apds9960@39 {
				compatible = "globallogic, apds9960";
				reg = <0x39>;
				int-gpio = <&gpio 26 0>;
				interrupt-parent = <&gpio>;
				interrupts = <26 1>;
				// wakeup-source; //
				status = "okay";
			};
		};
	};
};
*/
static struct apds9960_device {
	struct device		*device;
	struct mutex		mutex;

	struct regmap		*regmap;

	int			gesture_int;
	int			gesture_mode_on;
};


#ifdef CONFIG_SYSFS

static ssize_t apds9960_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9960_device *apds9960 = dev_get_drvdata(dev);
	unsigned int data;
	int len = 0;

	if(mutex_lock_killable(&apds9960->mutex))
		return -EINTR;

	if(strcmp(attr->attr.name, "id") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_ID, &data);
		pr_err("apds9960: Device id: %x\n", data);
	}
	else if(strcmp(attr->attr.name, "status") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_STATUS, &data);
		pr_err("apds9960: Status: %x\n", data);
	}
	else if(strcmp(attr->attr.name, "adc_itime") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_ATIME, &data);
		pr_err("apds9960: ADC Integration tome: %x\n", data);
	}
	else if(strcmp(attr->attr.name, "wait_time") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_WTIME, &data);
		pr_err("apds9960: wait time: %x\n", data);
	}

	mutex_unlock(&apds9960->mutex);

	return len;
}

static ssize_t apds9960_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct apds9960_device *apds9960 = dev_get_drvdata(dev);

	if(mutex_lock_killable(&apds9960->mutex))
		return -EINTR;

	mutex_unlock(&apds9960->mutex);

	return count;
}

static DEVICE_ATTR(id, S_IRUGO, apds9960_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, apds9960_show, NULL);
static DEVICE_ATTR(adc_itime, S_IRUGO | S_IWUSR, apds9960_show, apds9960_store);
static DEVICE_ATTR(wait_time, S_IRUGO | S_IWUSR, apds9960_show, apds9960_store);

static struct attribute *apds9960_attrs[] = {
	&dev_attr_id.attr,
	&dev_attr_status.attr,
	&dev_attr_adc_itime.attr,
	&dev_attr_wait_time.attr,
	NULL
};

static struct attribute_group apds9960_group = {
	.name = "configuration",
	.attrs = apds9960_attrs,
};

#endif /* CONFIG_SYSFS */


static const struct regmap_range apds9960_regrange[] = {
	{
		.range_min = APDS9960_REG_RANGE_RW1_START,
		.range_max = APDS9960_REG_RANGE_RW1_END,
	}, {
		.range_min = APDS9960_REG_RANGE_RW2_START,
		.range_max = APDS9960_REG_RANGE_RW2_END,
	}, {
		.range_min = APDS9960_REG_RANGE_R1_START,
		.range_max = APDS9960_REG_RANGE_R1_END,
	}, {
		.range_min = APDS9960_REG_RANGE_R2_START,
		.range_max = APDS9960_REG_RANGE_R2_END,
	}, {
		.range_min = APDS9960_REG_RANGE_R3_START,
		.range_max = APDS9960_REG_RANGE_R3_END,
	}, {
		.range_min = APDS9960_REG_RANGE_W_START,
		.range_max = APDS9960_REG_RANGE_W_END,
	},
};

static const struct reg_default apds9960_regdefault[] = {
	{ APDS9960_REG_ATIME, 0xFF},
	{ APDS9960_REG_WTIME, 0xFF},
};

static bool ds3231_writeable_reg(struct device *dev, unsigned int reg)
{
	if(reg >= APDS9960_REG_RANGE_RW1_START &&
			reg <= APDS9960_REG_RANGE_RW1_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_RW2_START &&
			reg <= APDS9960_REG_RANGE_RW2_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_W_START &&
			reg <= APDS9960_REG_RANGE_W_END)
		return true;

	return false;
}

static bool ds3231_readable_reg(struct device *dev, unsigned int reg)
{
	if(reg >= APDS9960_REG_RANGE_RW1_START &&
			reg <= APDS9960_REG_RANGE_RW1_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_RW2_START &&
			reg <= APDS9960_REG_RANGE_RW2_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_R1_START &&
			reg <= APDS9960_REG_RANGE_R1_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_R2_START &&
			reg <= APDS9960_REG_RANGE_R2_END)
		return true;
	if(reg >= APDS9960_REG_RANGE_R3_START &&
			reg <= APDS9960_REG_RANGE_R3_END)
		return true;

	return false;
}


/* TODO: complete irq routine after register configuration */
static irqreturn_t apds9960_isr(int irq, void *data)
{
	struct apds9960_device *apds9960 = data;

	pr_err("apds9960: in irq! gesture: %d\n", apds9960->gesture_mode_on);

	return IRQ_HANDLED;
}


/* TODO: config registers for gesture mode opeartion with interrup */
static int
apds9960_register_configuration(struct apds9960_device *apds9960)
{
	int err = 0;

	pr_err("apds9960: register configuration for gesture operations\n");

	return err;
}


static int
apds9960_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct regmap_config apds9960_cfg;
	struct apds9960_device *apds9960 = NULL;
	unsigned int data;
	int err = 0;

	pr_err("apds9960: probing the i2c device...\n");

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
		pr_err("apds9960: i2c adapter check failed!\n");
		return -EIO;
	}

	/* setting up the regmap configuration for apds9960 device */
	memset(&apds9960_cfg, 0, sizeof(apds9960_cfg));
	apds9960_cfg.reg_bits = APDS9960_REG_SIZE;
	apds9960_cfg.val_bits = APDS9960_REG_BYTE;
	apds9960_cfg.max_register = APDS9960_REG_MAX;

	apds9960_cfg.writeable_reg = ds3231_writeable_reg;
	apds9960_cfg.readable_reg = ds3231_readable_reg;
	apds9960_cfg.reg_defaults = apds9960_regdefault;
	apds9960_cfg.num_reg_defaults = ARRAY_SIZE(apds9960_regdefault);

	/*TODO: using spinlock (.fast_io) = true instead of
	 * mutex (.fast_io = false) results in an BUG:
	 * scheduling while in atomic-context with regmap_read
	 * or regmap_write functions
	 * */
	apds9960_cfg.fast_io = false;

	/* apds9960 data structure allocation with i2c interface */
	apds9960 = kzalloc(sizeof(struct apds9960_device), GFP_KERNEL);
	if(apds9960 == NULL){
		pr_err("apds9960: ERROR! Can`t allocate device!\n");
		return -ENOMEM;
	}

	mutex_init(&apds9960->mutex);
	apds9960->device = root_device_register(APDS9960_DEVICENAME);

	i2c_set_clientdata(client, apds9960);
	dev_set_drvdata(apds9960->device, apds9960);

	err = sysfs_create_group(&apds9960->device->kobj, &apds9960_group);
	if(err){
		pr_err("apds9960: ERROR! Can`t create sysfs attributes!\n");
		goto fail;
	}

	/* ERR may arrive here: unknown symbol type -> modprobe regmap-i2c */
	apds9960->regmap = devm_regmap_init_i2c(client, &apds9960_cfg);
	if(IS_ERR(apds9960->regmap)){
		pr_err("apds9960: ERROR! Can`t initialize regmap for i2c!\n");
		return PTR_ERR(apds9960->regmap);
	}

	/*TODO: apds9960 interrupt configuration */
	if(client->irq <= 0)
		pr_err("apds9960: no valid irq defined!\n");
	else
		pr_err("apds990: irq is: %d\n", client->irq);

	err = devm_request_irq(&client->dev, client->irq,
			apds9960_isr, IRQF_TRIGGER_FALLING,
			"apds9960_interrupt", apds9960);
	if(err){
		pr_err("apds9960: ERR! Can`t request irq (%d)!\n", client->irq);
		goto fail;
	}
	else
		pr_err("apds9960: irq request - success!\n");


	regmap_read(apds9960->regmap, APDS9960_REG_ID, &data);
	pr_err("apds9960: Device id: 0x%02x", data);

	pr_err("apds9960: probing the device - done!\n");
	pr_info("apds9960: client address:0x%02x. Data:%p\n",
			client->addr, apds9960);


	return 0;

fail:
	if(apds9960->device)
		root_device_unregister(apds9960->device);

	mutex_destroy(&apds9960->mutex);
	kfree(apds9960);
	return err;
}


static int
apds9960_remove(struct i2c_client *client)
{
	struct apds9960_device *apds9960 = i2c_get_clientdata(client);

	pr_err("apds9960: removing the device...\n");

	if(apds9960->device){
		root_device_unregister(apds9960->device);
	}

	mutex_destroy(&apds9960->mutex);
	kfree(apds9960);

	pr_err("apds9960: removing the device - done!\n");

	return 0;
}


static const struct of_device_id apds9960_ids[] = {
	{ .compatible = APDS9960_COMPATIBLE, },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, apds9960_ids);


static struct i2c_device_id apds9960_id[] = {
	{ APDS9960_DEVICENAME, 0},
	{ /* sentinel */},
};
MODULE_DEVICE_TABLE(i2c, apds9960_id);


static struct i2c_driver apds9960_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = APDS9960_DEVICENAME,
		.of_match_table = of_match_ptr(apds9960_ids),
	},
	.probe = apds9960_probe,
	.remove = apds9960_remove,
	.id_table = apds9960_id,
};

module_i2c_driver(apds9960_i2c_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksii Nedopytalskyi");
MODULE_DESCRIPTION("APDS9960 i2c-module for gesture sensing with regmap API");
/* END OF FILE */
