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

#include "apds9960.h"

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

#define APDS9960_REG_ENA_GEN		(0x40) /* gesture enable mask */
#define APDS9960_REG_ENA_PIEN		(0x20) /* prox interrupt enable mask */
#define APDS9960_REG_ENA_PEN		(0x04) /* prox detection enable mask */
#define APDS9960_REG_ENA_PON		(0x01) /* apds9960 power on  */

#define APDS9960_REG_CFG1_WAIT		(0x60) /* CONFIG1 register wait long */

#define APDS9960_REG_PPULSE_8US		(0x40) /* prox pulse 8 us, out 1 cnt */

#define APDS9960_REG_CTRL_IVD		(0x40) /* 50 mA LED Drive Strength */
#define APDS9960_REG_CTRL_PGAIN		(0x04) /* proximity gain x2 */
#define APDS9960_REG_CTRL_AGAIN		(0x01) /* ALS and Color gain x4 */

#define APDS9960_REG_GCONF1_GFIFOTH_4	(0x40)	/* interrupt after 4 datasets */
#define APDS9960_REG_GCONF2_GGAIN_8	(0xc0) /* gesture gain x8 */
#define APDS9960_REG_GCONF2_IVD_MAX	(0x18) /* max current strength for leds */
#define APDS9960_REG_GCONF3_UDLR	(0x03) /* up-down left-right fifo data */
#define APDS9960_REG_GCONF4_GIEN	(0x02) /* gesture interrupt enable */
#define APDS9960_REG_GCONF4_GMOD	(0x01) /* gesture mode state */

#define APDS9960_REG_GSTATUS_GVALID	(0x01) /* check for valid data in fifo */

#define APDS9960_REG_STATUS_PINT	(0x20) /* proximity interupt mask */
#define APDS9960_REG_STATUS_AINT	(0x10) /* als interrupt mask */
#define APDS9960_REG_STATUS_GINT	(0x04) /* gesture interrupt mask */

#define APDS9960_REG_GPENTH_DEFAULT	(0x50) /* gesture 'in' dflt threshold */
#define APDS9960_REG_GEXTH_DEFAULT	(0x40) /* gesture 'out' dflt threshold */

static struct apds9960_device {
	struct device		*device;
	/* struct gpio_desc	*irqpin; */
	/* int			gesture_int; */
	struct mutex		mutex;

	struct regmap		*regmap;

	int			gesture_mode_on;
	u8			gbuffer[4];
};

static int
apds9960_set_power_mode(struct apds9960_device *apds9960, bool  mode)
{
	int err = 0;

	err = regmap_update_bits(apds9960->regmap, APDS9960_REG_ENABLE, 1, mode);

	return err;
}

static int
apds9960_set_integration_time(struct apds9960_device *apds9960, unsigned int tim)
{
	return regmap_write(apds9960->regmap, APDS9960_REG_ATIME, tim);
}

static int
apds9960_set_waiting_time(struct apds9960_device *apds9960, unsigned int tim)
{
	return regmap_write(apds9960->regmap, APDS9960_REG_WTIME, tim);
}

static int
apds9960_get_prx_data(struct apds9960_device *apds9960, unsigned int *data)
{
	return regmap_read(apds9960->regmap, APDS9960_REG_PDATA, data);
}

static inline int
apds9960_get_gfifo_lvl(struct apds9960_device *apds9960)
{
	int level, err;

	err = regmap_read(apds9960->regmap, APDS9960_REG_GFLVL, &level);
	if(err)
		return err;
	else
		return level;
}

static int
apds9960_get_status(struct apds9960_device *apds9960, unsigned int *status)
{
	return regmap_read(apds9960->regmap, APDS9960_REG_STATUS, status);
}

static int
apds9960_get_gstatus(struct apds9960_device *apds9960, unsigned int *gstat)
{
	return regmap_read(apds9960->regmap, APDS9960_REG_GSTATUS, gstat);
}

static bool
apds9960_gesture_is_valid(struct apds9960_device *apds9960)
{
	unsigned int tmp;
	apds9960_get_gstatus(apds9960, &tmp);
	if(tmp & APDS9960_REG_GSTATUS_GVALID)
		return true;
	else
		return false;
}

