/*  drivers/misc/sec_jack.c
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/sec_jack.h>
#include <linux/of_gpio.h>

#include <linux/iio/consumer.h>

#ifdef CONFIG_JACK_ADC_MORE_CTIME
#define SEC_JACK_SAMPLE_SIZE	10
#else
#define SEC_JACK_SAMPLE_SIZE	5
#endif

#define MAX_ZONE_LIMIT		10
#define SEND_KEY_CHECK_TIME_MS	40		/* 40ms */
#define WAKE_LOCK_TIME		(HZ * 5)	/* 5 sec */
#define EAR_CHECK_LOOP_CNT	10

#define JACK_CLASS_NAME "audio"
#define JACK_DEV_NAME "earjack"
#define JACK_RESELECTOR_NAME "jack_reselector"

static struct class *jack_class;
static struct device *jack_dev;
static struct device *jack_reselector;
static bool recheck_jack;

struct sec_jack_info {
	struct iio_channel *padc;
	struct sec_jack_platform_data *pdata;
	struct delayed_work jack_detect_work;
	struct work_struct buttons_work;
	struct workqueue_struct *queue;
	struct input_dev *input_dev;
	struct wake_lock det_wake_lock;
	struct sec_jack_zone *zone;
	struct input_handler handler;
	struct input_handle handle;
	struct input_device_id ids[2];
	int det_irq;
	int dev_id;
	int pressed;
	int pressed_code;
	struct platform_device *send_key_dev;
	unsigned int cur_jack_type;
	int det_status;
};

/* with some modifications like moving all the gpio structs inside
 * the platform data and getting the name for the switch and
 * gpio_event from the platform data, the driver could support more than
 * one headset jack, but currently user space is looking only for
 * one key file and switch for a headset so it'd be overkill and
 * untestable so we limit to one instantiation for now.
 */
static atomic_t instantiated = ATOMIC_INIT(0);

/* sysfs name HeadsetObserver.java looks for to track headset state
 */
struct switch_dev switch_jack_detection = {
	.name = "h2w",
};

/* To support AT+FCESTEST=1 */
struct switch_dev switch_sendend = {
	.name = "send_end",
};

static struct gpio_event_direct_entry sec_jack_key_map[] = {
	{
		.code	= KEY_UNKNOWN,
	},
};

static struct gpio_event_input_info sec_jack_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.type = EV_KEY,
	.debounce_time.tv64 = SEND_KEY_CHECK_TIME_MS * NSEC_PER_MSEC,
	.keymap = sec_jack_key_map,
	.keymap_size = ARRAY_SIZE(sec_jack_key_map)
};

static struct gpio_event_info *sec_jack_input_info[] = {
	&sec_jack_key_info.info,
};

static struct gpio_event_platform_data sec_jack_input_data = {
	.name = "sec_jack",
	.info = sec_jack_input_info,
	.info_count = ARRAY_SIZE(sec_jack_input_info),
};

