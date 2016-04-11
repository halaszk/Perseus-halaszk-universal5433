/*
 * RGB-led driver for Maxim MAX77843
 *
 * Copyright (C) 2013 Maxim Integrated Product
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/sysfs_helpers.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/mfd/max77843.h>
#include <linux/mfd/max77843-private.h>
#include <linux/leds-max77843-rgb.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>
#include <linux/sec_sysfs.h>

#define SEC_LED_SPECIFIC

/* Registers */
/*defined max77843-private.h*//*
 max77843_led_reg {
	MAX77843_RGBLED_REG_LEDEN           = 0x30,
	MAX77843_RGBLED_REG_LED0BRT         = 0x31,
	MAX77843_RGBLED_REG_LED1BRT         = 0x32,
	MAX77843_RGBLED_REG_LED2BRT         = 0x33,
	MAX77843_RGBLED_REG_LED3BRT         = 0x34,
	MAX77843_RGBLED_REG_LEDRMP          = 0x36,
	MAX77843_RGBLED_REG_LEDBLNK         = 0x38,
	MAX77843_LED_REG_END,
};*/

/* MAX77843_REG_LED0BRT */
#define MAX77843_LED0BRT	0xFF

/* MAX77843_REG_LED1BRT */
#define MAX77843_LED1BRT	0xFF

/* MAX77843_REG_LED2BRT */
#define MAX77843_LED2BRT	0xFF

/* MAX77843_REG_LED3BRT */
#define MAX77843_LED3BRT	0xFF

/* MAX77843_REG_LEDBLNK */
#define MAX77843_LEDBLINKD	0xF0
#define MAX77843_LEDBLINKP	0x0F

/* MAX77843_REG_LEDRMP */
#define MAX77843_RAMPUP		0xF0
#define MAX77843_RAMPDN		0x0F

#define LED_R_MASK		0x00FF0000
#define LED_G_MASK		0x0000FF00
#define LED_B_MASK		0x000000FF
#define LED_MAX_CURRENT		0xFF

/* MAX77843_STATE*/
#define LED_DISABLE			0
#define LED_ALWAYS_ON			1
#define LED_BLINK			2

#define LEDBLNK_ON(time)	((time < 100) ? 0 :			\
				(time < 500) ? time/100-1 :		\
				(time < 3250) ? (time-500)/250+4 : 15)

#define LEDBLNK_OFF(time)	((time < 500) ? 0x00 :			\
				(time < 5000) ? time/500 :		\
				(time < 8000) ? (time-5000)/1000+10 :	 \
				(time < 12000) ? (time-8000)/2000+13 : 15)

static u8 led_dynamic_current = 0x14;
static u8 led_color_dynamic_current = 0x14;
static u8 led_lowpower_mode = 0x0;

enum max77843_led_color {
	WHITE,
	RED,
	GREEN,
	BLUE,
};
enum max77843_led_pattern {
	PATTERN_OFF,
	CHARGING,
	CHARGING_ERR,
	MISSED_NOTI,
	LOW_BATTERY,
	FULLY_CHARGED,
	POWERING,
};

static struct device *led_dev;

struct max77843_rgb {
	struct led_classdev led[4];
	struct i2c_client *i2c;
	unsigned int delay_on_times_ms;
	unsigned int delay_off_times_ms;
};

#ifdef SEC_LED_SPECIFIC
static struct leds_control {
    u8 	current_low;
    u8 	current_high;
    u16 	noti_ramp_control;
    u16 	noti_ramp_up;
    u16 	noti_ramp_down;
    u16 	noti_delay_on;
    u16 	noti_delay_off;
} leds_control = {
    .current_low = 5,
    .current_high = 40,
    .noti_ramp_control = 0,
    .noti_ramp_up = 800,
    .noti_ramp_down = 1000,
    .noti_delay_on = 500,
    .noti_delay_off = 5000,
};
#endif