static int
apds9960_get_gfifo_data(struct apds9960_device *apds9960)
{
	int err = 0, gfifo_lvl = 0;
	mutex_lock(&apds9960->mutex);
	apds9960->gesture_mode_on = 1;

	if(apds9960_gesture_is_valid(apds9960) == false){
		pr_err("+++ isr: gesture data validation failed!\n");
		goto err_read;
	}

	gfifo_lvl = apds9960_get_gfifo_lvl(apds9960);
	while(gfifo_lvl){
		/* TODO: throttling occured */
	/*while(gfifo_lvl || (gfifo_lvl = apds9960_get_gfifo_lvl(apds9960) > 0)){*/
		err = regmap_bulk_read(apds9960->regmap, APDS9960_REG_GFIFO_U,
				&apds9960->gbuffer, 4);

		if(err)
			goto err_read;

		gfifo_lvl--;
	}

err_read:
	apds9960->gesture_mode_on = 0;
	mutex_unlock(&apds9960->mutex);

	pr_err("gesture data: [u]%x [d]%x [l]%x [r]%x\n", apds9960->gbuffer[0],
		apds9960->gbuffer[1], apds9960->gbuffer[2], apds9960->gbuffer[3]);

	return err;
}


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
		len = sprintf(buf, "apds9960: Device id: %x\n", data);
	}
	else if(strcmp(attr->attr.name, "status") == 0){
		apds9960_get_status(apds9960, &data);
		len = sprintf(buf, "apds9960: Status: 0x%x\n", data);
	}
	else if(strcmp(attr->attr.name, "adc_itime") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_ATIME, &data);
		len = sprintf(buf, "apds9960: ADC Integration time: 0x%x\n", data);
	}
	else if(strcmp(attr->attr.name, "wait_time") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_WTIME, &data);
		len = sprintf(buf, "apds9960: wait time: 0x%x\n", data);
	}
	else if(strcmp(attr->attr.name, "powermode") == 0){
		regmap_read(apds9960->regmap, APDS9960_REG_ENABLE, &data);
		data &= 0x01;
		len = sprintf(buf, "apds9960: power is %x\n", data);
	}

	mutex_unlock(&apds9960->mutex);

	return len;
}

static ssize_t apds9960_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct apds9960_device *apds9960 = dev_get_drvdata(dev);
	unsigned int time;
	bool powermode = false;

	if(mutex_lock_killable(&apds9960->mutex))
		return -EINTR;

	if(strcmp(attr->attr.name, "powermode") == 0){
		if(kstrtobool(&buf[0], &powermode) != 0)
			goto fail;

		apds9960_set_power_mode(apds9960, powermode);
	}

	else if(strcmp(attr->attr.name, "adc_itime") == 0){
		if(kstrtou32(buf, 0, &time) != 0)
			goto fail;

		apds9960_set_integration_time(apds9960, time);
	}

	else if(strcmp(attr->attr.name, "wait_time") == 0){
		if(kstrtou32(buf, 0, &time) != 0)
			goto fail;

		apds9960_set_waiting_time(apds9960, time);
	}
fail:
	mutex_unlock(&apds9960->mutex);

	return count;
}

static DEVICE_ATTR(id, S_IRUGO, apds9960_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, apds9960_show, NULL);
static DEVICE_ATTR(powermode, S_IRUGO | S_IWUSR, apds9960_show, apds9960_store);
static DEVICE_ATTR(adc_itime, S_IRUGO | S_IWUSR, apds9960_show, apds9960_store);
static DEVICE_ATTR(wait_time, S_IRUGO | S_IWUSR, apds9960_show, apds9960_store);

static struct attribute *apds9960_attrs[] = {
	&dev_attr_id.attr,
	&dev_attr_status.attr,
	&dev_attr_adc_itime.attr,
	&dev_attr_wait_time.attr,
	&dev_attr_powermode.attr,
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

static irqreturn_t apds9960_isr(int irq, void *data)
{
	struct apds9960_device *apds9960 = data;
	int status, prox, err;

	pr_err("+++ in iqr!\n");
	err = apds9960_get_status(apds9960, &status);
	if(err < 0){
		pr_err("apds9960: ERR! can`t read status in irq!\n");
		return IRQ_HANDLED;
	}

	if(status & APDS9960_REG_STATUS_PINT){
		apds9960_get_prx_data(apds9960, &prox);
		pr_err("prox interrupt occured! prox: %d\n", prox);
	}

	if(status & APDS9960_REG_STATUS_GINT){
		apds9960_get_gfifo_data(apds9960);
		pr_err("gesture interrupt occured!\n");
	}

	pr_err("--- out irq!\n");

	return IRQ_HANDLED;
}


static int
apds9960_register_configuration(struct apds9960_device *apds9960)
{
	int err = 0;

	/* clear enable register bits before for safe operations */
	err += regmap_update_bits(apds9960->regmap,
			APDS9960_REG_ENABLE, 0xff, 0);

	/* CONFIG1 regiser (0x8D) - wait long time x1 */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_CONFIG1,
			APDS9960_REG_CFG1_WAIT, 0xFF);

	/* CONFIG2 register (0x90) - saturation interupts for prox and clear */
	err += regmap_update_bits(apds9960->regmap,
			APDS9960_REG_CONFIG2, 0xFF, 1);