static void sec_jack_gpio_init(struct sec_jack_platform_data *pdata)
{
/* Ear Microphone BIAS */
	if (gpio_request(pdata->ear_micbias_gpio, "EAR MIC")) {
		pr_err(KERN_ERR "GPIO_EAR_MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(pdata->ear_micbias_gpio, 1);
	gpio_set_value(pdata->ear_micbias_gpio, 0);
}

static void set_sec_micbias_state(struct sec_jack_info *hi, bool state)
{
	struct sec_jack_platform_data *pdata = hi->pdata;
 
	if(pdata->ear_micbias_gpio > 0)
		gpio_set_value(pdata->ear_micbias_gpio, state); /*Uses external Mic Bias*/
}

static int sec_jack_get_adc_data(struct iio_channel *padc)
{
	int adc_data = -1;
	int adc_max = 0;
	int adc_min = 0xFFFF;
	int adc_total = 0;
	int adc_retry_cnt = 0;
	int i, ret;

	for (i = 0; i < SEC_JACK_SAMPLE_SIZE; i++) {
		ret = iio_read_channel_raw(&padc[0], &adc_data);
		if (ret < 0) {
			pr_info("read channel error[%d]\n", ret);
			return ret;
		}
		if (adc_data < 0) {
			adc_retry_cnt++;

			if (adc_retry_cnt > 10)
				return adc_data;
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (SEC_JACK_SAMPLE_SIZE - 2);
}

/* gpio_input driver does not support to read adc value.
 * We use input filter to support 3-buttons of headset
 * without changing gpio_input driver.
 */
static bool sec_jack_buttons_filter(struct input_handle *handle,
				    unsigned int type, unsigned int code,
				    int value)
{
	struct sec_jack_info *hi = handle->handler->private;

	if (hi->det_status == true)
		return false;

	if (type != EV_KEY || code != KEY_UNKNOWN)
		return false;

	hi->pressed = value;

	/* This is called in timer handler of gpio_input driver.
	 * We use workqueue to read adc value.
	 */
	queue_work(hi->queue, &hi->buttons_work);

	return true;
}

static int sec_jack_buttons_connect(struct input_handler *handler,
				    struct input_dev *dev,
				    const struct input_device_id *id)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata;
	struct sec_jack_buttons_zone *btn_zones;
	int num_buttons_zones = ARRAY_SIZE(pdata->jack_buttons_zones);
	int err;
	int i;

	/* bind input_handler to input device related to only sec_jack */
	if (dev->name != sec_jack_input_data.name)
		return -ENODEV;

	hi = handler->private;
	pdata = hi->pdata;
	btn_zones = pdata->jack_buttons_zones;

	hi->input_dev = dev;
	hi->handle.dev = dev;
	hi->handle.handler = handler;
	hi->handle.open = 0;
	hi->handle.name = "sec_jack_buttons";

	err = input_register_handle(&hi->handle);
	if (err) {
		pr_err("%s: Failed to register sec_jack buttons handle, "
			"error %d\n", __func__, err);
		goto err_register_handle;
	}

	err = input_open_device(&hi->handle);
	if (err) {
		pr_err("%s: Failed to open input device, error %d\n",
			__func__, err);
		goto err_open_device;
	}

	for (i = 0; i < num_buttons_zones; i++)
		input_set_capability(dev, EV_KEY, btn_zones[i].code);

	return 0;

 err_open_device:
	input_unregister_handle(&hi->handle);
 err_register_handle:

	return err;
}

static void sec_jack_buttons_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
}

static void sec_jack_set_type(struct sec_jack_info *hi, int jack_type)
{
	/* this can happen during slow inserts where we think we identified
	 * the type but then we get another interrupt and do it again
	 */
	if (jack_type == hi->cur_jack_type) {
		if (jack_type != SEC_HEADSET_4POLE)
			set_sec_micbias_state(hi, false);

		return;
	}

	if (jack_type == SEC_HEADSET_4POLE) {
		/* for a 4 pole headset, enable detection of send/end key */
		if (hi->send_key_dev == NULL)
			/* enable to get events again */
			hi->send_key_dev = platform_device_register_data(NULL,
					GPIO_EVENT_DEV_NAME,
					hi->dev_id,
					&sec_jack_input_data,
					sizeof(sec_jack_input_data));
	} else {
		/* for all other jacks, disable send/end key detection */
		if (hi->send_key_dev != NULL) {
			/* disable to prevent false events on next insert */
			platform_device_unregister(hi->send_key_dev);
			hi->send_key_dev = NULL;
		}
		/* micbias is left enabled for 4pole and disabled otherwise */
		set_sec_micbias_state(hi, false);
	}
	/* if user inserted ear jack slowly, different jack event can occur
	 * sometimes because irq_thread is defined IRQ_ONESHOT, detach status
	 * can be ignored sometimes so in that case, driver inform detach
	 * event to user side
	 */
	switch_set_state(&switch_jack_detection, SEC_JACK_NO_DEVICE);

	hi->cur_jack_type = jack_type;
	pr_info("%s : jack_type = %d\n", __func__, jack_type);

	switch_set_state(&switch_jack_detection, jack_type);
}

static void handle_jack_not_inserted(struct sec_jack_info *hi)
{
	sec_jack_set_type(hi, SEC_JACK_NO_DEVICE);
	set_sec_micbias_state(hi, false);
}

static void determine_jack_type(struct sec_jack_info *hi)
{
	struct sec_jack_platform_data *pdata = hi->pdata;
	struct sec_jack_zone *zones = pdata->jack_zones;
	int size = ARRAY_SIZE(pdata->jack_zones);
	int count[MAX_ZONE_LIMIT] = {0};
	int adc;
	int i;
	unsigned npolarity = !pdata->det_active_high;

	/* set mic bias to enable adc */
	set_sec_micbias_state(hi, true);

	while (gpio_get_value(pdata->det_gpio) ^ npolarity) {
		adc = sec_jack_get_adc_data(hi->padc);

		pr_info("%s: adc = %d\n", __func__, adc);

		if (adc < 0)
			break;

		/* determine the type of headset based on the
		 * adc value.  An adc value can fall in various
		 * ranges or zones.  Within some ranges, the type
		 * can be returned immediately.  Within others, the
		 * value is considered unstable and we need to sample
		 * a few more types (up to the limit determined by
		 * the range) before we return the type for that range.
		 */
		for (i = 0; i < size; i++) {
			if (adc <= zones[i].adc_high) {
				if (++count[i] > zones[i].check_count) {
					if (recheck_jack == true && i == 4) {
						pr_info("%s : something wrong connection!\n",
								__func__);
						handle_jack_not_inserted(hi);

						recheck_jack = false;
						return;
					}
					sec_jack_set_type(hi,
						zones[i].jack_type);
					return;
				}
				msleep(zones[i].delay_ms);
				break;
			}
		}
	}

	recheck_jack = false;
	/* jack removed before detection complete */
	pr_debug("%s : jack removed before detection complete\n", __func__);
	handle_jack_not_inserted(hi);
}

/* thread run whenever the headset detect state changes (either insertion
 * or removal).
 */
static irqreturn_t sec_jack_detect_irq_thread(int irq, void *dev_id)
{
	struct sec_jack_info *hi = dev_id;
	struct sec_jack_platform_data *pdata = hi->pdata;
	unsigned npolarity = !pdata->det_active_high;
	int curr_data;
	int pre_data;
	int loopcnt;
	int check_loop_cnt = EAR_CHECK_LOOP_CNT;

	hi->det_status = true;

	/* prevent suspend to allow user space to respond to switch */
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	pr_info("%s: detect_irq(%d)\n", __func__,
 		gpio_get_value(pdata->det_gpio) ^ npolarity);

	/* debounce headset jack.  don't try to determine the type of
	 * headset until the detect state is true for a while.
	 */
	pre_data = 0;
	loopcnt = 0;
	while (true) {
		curr_data = gpio_get_value(pdata->det_gpio);
		if (pre_data == curr_data)
			loopcnt++;
		else
			loopcnt = 0;

		pre_data = curr_data;

		if (loopcnt >= check_loop_cnt) {
			if (!curr_data ^ npolarity) {
				/* jack not detected. */
				handle_jack_not_inserted(hi);
				hi->det_status = false;
				return IRQ_HANDLED;
			}
			break;
		}
		msleep(20);
	}

	/* jack presence was detected the whole time, figure out which type */
	determine_jack_type(hi);
	hi->det_status = false;

	return IRQ_HANDLED;
}

/* thread run whenever the button of headset is pressed or released */
void sec_jack_buttons_work(struct work_struct *work)
{
	struct sec_jack_info *hi =
		container_of(work, struct sec_jack_info, buttons_work);
	struct sec_jack_platform_data *pdata = hi->pdata;
	struct sec_jack_buttons_zone *btn_zones = pdata->jack_buttons_zones;
	int num_buttons_zones = ARRAY_SIZE(pdata->jack_buttons_zones);
	int adc;
	int i;

	/* prevent suspend to allow user space to respond to switch */
	wake_lock_timeout(&hi->det_wake_lock, WAKE_LOCK_TIME);

	/* when button is released */
	if (hi->pressed == 0) {
		input_report_key(hi->input_dev, hi->pressed_code, 0);
		switch_set_state(&switch_sendend, 0);
		input_sync(hi->input_dev);
		pr_info("%s: earkey is released\n", __func__);
		pr_debug("keycode=%d\n", hi->pressed_code);
		return;
	}

	/* when button is pressed */
	adc = sec_jack_get_adc_data(hi->padc);

	for (i = 0; i < num_buttons_zones; i++)
		if (adc >= btn_zones[i].adc_low &&
		    adc <= btn_zones[i].adc_high) {
			hi->pressed_code = btn_zones[i].code;
			input_report_key(hi->input_dev, btn_zones[i].code, 1);
			switch_set_state(&switch_sendend, 1);
			input_sync(hi->input_dev);
			pr_info("%s: earkey is pressed (adc:%d)\n",
					__func__, adc);
			pr_debug("keycode=%d, is pressed\n", btn_zones[i].code);
			return;
		}

	pr_warn("%s: key is skipped. ADC value is %d\n", __func__, adc);
}

static int sec_jack_get_button_type(int arg)
{
	switch(arg) {
	case 1:
		return KEY_VOLUMEUP;
	case 2:
		return KEY_VOLUMEDOWN;
	case 3:
		return KEY_VOICECOMMAND;
	case 0:
	default:
		return KEY_MEDIA;
	}
}

static struct sec_jack_platform_data *sec_jack_populate_dt_pdata(struct device *dev)
{
	struct sec_jack_platform_data *pdata;
	struct of_phandle_args args;
	int i = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s : could not allocate memory for platform data\n", __func__);
		goto alloc_err;
	}

	pdata->det_gpio = of_get_named_gpio(dev->of_node, "earjack-detect-gpio", 0);
	if (pdata->det_gpio < 0) {
		pr_err("%s : can not find the earjack-detect-gpio in the dt\n", __func__);
	} else
		pr_info("%s : earjack-detect-gpio =%d\n", __func__, pdata->det_gpio);

	pdata->send_end_gpio = of_get_named_gpio(dev->of_node, "earjack-sendend-gpio", 0);
	if (pdata->send_end_gpio < 0) {
		pr_err("%s : can not find the earjack-detect-gpio in the dt\n", __func__);
	} else
		pr_info("%s : earjack-sendend-gpio =%d\n", __func__, pdata->send_end_gpio);

	pdata->ear_micbias_gpio = of_get_named_gpio(dev->of_node, "earjack-micbias-gpio", 0);
	if (pdata->ear_micbias_gpio < 0) {
		pr_err("%s : can not find the earjack-micbias-gpio in the dt\n", __func__);
	} else
		pr_info("%s : earjack-micbias-gpio =%d\n", __func__, pdata->ear_micbias_gpio);

	for( i=0; i<4; i++)
	{
		of_parse_phandle_with_args(dev->of_node, "det-zones-list","#list-det-cells", i, &args);
		pdata->jack_zones[i].adc_high = args.args[0];
		pdata->jack_zones[i].delay_ms = args.args[1];
		pdata->jack_zones[i].check_count = args.args[2];
		pdata->jack_zones[i].jack_type = args.args[3]==0?SEC_HEADSET_3POLE:SEC_HEADSET_4POLE;
		pr_info("%s : %d, %d, %d, %d, %d \n",
				__func__, args.args_count, args.args[0],
				args.args[1], args.args[2],args.args[3]);
	}
	for( i=0; i<4; i++)
	{
		of_parse_phandle_with_args(dev->of_node, "but-zones-list","#list-but-cells", i, &args);
		pdata->jack_buttons_zones[i].code = sec_jack_get_button_type(args.args[0]);
		pdata->jack_buttons_zones[i].adc_low = args.args[1];
		pdata->jack_buttons_zones[i].adc_high = args.args[2];
		pr_info("%s : %d, %d, %d, %d\n",
				__func__, args.args_count, args.args[0],
				args.args[1], args.args[2]);
	}

	if (of_find_property(dev->of_node, "send-end-active-high", NULL))
		pdata->send_end_active_high = true;

	return pdata;

alloc_err:
	devm_kfree(dev, pdata);
	return NULL;
}

static int sec_jack_pinctrl_configure(struct sec_jack_platform_data *pdata, bool active)
{
	struct pinctrl_state *set_state;
	int retval;

	pr_info("%s - active = %d\n", __func__, active);
	if (active) {
		set_state = pinctrl_lookup_state(pdata->jack_pinctrl, "earjack_gpio_active");
		if (IS_ERR(set_state)) {
			pr_err("%s: cannot get earjack pinctrl active state\n", __func__);
			return PTR_ERR(set_state);
		}
	} else {
		set_state = pinctrl_lookup_state(pdata->jack_pinctrl, "earjack_gpio_suspend");
		if (IS_ERR(set_state)) {
			pr_err("%s: cannot get earjack pinctrl sleep state\n", __func__);
			return PTR_ERR(set_state);
		}
	}
	retval = pinctrl_select_state(pdata->jack_pinctrl, set_state);
	if (retval) {
		pr_err("%s: cannot set earjack pinctrl active state\n", __func__);
		return retval;
	}
	return 0;
}

static ssize_t select_jack_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s : operate nothing\n", __func__);

	return 0;
}