static unsigned int lcdtype_color;
extern unsigned int lcdtype;
#if defined(CONFIG_LEDS_USE_ED28) && defined(CONFIG_SEC_FACTORY)
bool jig_status = false;

static int __init jig_check(char *str)
{
	jig_status = true;
	return 0;
}
__setup("jig", jig_check);
#endif

static int max77843_rgb_number(struct led_classdev *led_cdev,
				struct max77843_rgb **p)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(parent);
	int i;

	*p = max77843_rgb;

	for (i = 0; i < 4; i++) {
		if (led_cdev == &max77843_rgb->led[i]) {
			pr_info("leds-max77843-rgb: %s, %d\n", __func__, i);
			return i;
		}
	}

	return -ENODEV;
}

static void max77843_rgb_set(struct led_classdev *led_cdev,
				unsigned int brightness)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;

	ret = max77843_rgb_number(led_cdev, &max77843_rgb);
	if (IS_ERR_VALUE(ret)) {
		dev_err(led_cdev->dev,
			"max77843_rgb_number() returns %d.\n", ret);
		return;
	}

	dev = led_cdev->dev;
	n = ret;

	if (brightness == LED_OFF) {
		/* Flash OFF */
		ret = max77843_update_reg(max77843_rgb->i2c,
					MAX77843_RGBLED_REG_LEDEN, 0 , 3 << (2*n));
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "can't write LEDEN : %d\n", ret);
			return;
		}
	} else {
		/* Set current */
		if (n==BLUE && (lcdtype_color != 51))
			brightness = (brightness * 5) / 2;
		ret = max77843_write_reg(max77843_rgb->i2c,
				MAX77843_RGBLED_REG_LED0BRT + n, brightness);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "can't write LEDxBRT : %d\n", ret);
			return;
		}
		/* Flash ON */
		ret = max77843_update_reg(max77843_rgb->i2c,
				MAX77843_RGBLED_REG_LEDEN, 0x55, 3 << (2*n));
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "can't write FLASH_EN : %d\n", ret);
			return;
		}
	}
}

static void max77843_rgb_set_state(struct led_classdev *led_cdev,
				unsigned int brightness, unsigned int led_state)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;

	pr_info("leds-max77843-rgb: %s\n", __func__);

	ret = max77843_rgb_number(led_cdev, &max77843_rgb);

	if (IS_ERR_VALUE(ret)) {
		dev_err(led_cdev->dev,
			"max77843_rgb_number() returns %d.\n", ret);
		return;
	}

	dev = led_cdev->dev;
	n = ret;

	max77843_rgb_set(led_cdev, brightness);

	ret = max77843_update_reg(max77843_rgb->i2c,
			MAX77843_RGBLED_REG_LEDEN, led_state << (2*n), 0x3 << 2*n);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "can't write FLASH_EN : %d\n", ret);
		return;
	}
}

static unsigned int max77843_rgb_get(struct led_classdev *led_cdev)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;
	u8 value;

	pr_info("leds-max77843-rgb: %s\n", __func__);

	ret = max77843_rgb_number(led_cdev, &max77843_rgb);
	if (IS_ERR_VALUE(ret)) {
		dev_err(led_cdev->dev,
			"max77843_rgb_number() returns %d.\n", ret);
		return 0;
	}
	n = ret;

	dev = led_cdev->dev;

	/* Get status */
	ret = max77843_read_reg(max77843_rgb->i2c,
				MAX77843_RGBLED_REG_LEDEN, &value);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "can't read LEDEN : %d\n", ret);
		return 0;
	}
	if (!(value & (1 << n)))
		return LED_OFF;

	/* Get current */
	ret = max77843_read_reg(max77843_rgb->i2c,
				MAX77843_RGBLED_REG_LED0BRT + n, &value);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "can't read LED0BRT : %d\n", ret);
		return 0;
	}

	return value;
}