	/* prox pulse count register (0x8E) - 8us width and 1 count output */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_PPULSE,
			APDS9960_REG_PPULSE_8US, 0xFF);

	/* Control register one (0x8F) gain control */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_CONTROL,
			APDS9960_REG_CTRL_IVD | APDS9960_REG_CTRL_PGAIN |
			APDS9960_REG_CTRL_AGAIN, 0xFF);

	/* gesture GPENTH (0x40) and GENTH (0xA1) in-out thresholds*/
	err += regmap_write(apds9960->regmap, APDS9960_REG_GPENTH,
			APDS9960_REG_GPENTH_DEFAULT);
	err += regmap_write(apds9960->regmap, APDS9960_REG_GEXTH,
			APDS9960_REG_GEXTH_DEFAULT);

	/* gesture configuration 1 (0xA2) - masks and isr for all channels */
	err += regmap_update_bits(apds9960->regmap,
			APDS9960_REG_GCONF1, APDS9960_REG_GCONF1_GFIFOTH_4, 0xFF);

	/* gesture configuration 2 (0xA3) - wtime, VD current, gesture gain */
	err += regmap_update_bits(apds9960->regmap,
			APDS9960_REG_GCONF2, APDS9960_REG_GCONF2_GGAIN_8 |
			APDS9960_REG_GCONF2_IVD_MAX, 0xFF);

	/* gesture configuration 3 (0xAA) - dimensions selection: both pairs */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_GCONF3,
			APDS9960_REG_GCONF3_UDLR, 0xFF);

	/* gesture configuration 4 (0xAB) - operation mode and status */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_GCONF4,
		APDS9960_REG_GCONF4_GIEN | APDS9960_REG_GCONF4_GMOD, 0xFF);

	/* Enable Register (0x80) configuration for gesture mode */
	err += regmap_update_bits(apds9960->regmap, APDS9960_REG_ENABLE,
			APDS9960_REG_ENA_GEN | APDS9960_REG_ENA_PEN
			/*| APDS9960_REG_ENA_PIEN*/, 0xFF);
	if(err < 0){
		pr_err("apds9960: can`t set-config ENABLE register!\n");
		err = -EIO;
	}

	return err;
}


static int
apds9960_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct regmap_config apds9960_cfg;
	struct apds9960_device *apds9960 = NULL;
	unsigned int data;
	int err = 0;
	u32 tmp = 0;

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
	apds9960_cfg.cache_type = REGCACHE_RBTREE; /* !!! */
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

	err = regmap_read(apds9960->regmap, APDS9960_REG_ID, &data);
	if(err != 0){
		pr_err("apds9960: ERR! Can`t communicate!\n");
		goto fail;
	}
	else
		pr_err("apds9960: Device id: 0x%02x", data);

	if(apds9960_register_configuration(apds9960) < 0)
		pr_err("apds9960: ERR! Device configuration failed!\n");

	if(!device_property_present(&client->dev, "interrupts"))
		pr_err("can` read device tree property\n");

	device_property_read_u32(&client->dev, "interrupts", &tmp);
	pr_err("irq device property read: %d\n", tmp);

	if(client->irq <= 0)
		pr_err("apds9960: no valid irq defined!\n");
	else
		pr_err("apds990: irq is: %d\n", client->irq);

	err = devm_request_threaded_irq(&client->dev, client->irq,
			NULL, apds9960_isr, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"apds9960_interrupt", apds9960);
	if(err){
		pr_err("apds9960: ERR! Can`t request irq (%d)!\n", client->irq);
		goto fail;
	}
	else
		pr_err("apds9960: irq request - success!\n");

/*
	apds9960->irqpin = devm_gpiod_get(&client->dev, "int", GPIOD_IN);
	if(IS_ERR(apds9960->irqpin)){
		pr_err("apds9960: ERR! Can`t get INT PIN\n");
	}

	apds9960->gesture_int = gpiod_to_irq(apds9960->irqpin);
	if(apds9960->gesture_int < 0)
		pr_err("apds9960: ERR! Can`t set gpio irq number!\n");

	err = devm_request_irq(&client->dev, apds9960->gesture_int,
			apds9960_isr, IRQF_TRIGGER_FALLING,
			"apds9960_interrupt", apds9960);
	if(err)
		pr_err("apds9960: ERR! Can`t request irq with gpio!\n");
*/


	if(apds9960_set_power_mode(apds9960, 1) < 0)
		pr_err("apds9960: ERR! can`t turn on device!\n");

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

	if(apds9960_set_power_mode(apds9960, 0) < 0)
		pr_err("apds9960: ERR! can`t turn off device!\n");

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