static ssize_t select_jack_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;


	sscanf(buf, "%d", &value);
	pr_err("%s: User  selection : 0X%x", __func__, value);
	if (value == SEC_HEADSET_4POLE) {
		set_sec_micbias_state(hi, true);
		msleep(100);
	}

	sec_jack_set_type(hi, value);

	return size;
}

static ssize_t earjack_key_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->pressed <= 0)
		value = 0;
	else
		value = 1;

	return sprintf(buf, "%d\n", value);
}

static ssize_t earjack_key_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}

static ssize_t earjack_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;

	if (hi->cur_jack_type == SEC_HEADSET_3POLE ||
		hi->cur_jack_type == SEC_HEADSET_4POLE)
		value = 1;
	else
		value = 0;

	return sprintf(buf, "%d\n", value);
}

static ssize_t earjack_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("%s : operate nothing\n", __func__);

	return size;
}

static ssize_t reselect_jack_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s : operate nothing\n", __func__);

	return 0;
}

static ssize_t reselect_jack_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_jack_info *hi = dev_get_drvdata(dev);
	int value = 0;


	sscanf(buf, "%d", &value);
	pr_err("%s: User reselection : 0X%x", __func__, value);

	if (value == 1) {
		recheck_jack = true;
		determine_jack_type(hi);
	}

	return size;
}