static int max77843_rgb_ramp(struct device *dev, int ramp_up, int ramp_down)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	int value;
	int ret;

	pr_info("leds-max77843-rgb: %s\n", __func__);

	if (ramp_up <= 800) {
		ramp_up /= 100;
	} else {
		ramp_up = (ramp_up - 800) * 2 + 800;
		ramp_up /= 100;
	}

	if (ramp_down <= 800) {
		ramp_down /= 100;
	} else {
		ramp_down = (ramp_down - 800) * 2 + 800;
		ramp_down /= 100;
	}

	value = (ramp_down) | (ramp_up << 4);
	ret = max77843_write_reg(max77843_rgb->i2c,
					MAX77843_RGBLED_REG_LEDRMP, value);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "can't write REG_LEDRMP : %d\n", ret);
		return -ENODEV;
	}

	return 0;
}

static int max77843_rgb_blink(struct device *dev,
				unsigned int delay_on, unsigned int delay_off)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	int value;
	int ret = 0;

	pr_info("leds-max77843-rgb: %s\n", __func__);

	if( delay_on > 3250 || delay_off > 12000 )
		return -EINVAL;
	else {
		value = (LEDBLNK_ON(delay_on) << 4) | LEDBLNK_OFF(delay_off);
		ret = max77843_write_reg(max77843_rgb->i2c,
					MAX77843_RGBLED_REG_LEDBLNK, value);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "can't write REG_LEDBLNK : %d\n", ret);
			return -EINVAL;
		}
	}

	return ret;
}

#ifdef CONFIG_OF
static struct max77843_rgb_platform_data
			*max77843_rgb_parse_dt(struct device *dev)
{
	struct max77843_rgb_platform_data *pdata;
	struct device_node *nproot = dev->parent->of_node;
	struct device_node *np;
	int ret;
	int i;

	pr_info("leds-max77843-rgb: %s\n", __func__);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (unlikely(pdata == NULL))
		return ERR_PTR(-ENOMEM);

	np = of_find_node_by_name(nproot, "rgb");
	if (unlikely(np == NULL)) {
		dev_err(dev, "rgb node not found\n");
		devm_kfree(dev, pdata);
		return ERR_PTR(-EINVAL);
	}

	for (i = 0; i < 4; i++)	{
		ret = of_property_read_string_index(np, "rgb-name", i,
						(const char **)&pdata->name[i]);

		pr_info("leds-max77843-rgb: %s, %s\n", __func__,pdata->name[i]);

		if (IS_ERR_VALUE(ret)) {
			devm_kfree(dev, pdata);
			return ERR_PTR(ret);
		}
	}

	return pdata;
}
#endif

static void max77843_rgb_reset(struct device *dev)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	max77843_rgb_set_state(&max77843_rgb->led[RED], LED_OFF, LED_DISABLE);
	max77843_rgb_set_state(&max77843_rgb->led[GREEN], LED_OFF, LED_DISABLE);
	max77843_rgb_set_state(&max77843_rgb->led[BLUE], LED_OFF, LED_DISABLE);
	max77843_rgb_ramp(dev, 0, 0);
}

static ssize_t store_max77843_rgb_lowpower(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int ret;
	u8 led_lowpower;
    struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 0, &led_lowpower);
	if (ret != 0) {
		dev_err(dev, "fail to get led_lowpower.\n");
		return count;
	}

	led_lowpower_mode = led_lowpower;
    led_dynamic_current = (led_lowpower_mode) ? leds_control.current_low : leds_control.current_high;

    max77843_rgb_set_state(&max77843_rgb->led[RED], led_dynamic_current, LED_BLINK);
    max77843_rgb_set_state(&max77843_rgb->led[GREEN], led_dynamic_current, LED_BLINK);
    max77843_rgb_set_state(&max77843_rgb->led[BLUE], led_dynamic_current, LED_BLINK);

	dev_dbg(dev, "led_lowpower mode set to %i\n", led_lowpower);

	return count;
}
static ssize_t store_max77843_rgb_brightness(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int ret;
    u8 max_brightness;
	u8 brightness;
	pr_info("leds-max77843-rgb: %s\n", __func__);

	ret = kstrtou8(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get led_brightness.\n");
		return count;
	}

	led_lowpower_mode = 0;

    max_brightness = (led_lowpower_mode) ? leds_control.current_low : leds_control.current_high;
    brightness = (brightness * max_brightness) / LED_MAX_CURRENT;

	led_dynamic_current = brightness;

	dev_dbg(dev, "led brightness set to %i\n", brightness);

	return count;
}

static ssize_t store_max77843_rgb_pattern(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int mode = 0;
	int ret;
	pr_info("leds-max77843-rgb: %s\n", __func__);

	ret = sscanf(buf, "%1d", &mode);
	if (ret == 0) {
		dev_err(dev, "fail to get led_pattern mode.\n");
		return count;
	}

	/* Set all LEDs Off */
	max77843_rgb_reset(dev);
	if (mode == PATTERN_OFF)
		return count;

	/* Set to low power consumption mode */
	if (led_lowpower_mode == 1)
		led_dynamic_current = 0x5;
	else
		led_dynamic_current = led_color_dynamic_current;

	switch (mode) {

	case CHARGING:
		max77843_rgb_set_state(&max77843_rgb->led[RED], led_dynamic_current, LED_ALWAYS_ON);
		break;
	case CHARGING_ERR:
	if (leds_control.noti_ramp_control == 1)
		max77843_rgb_ramp(dev, leds_control.noti_ramp_up, leds_control.noti_ramp_down);
		max77843_rgb_blink(dev, 500, 500);
		max77843_rgb_set_state(&max77843_rgb->led[RED], led_dynamic_current, LED_BLINK);
		break;
	case MISSED_NOTI:
	if (leds_control.noti_ramp_control == 1)
		max77843_rgb_ramp(dev, leds_control.noti_ramp_up, leds_control.noti_ramp_down);
		max77843_rgb_blink(dev, leds_control.noti_delay_on, leds_control.noti_delay_off);
		max77843_rgb_set_state(&max77843_rgb->led[BLUE], led_dynamic_current, LED_BLINK);
		break;
	case LOW_BATTERY:
	if (leds_control.noti_ramp_control == 1)
		max77843_rgb_ramp(dev, leds_control.noti_ramp_up, leds_control.noti_ramp_down);
		max77843_rgb_blink(dev, leds_control.noti_delay_on, leds_control.noti_delay_off);
		max77843_rgb_set_state(&max77843_rgb->led[RED], led_dynamic_current, LED_BLINK);
		break;
	case FULLY_CHARGED:
		max77843_rgb_set_state(&max77843_rgb->led[GREEN], led_dynamic_current, LED_ALWAYS_ON);
		break;
	case POWERING:
	if (leds_control.noti_ramp_control == 1)
		max77843_rgb_ramp(dev, leds_control.noti_ramp_up, leds_control.noti_ramp_down);
		max77843_rgb_blink(dev, leds_control.noti_delay_on, leds_control.noti_delay_off);
		max77843_rgb_set_state(&max77843_rgb->led[BLUE], led_dynamic_current, LED_ALWAYS_ON);
		max77843_rgb_set_state(&max77843_rgb->led[GREEN], led_dynamic_current, LED_BLINK);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t store_max77843_rgb_blink(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	int led_brightness = 0;
	int delay_on_time = 0;
	int delay_off_time = 0;
	u8 led_r_brightness = 0;
	u8 led_g_brightness = 0;
	u8 led_b_brightness = 0;
	int ret;

	ret = sscanf(buf, "0x%8x %5d %5d", &led_brightness,
					&delay_on_time, &delay_off_time);
	if (ret == 0) {
		dev_err(dev, "fail to get led_blink value.\n");
		return count;
	}

	/*Reset led*/
	max77843_rgb_reset(dev);
	max77843_rgb_blink(dev, 0, 0);

	led_r_brightness = (led_brightness & LED_R_MASK) >> 16;
	led_g_brightness = (led_brightness & LED_G_MASK) >> 8;
	led_b_brightness = led_brightness & LED_B_MASK;

	/* In user case, LED current is restricted to less than 2mA */
	led_r_brightness = (led_r_brightness * led_dynamic_current) / LED_MAX_CURRENT;
	led_g_brightness = (led_g_brightness * led_dynamic_current) / LED_MAX_CURRENT;
	led_b_brightness = (led_b_brightness * led_dynamic_current) / LED_MAX_CURRENT;

	if (led_r_brightness) {
		max77843_rgb_set_state(&max77843_rgb->led[RED], led_r_brightness, LED_BLINK);
	}
	if (led_g_brightness) {
		max77843_rgb_set_state(&max77843_rgb->led[GREEN], led_g_brightness, LED_BLINK);
	}
	if (led_b_brightness) {
		max77843_rgb_set_state(&max77843_rgb->led[BLUE], led_b_brightness, LED_BLINK);
	}
	/*Set LED blink mode*/
		if (leds_control.noti_ramp_control == 1)
	max77843_rgb_ramp(dev, leds_control.noti_ramp_up, leds_control.noti_ramp_down);
	
	max77843_rgb_blink(dev, delay_on_time, delay_off_time);
	

	pr_info("leds-max77843-rgb: %s\n", __func__);
	dev_dbg(dev, "led_blink is called, Color:0x%X Brightness:%i\n",
			led_brightness, led_dynamic_current);
	return count;
}

static ssize_t store_led_r(struct device *dev,
			struct device_attribute *devattr,
				const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0) {
		max77843_rgb_set_state(&max77843_rgb->led[RED], brightness, LED_ALWAYS_ON);
	} else {
		max77843_rgb_set_state(&max77843_rgb->led[RED], LED_OFF, LED_DISABLE);
	}
out:
	pr_info("leds-max77843-rgb: %s\n", __func__);
	return count;
}
static ssize_t store_led_g(struct device *dev,
			struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0) {
		max77843_rgb_set_state(&max77843_rgb->led[GREEN], brightness, LED_ALWAYS_ON);
	} else {
		max77843_rgb_set_state(&max77843_rgb->led[GREEN], LED_OFF, LED_DISABLE);
	}
out:
	pr_info("leds-max77843-rgb: %s\n", __func__);
	return count;
}
static ssize_t store_led_b(struct device *dev,
		struct device_attribute *devattr,
		const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0) {
		max77843_rgb_set_state(&max77843_rgb->led[BLUE], brightness, LED_ALWAYS_ON);
	} else	{
		max77843_rgb_set_state(&max77843_rgb->led[BLUE], LED_OFF, LED_DISABLE);
	}