static DEVICE_ATTR(key_state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_key_state_show, earjack_key_state_store);

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR | S_IWGRP,
		   earjack_state_show, earjack_state_store);

static DEVICE_ATTR(select_jack, S_IRUGO | S_IWUSR | S_IWGRP,
		select_jack_show, select_jack_store);

static DEVICE_ATTR(reselect_jack, S_IRUGO | S_IWUSR | S_IWGRP,
		reselect_jack_show, reselect_jack_store);

static void create_class_device_file(struct sec_jack_info *hi)
{
	jack_class = class_create(THIS_MODULE, JACK_CLASS_NAME);
	if (IS_ERR(jack_class))
		pr_err("Failed to create class(sec_jack)\n");

	/* support PBA function test */
	jack_dev = device_create(jack_class, NULL, 0, hi, JACK_DEV_NAME);
	if (IS_ERR(jack_dev))
		pr_err("Failed to create device(sec_jack)!= %ld\n",
			IS_ERR(jack_dev));

	if (device_create_file(jack_dev, &dev_attr_select_jack) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_select_jack.attr.name);

	jack_reselector = device_create(jack_class, NULL, 0, hi,
		JACK_RESELECTOR_NAME);
	if (IS_ERR(jack_reselector))
		pr_err("Failed to create device(sec_jack)!= %ld\n",
			IS_ERR(jack_reselector));

	if (device_create_file(jack_reselector, &dev_attr_reselect_jack) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_reselect_jack.attr.name);

	if (device_create_file(jack_dev, &dev_attr_key_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_key_state.attr.name);

	if (device_create_file(jack_dev, &dev_attr_state) < 0)
		pr_err("Failed to create device file (%s)!\n",
			dev_attr_state.attr.name);
}

static void destroy_class_device_file(void)
{
	device_remove_file(jack_dev, &dev_attr_state);
	device_remove_file(jack_dev, &dev_attr_key_state);
	device_remove_file(jack_reselector, &dev_attr_reselect_jack);
	device_remove_file(jack_dev, &dev_attr_select_jack);
	device_destroy(jack_class, 0);
	class_destroy(jack_class);
}

static int sec_jack_probe(struct platform_device *pdev)
{
	struct sec_jack_info *hi;
	struct sec_jack_platform_data *pdata = pdev->dev.platform_data;
	int ret;

	pr_info("%s : Registering jack driver\n", __func__);

	pdata = sec_jack_populate_dt_pdata(&pdev->dev);
	if (!pdata) {
		pr_err("%s : pdata is NULL.\n", __func__);
		return -ENODEV;
	} else
		sec_jack_gpio_init(pdata);

	if (atomic_xchg(&instantiated, 1)) {
		pr_err("%s : already instantiated, can only have one\n",
			__func__);
		return -ENODEV;
	}

	sec_jack_key_map[0].gpio = pdata->send_end_gpio;

	hi = kzalloc(sizeof(struct sec_jack_info), GFP_KERNEL);
	if (hi == NULL) {
		pr_err("%s : Failed to allocate memory.\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	hi->pdata = pdata;

	/* make the id of our gpio_event device the same as our platform device,
	 * which makes it the responsiblity of the board file to make sure
	 * it is unique relative to other gpio_event devices
	 */
	hi->dev_id = pdev->id;

	ret = gpio_request(pdata->det_gpio, "ear_jack_detect");
	if (ret) {
		pr_err("%s : gpio_request failed for %d\n",
		       __func__, pdata->det_gpio);
		goto err_gpio_request;
	}

	pdata->jack_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pdata->jack_pinctrl)) {
		if (PTR_ERR(pdata->jack_pinctrl) == -EPROBE_DEFER)
		{
			kfree(hi);
			atomic_set(&instantiated, 0);
			return -EPROBE_DEFER;
		}

		pr_err("%s: Target does not use pinctrl\n", __func__);
		pdata->jack_pinctrl = NULL;
	}
	pdata->jack_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pdata->jack_pinctrl)) {
		if (PTR_ERR(pdata->jack_pinctrl) == -EPROBE_DEFER)
		{
			kfree(hi);
			atomic_set(&instantiated, 0);
			return -EPROBE_DEFER;
		}

		pr_err("%s: Target does not use pinctrl\n", __func__);
		pdata->jack_pinctrl = NULL;
	}

	if (pdata->jack_pinctrl) {
		ret = sec_jack_pinctrl_configure(pdata, true);
		if (ret) {
			pr_err("%s: cannot set earjack pinctrl active state\n", __func__);
			goto err_gpio_request;
		}
	}
	if (pdata->jack_pinctrl) {
		ret = sec_jack_pinctrl_configure(pdata, true);
		if (ret) {
			pr_err("%s: cannot set earjack pinctrl active state\n", __func__);
			goto err_gpio_request;
		}
	}

	ret = switch_dev_register(&switch_jack_detection);
	if (ret < 0) {
		pr_err("%s : Failed to register switch device\n", __func__);
		goto err_switch_dev_register;
	}
	switch_set_state(&switch_jack_detection, SEC_JACK_NO_DEVICE);

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0) {
		printk(KERN_ERR "SEC JACK: Failed to register switch device\n");
		goto err_switch_dev_register_send_end;
	}
	wake_lock_init(&hi->det_wake_lock, WAKE_LOCK_SUSPEND, "sec_jack_det");

	INIT_WORK(&hi->buttons_work, sec_jack_buttons_work);
	hi->queue = create_singlethread_workqueue("sec_jack_wq");
	if (hi->queue == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to create workqueue\n", __func__);
		goto err_create_wq_failed;
	}

	hi->det_irq = gpio_to_irq(pdata->det_gpio);

	set_bit(EV_KEY, hi->ids[0].evbit);
	hi->ids[0].flags = INPUT_DEVICE_ID_MATCH_EVBIT;
	hi->handler.filter = sec_jack_buttons_filter;
	hi->handler.connect = sec_jack_buttons_connect;
	hi->handler.disconnect = sec_jack_buttons_disconnect;
	hi->handler.name = "sec_jack_buttons";
	hi->handler.id_table = hi->ids;
	hi->handler.private = hi;

	/* Register adc client */
	hi->padc = iio_channel_get(&pdev->dev, NULL);

	if (IS_ERR(hi->padc)) {
		dev_err(&pdev->dev, "cannot register adc\n");
		ret = PTR_ERR(hi->padc);
		goto err_register_adc;
	}

	/* Create sec_jack class & deive & file */
	create_class_device_file(hi);

	ret = input_register_handler(&hi->handler);
	if (ret) {
		pr_err("%s : Failed to register_handler\n", __func__);
		goto err_register_input_handler;
	}

	ret = request_threaded_irq(hi->det_irq, NULL,
				   sec_jack_detect_irq_thread,
				   IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
				   IRQF_ONESHOT, "sec_headset_detect", hi);
	if (ret) {
		pr_err("%s : Failed to request_irq.\n", __func__);
		goto err_request_detect_irq;
	}

	/* to handle insert/removal when we're sleeping in a call */
	ret = enable_irq_wake(hi->det_irq);
	if (ret) {
		pr_err("%s : Failed to enable_irq_wake.\n", __func__);
		goto err_enable_irq_wake;
	}

	dev_set_drvdata(&pdev->dev, hi);

	/* Prove current earjack state */
	determine_jack_type(hi);
	pr_info("%s : Register jack driver success!!\n", __func__);

	return 0;