out:
	pr_info("leds-max77843-rgb: %s\n", __func__);
	return count;
}

/* Added for led common class */
static ssize_t led_delay_on_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", max77843_rgb->delay_on_times_ms);
}

static ssize_t led_delay_on_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int time;

	if (kstrtouint(buf, 0, &time)) {
		dev_err(dev, "can not write led_delay_on\n");
		return count;
	}

	max77843_rgb->delay_on_times_ms = time;

	return count;
}

static ssize_t led_delay_off_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", max77843_rgb->delay_off_times_ms);
}

static ssize_t led_delay_off_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int time;

	if (kstrtouint(buf, 0, &time)) {
		dev_err(dev, "can not write led_delay_off\n");
		return count;
	}

	max77843_rgb->delay_off_times_ms = time;

	return count;
}

static ssize_t led_blink_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	const struct device *parent = dev->parent;
	struct max77843_rgb *max77843_rgb_num = dev_get_drvdata(parent);
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	unsigned int blink_set;
	int n = 0;
	int i;

	if (!sscanf(buf, "%1d", &blink_set)) {
		dev_err(dev, "can not write led_blink\n");
		return count;
	}

	if (!blink_set) {
		max77843_rgb->delay_on_times_ms = LED_OFF;
		max77843_rgb->delay_off_times_ms = LED_OFF;
	}

	for (i = 0; i < 4; i++) {
		if (dev == max77843_rgb_num->led[i].dev)
			n = i;
	}

	max77843_rgb_blink(max77843_rgb_num->led[n].dev->parent,
		max77843_rgb->delay_on_times_ms,
		max77843_rgb->delay_off_times_ms);
	max77843_rgb_set_state(&max77843_rgb_num->led[n], led_dynamic_current, LED_BLINK);

	pr_info("leds-max77843-rgb: %s\n", __func__);
	return count;
}

/* permission for sysfs node */
static DEVICE_ATTR(delay_on, 0640, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0640, led_delay_off_show, led_delay_off_store);
static DEVICE_ATTR(blink, 0640, NULL, led_blink_store);

#ifdef SEC_LED_SPECIFIC
static ssize_t show_leds_property(struct device *dev,
                                  struct device_attribute *attr, char *buf);

static ssize_t store_leds_property(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t len);

#define LEDS_ATTR(_name)				\
{							\
.attr = {					\
.name = #_name,			\
.mode = S_IRUGO | S_IWUSR | S_IWGRP,	\
},					\
.show = show_leds_property,			\
.store = store_leds_property,			\
}

static struct device_attribute leds_control_attrs[] = {
    LEDS_ATTR(led_lowpower_current),
    LEDS_ATTR(led_highpower_current),
    LEDS_ATTR(led_notification_ramp_control),
    LEDS_ATTR(led_notification_ramp_up),
    LEDS_ATTR(led_notification_ramp_down),
    LEDS_ATTR(led_notification_delay_on),
    LEDS_ATTR(led_notification_delay_off),
};

enum {
    LOWPOWER_CURRENT = 0,
    HIGHPOWER_CURRENT,
	NOTIFICATION_RAMP_CONTROL,
    NOTIFICATION_RAMP_UP,
    NOTIFICATION_RAMP_DOWN,
    NOTIFICATION_DELAY_ON,
    NOTIFICATION_DELAY_OFF,
};

static ssize_t show_leds_property(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    const ptrdiff_t offset = attr - leds_control_attrs;
    
    switch (offset) {
        case LOWPOWER_CURRENT:
            return sprintf(buf, "%d", leds_control.current_low);
        case HIGHPOWER_CURRENT:
            return sprintf(buf, "%d", leds_control.current_high);
        case NOTIFICATION_RAMP_CONTROL:
            return sprintf(buf, "%d", leds_control.noti_ramp_control);
        case NOTIFICATION_RAMP_UP:
            return sprintf(buf, "%d", leds_control.noti_ramp_up);
        case NOTIFICATION_RAMP_DOWN:
            return sprintf(buf, "%d", leds_control.noti_ramp_down);
        case NOTIFICATION_DELAY_ON:
            return sprintf(buf, "%d", leds_control.noti_delay_on);
        case NOTIFICATION_DELAY_OFF:
            return sprintf(buf, "%d", leds_control.noti_delay_off);
    }
    
    return -EINVAL;
}