err_enable_irq_wake:
	free_irq(hi->det_irq, hi);
err_request_detect_irq:
	input_unregister_handler(&hi->handler);
err_register_input_handler:
	iio_channel_release(hi->padc);
	destroy_class_device_file();
err_register_adc:
	destroy_workqueue(hi->queue);
err_create_wq_failed:
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
err_switch_dev_register_send_end:
	switch_dev_unregister(&switch_jack_detection);
err_switch_dev_register:
	gpio_free(pdata->det_gpio);
err_gpio_request:
	kfree(hi);
err_kzalloc:
	atomic_set(&instantiated, 0);
	gpio_free(pdata->ear_micbias_gpio);

	return ret;
}

static int sec_jack_remove(struct platform_device *pdev)
{

	struct sec_jack_info *hi = dev_get_drvdata(&pdev->dev);

	pr_info("%s :\n", __func__);
	disable_irq_wake(hi->det_irq);
	free_irq(hi->det_irq, hi);
	destroy_workqueue(hi->queue);
	if (hi->send_key_dev) {
		platform_device_unregister(hi->send_key_dev);
		hi->send_key_dev = NULL;
	}
	input_unregister_handler(&hi->handler);
	wake_lock_destroy(&hi->det_wake_lock);
	switch_dev_unregister(&switch_sendend);
	switch_dev_unregister(&switch_jack_detection);
	gpio_free(hi->pdata->det_gpio);
	iio_channel_release(hi->padc);
	kfree(hi);
	atomic_set(&instantiated, 0);

	return 0;
}

static const struct of_device_id sec_jack_dt_match[] = {
	{ .compatible = "sec_jack" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_jack_dt_match);

static struct platform_driver sec_jack_driver = {
	.probe	= sec_jack_probe,
	.remove	= sec_jack_remove,
	.driver	= {
		.name = "sec_jack",
		.owner = THIS_MODULE,
		.of_match_table = sec_jack_dt_match,
	},
};

static int __init sec_jack_init(void)
{
	int ret;

	ret =  platform_driver_register(&sec_jack_driver);

	if (ret)
		pr_err("%s: Failed to add sec jack driver\n", __func__);

	return ret;
}

static void __exit sec_jack_exit(void)
{
	platform_driver_unregister(&sec_jack_driver);
}

module_init(sec_jack_init);
module_exit(sec_jack_exit);

MODULE_AUTHOR("ms17.kim@samsung.com");
MODULE_DESCRIPTION("Samsung Electronics Corp Ear-Jack detection driver");
MODULE_LICENSE("GPL");