static ssize_t store_leds_property(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t len)
{
    int val;
    const ptrdiff_t offset = attr - leds_control_attrs;
    
    if(sscanf(buf, "%d", &val) != 1)
        return -EINVAL;
    
    switch (offset) {
        case LOWPOWER_CURRENT:
            sanitize_min_max(val, 0, LED_MAX_CURRENT);
            leds_control.current_low = val;
            break;
        case HIGHPOWER_CURRENT:
            sanitize_min_max(val, 0, LED_MAX_CURRENT);
            leds_control.current_high = val;
            break;
	case NOTIFICATION_RAMP_CONTROL:
            sanitize_min_max(val, 0, 1);
            leds_control.noti_ramp_control = val;
            break;
	case NOTIFICATION_RAMP_UP:
            sanitize_min_max(val, 0, 2000);
            leds_control.noti_ramp_up = val;
            break;
        case NOTIFICATION_RAMP_DOWN:
            sanitize_min_max(val, 0, 2000);
            leds_control.noti_ramp_down = val;
            break;
	case NOTIFICATION_DELAY_ON:
            sanitize_min_max(val, 0, 10000);
            leds_control.noti_delay_on = val;
            break;
        case NOTIFICATION_DELAY_OFF:
            sanitize_min_max(val, 0, 10000);
            leds_control.noti_delay_off = val;
            break;
    }
    
    return len;
}
/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR(led_r, 0660, NULL, store_led_r);
static DEVICE_ATTR(led_g, 0660, NULL, store_led_g);
static DEVICE_ATTR(led_b, 0660, NULL, store_led_b);
/* led_pattern node permission is 222 */
/* To access sysfs node from other groups */
static DEVICE_ATTR(led_pattern, 0660, NULL, store_max77843_rgb_pattern);
static DEVICE_ATTR(led_blink, 0660, NULL,  store_max77843_rgb_blink);
static DEVICE_ATTR(led_brightness, 0660, NULL, store_max77843_rgb_brightness);
static DEVICE_ATTR(led_lowpower, 0660, NULL,  store_max77843_rgb_lowpower);
#endif

static struct attribute *led_class_attrs[] = {
	&dev_attr_delay_on.attr,
	&dev_attr_delay_off.attr,
	&dev_attr_blink.attr,
	NULL,
};

static struct attribute_group common_led_attr_group = {
	.attrs = led_class_attrs,
};

#ifdef SEC_LED_SPECIFIC
static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_r.attr,
	&dev_attr_led_g.attr,
	&dev_attr_led_b.attr,
	&dev_attr_led_pattern.attr,
	&dev_attr_led_blink.attr,
	&dev_attr_led_brightness.attr,
	&dev_attr_led_lowpower.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};
#endif
static int max77843_rgb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct max77843_rgb_platform_data *pdata;
	struct max77843_rgb *max77843_rgb;
	struct max77843_dev *max77843_dev = dev_get_drvdata(dev->parent);
	char temp_name[4][40] = {{0,},}, name[40] = {0,}, *p;
	int i, ret;

	pr_info("leds-max77843-rgb: %s\n", __func__);

#ifdef CONFIG_OF
	pdata = max77843_rgb_parse_dt(dev);
	if (unlikely(IS_ERR(pdata)))
		return PTR_ERR(pdata);
#else
	pdata = dev_get_platdata(dev);
#endif

	max77843_rgb = devm_kzalloc(dev, sizeof(struct max77843_rgb), GFP_KERNEL);
	if (unlikely(!max77843_rgb))
		return -ENOMEM;
	pr_info("leds-max77843-rgb: %s 1 \n", __func__);

	max77843_rgb->i2c = max77843_dev->i2c;

	for (i = 0; i < 4; i++) {
		ret = snprintf(name, 30, "%s", pdata->name[i])+1;
		if (1 > ret)
			goto alloc_err_flash;

		p = devm_kzalloc(dev, ret, GFP_KERNEL);
		if (unlikely(!p))
			goto alloc_err_flash;

		strcpy(p, name);
		strcpy(temp_name[i], name);
		max77843_rgb->led[i].name = p;
		max77843_rgb->led[i].brightness_set = max77843_rgb_set;
		max77843_rgb->led[i].brightness_get = max77843_rgb_get;
		max77843_rgb->led[i].max_brightness = LED_MAX_CURRENT;

		ret = led_classdev_register(dev, &max77843_rgb->led[i]);
		if (IS_ERR_VALUE(ret)) {
			dev_err(dev, "unable to register RGB : %d\n", ret);
			goto alloc_err_flash_plus;
		}
		ret = sysfs_create_group(&max77843_rgb->led[i].dev->kobj,
						&common_led_attr_group);
		if (ret) {
			dev_err(dev, "can not register sysfs attribute\n");
			goto register_err_flash;
		}
	}

	led_dev = sec_device_create(max77843_rgb, "led");
	if (IS_ERR(led_dev)) {
		dev_err(dev, "Failed to create device for samsung specific led\n");
		goto register_err_flash;
	}


	ret = sysfs_create_group(&led_dev->kobj, &sec_led_attr_group);
	if (ret < 0) {
		dev_err(dev, "Failed to create sysfs group for samsung specific led\n");
		goto device_create_err;
	}

    for(i = 0; i < ARRAY_SIZE(leds_control_attrs); i++) {
        ret = sysfs_create_file(&led_dev->kobj, &leds_control_attrs[i].attr);
    }

	platform_set_drvdata(pdev, max77843_rgb);
#if defined(CONFIG_LEDS_USE_ED28) && defined(CONFIG_SEC_FACTORY)
	if( lcdtype == 0 && jig_status == false) {
		max77843_rgb_set_state(&max77843_rgb->led[RED], led_dynamic_current, LED_ALWAYS_ON);
	}
#endif
	lcdtype_color = lcdtype >> 0x10;
	if (lcdtype_color == 51)
		led_color_dynamic_current = 0x5A;

	pr_info("leds-max77843-rgb: %s done\n", __func__);

	return 0;

device_create_err:
	sec_device_destroy(led_dev->devt);
register_err_flash:
	led_classdev_unregister(&max77843_rgb->led[i]);
alloc_err_flash_plus:
	devm_kfree(dev, temp_name[i]);
alloc_err_flash:
	while (i--) {
		led_classdev_unregister(&max77843_rgb->led[i]);
		devm_kfree(dev, temp_name[i]);
	}
	devm_kfree(dev, max77843_rgb);
	return -ENOMEM;
}

static int max77843_rgb_remove(struct platform_device *pdev)
{
	struct max77843_rgb *max77843_rgb = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < 4; i++)
		led_classdev_unregister(&max77843_rgb->led[i]);

	return 0;
}

static void max77843_rgb_shutdown(struct device *dev)
{
	struct max77843_rgb *max77843_rgb = dev_get_drvdata(dev);
	int i;

	if (!max77843_rgb->i2c)
		return;

	max77843_rgb_reset(dev);

	sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);

	for (i = 0; i < 4; i++){
		sysfs_remove_group(&max77843_rgb->led[i].dev->kobj,
						&common_led_attr_group);
		led_classdev_unregister(&max77843_rgb->led[i]);
	}
	devm_kfree(dev, max77843_rgb);
}
static struct platform_driver max77843_fled_driver = {
	.driver		= {
		.name	= "leds-max77843-rgb",
		.owner	= THIS_MODULE,
		.shutdown = max77843_rgb_shutdown,
	},
	.probe		= max77843_rgb_probe,
	.remove		= __devexit_p(max77843_rgb_remove),
};

static int __init max77843_rgb_init(void)
{
	pr_info("leds-max77843-rgb: %s\n", __func__);
	return platform_driver_register(&max77843_fled_driver);
}
module_init(max77843_rgb_init);

static void __exit max77843_rgb_exit(void)
{
	platform_driver_unregister(&max77843_fled_driver);
}
module_exit(max77843_rgb_exit);

MODULE_ALIAS("platform:max77843-rgb");
MODULE_AUTHOR("Jeongwoong Lee<jell.lee@samsung.com>");
MODULE_DESCRIPTION("MAX77843 RGB driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
