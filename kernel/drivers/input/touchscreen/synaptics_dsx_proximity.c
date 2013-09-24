/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/synaptics_dsx.h>
#include "synaptics_dsx_i2c.h"

#define DRIVER_NAME "synaptics_dsx_i2c"
#define PROX_PHYS_NAME "synaptics_dsx_i2c/input1"

#define FINGER_LIFT_TIME 50 /* ms */
#define HOVER_Z_MAX (255)

#define F51_PROXIMITY_ENABLES_OFFSET (0)
#define FINGER_HOVER_EN (1 << 0)
#define AIR_SWIPE_EN (1 << 1)
#define LARGE_OBJ_EN (1 << 2)
#define HOVER_PINCH_EN (1 << 3)
#define F51_PROXIMITY_ENABLES FINGER_HOVER_EN

#define F51_GENERAL_CONTROL_OFFSET (1)
#define NO_PROXIMITY_ON_TOUCH (1 << 2)
#define CONTINUOUS_LOAD_REPORT (1 << 3)
#define HOST_REZERO_COMMAND (1 << 4)
#define EDGE_SWIPE_EN (1 << 5)
#define F51_GENERAL_CONTROL (NO_PROXIMITY_ON_TOUCH | HOST_REZERO_COMMAND)

static ssize_t synaptics_rmi4_prox_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_prox_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_prox_general_control_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_prox_general_control_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static struct device_attribute attrs[] = {
	__ATTR(proximity_enables, (S_IRUGO | S_IWUGO),
			synaptics_rmi4_prox_enables_show,
			synaptics_rmi4_prox_enables_store),
	__ATTR(general_control, (S_IRUGO | S_IWUGO),
			synaptics_rmi4_prox_general_control_show,
			synaptics_rmi4_prox_general_control_store),
};

struct prox_finger_data {
	union {
		struct {
			unsigned char finger_hover_det:1;
			unsigned char air_swipe_det:1;
			unsigned char large_obj_det:1;
			unsigned char f51_data0_b3:1;
			unsigned char hover_pinch_det:1;
			unsigned char f51_data0_b5__7:3;
			unsigned char hover_finger_x_4__11;
			unsigned char hover_finger_y_4__11;
			unsigned char hover_finger_xy_0__3;
			unsigned char hover_finger_z;
			unsigned char f51_data2_b0__6:7;
			unsigned char hover_pinch_dir:1;
			unsigned char air_swipe_dir_0:1;
			unsigned char air_swipe_dir_1:1;
			unsigned char f51_data3_b2__4:3;
			unsigned char object_present:1;
			unsigned char large_obj_act:2;
		} __packed;
		unsigned char proximity_data[7];
	};
};

struct synaptics_rmi4_prox_handle {
	bool finger_present;
	unsigned char intr_mask;
	unsigned char proximity_enables;
	unsigned char general_control;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short proximity_enables_addr;
	unsigned short general_control_addr;
	struct input_dev *prox_dev;
	struct timer_list finger_lift_timer;
	struct prox_finger_data *finger_data;
	struct synaptics_rmi4_access_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
};

static struct synaptics_rmi4_prox_handle *prox;

DECLARE_COMPLETION(prox_remove_complete);

static void prox_finger_lift(void)
{
	input_report_key(prox->prox_dev, BTN_TOUCH, 0);
	input_report_key(prox->prox_dev, BTN_TOOL_FINGER, 0);
	input_sync(prox->prox_dev);
	prox->finger_present = false;

	return;
}

static void prox_finger_lift_timer(unsigned long data)
{
	if (prox->finger_present) {
		prox->finger_present = false;
		mod_timer(&prox->finger_lift_timer,
				jiffies + msecs_to_jiffies(FINGER_LIFT_TIME));
	} else {
		prox_finger_lift();
	}

	return;
}

static void prox_finger_report(void)
{
	int retval;
	int x;
	int y;
	int z;
	struct prox_finger_data *data;

	data = prox->finger_data;

	retval = prox->fn_ptr->read(prox->rmi4_data,
			prox->data_base_addr,
			data->proximity_data,
			sizeof(data->proximity_data));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to read proximity data registers\n",
				__func__);
		return;
	}

	if (data->proximity_data[0] == 0x00)
		return;

	if (data->finger_hover_det && (data->hover_finger_z > 0)) {
		x = (data->hover_finger_x_4__11 << 4) |
				(data->hover_finger_xy_0__3 & 0x0f);
		y = (data->hover_finger_y_4__11 << 4) |
				(data->hover_finger_xy_0__3 >> 4);
		z = HOVER_Z_MAX - data->hover_finger_z;

		input_report_key(prox->prox_dev, BTN_TOUCH, 0);
		input_report_key(prox->prox_dev, BTN_TOOL_FINGER, 1);
		input_report_abs(prox->prox_dev, ABS_X, x);
		input_report_abs(prox->prox_dev, ABS_Y, y);
		input_report_abs(prox->prox_dev, ABS_DISTANCE, z);

		input_sync(prox->prox_dev);

		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: x = %d y = %d z = %d\n",
				__func__, x, y, z);

		prox->finger_present = true;
		prox_finger_lift_timer((unsigned long)prox);
	}

	if (data->air_swipe_det) {
		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: Swipe direction 0 = %d\n",
				__func__, data->air_swipe_dir_0);
		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: Swipe direction 1 = %d\n",
				__func__, data->air_swipe_dir_1);
	}

	if (data->large_obj_det) {
		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: Large object activity = %d\n",
				__func__, data->large_obj_act);
	}

	if (data->hover_pinch_det) {
		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: Hover pinch direction = %d\n",
				__func__, data->hover_pinch_dir);
	}

	if (data->object_present) {
		dev_dbg(&prox->rmi4_data->i2c_client->dev,
				"%s: Object presence detected\n",
				__func__);
	}

	return;
}

static int prox_set_enables(void)
{
	int retval;

	retval = prox->fn_ptr->write(prox->rmi4_data,
			prox->proximity_enables_addr,
			&prox->proximity_enables,
			sizeof(prox->proximity_enables));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to write to proximity_enables\n",
				__func__);
		return retval;
	}

	retval = prox->fn_ptr->write(prox->rmi4_data,
			prox->general_control_addr,
			&prox->general_control,
			sizeof(prox->general_control));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to write to general_control\n",
				__func__);
		return retval;
	}

	return 0;
}

static void prox_set_params(void)
{
	input_set_abs_params(prox->prox_dev, ABS_X, 0,
			prox->rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(prox->prox_dev, ABS_Y, 0,
			prox->rmi4_data->sensor_max_y, 0, 0);
	input_set_abs_params(prox->prox_dev, ABS_DISTANCE, 0,
			HOVER_Z_MAX, 0, 0);

	return;
}

static int prox_scan_pdt(void)
{
	int retval;
	unsigned char ii;
	unsigned char page;
	unsigned char intr_count = 0;
	unsigned char intr_off;
	unsigned char intr_src;
	unsigned short addr;
	struct synaptics_rmi4_fn_desc fd;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
			addr |= (page << 8);

			retval = prox->fn_ptr->read(prox->rmi4_data,
					addr,
					(unsigned char *)&fd,
					sizeof(fd));
			if (retval < 0)
				return retval;

			if (fd.fn_number) {
				dev_dbg(&prox->rmi4_data->i2c_client->dev,
						"%s: Found F%02x\n",
						__func__, fd.fn_number);
				switch (fd.fn_number) {
				case SYNAPTICS_RMI4_F51:
					goto f51_found;
					break;
				}
			} else {
				break;
			}

			intr_count += (fd.intr_src_count & MASK_3BIT);
		}
	}

	dev_err(&prox->rmi4_data->i2c_client->dev,
			"%s: Failed to find F5\n",
			__func__);
	return -EINVAL;

f51_found:
	prox->query_base_addr = fd.query_base_addr | (page << 8);
	prox->control_base_addr = fd.ctrl_base_addr | (page << 8);
	prox->data_base_addr = fd.data_base_addr | (page << 8);
	prox->command_base_addr = fd.cmd_base_addr | (page << 8);

	prox->proximity_enables_addr = prox->control_base_addr +
			F51_PROXIMITY_ENABLES_OFFSET;
	prox->general_control_addr = prox->control_base_addr +
			F51_GENERAL_CONTROL_OFFSET;

	prox->intr_mask = 0;
	intr_src = fd.intr_src_count;
	intr_off = intr_count % 8;
	for (ii = intr_off;
			ii < ((intr_src & MASK_3BIT) +
			intr_off);
			ii++) {
		prox->intr_mask |= 1 << ii;
	}

	prox->rmi4_data->intr_mask[0] |= prox->intr_mask;

	addr = prox->rmi4_data->f01_ctrl_base_addr + 1;

	retval = prox->fn_ptr->write(prox->rmi4_data,
			addr,
			&(prox->rmi4_data->intr_mask[0]),
			sizeof(prox->rmi4_data->intr_mask[0]));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to set interrupt enable bit\n",
				__func__);
		return retval;
	}

	return 0;
}

static ssize_t synaptics_rmi4_prox_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!prox)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			prox->proximity_enables);
}

static ssize_t synaptics_rmi4_prox_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;

	if (!prox)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	prox->proximity_enables = (unsigned char)input;

	retval = prox->fn_ptr->write(prox->rmi4_data,
			prox->proximity_enables_addr,
			&prox->proximity_enables,
			sizeof(prox->proximity_enables));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to write to proximity_enables\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_prox_general_control_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!prox)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			prox->general_control);
}

static ssize_t synaptics_rmi4_prox_general_control_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;

	if (!prox)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	prox->general_control = (unsigned char)input;

	retval = prox->fn_ptr->write(prox->rmi4_data,
			prox->general_control_addr,
			&prox->general_control,
			sizeof(prox->general_control));
	if (retval < 0) {
		dev_err(&prox->rmi4_data->i2c_client->dev,
				"%s: Failed to write to general_control\n",
				__func__);
		return retval;
	}

	return count;
}

int synaptics_rmi4_prox_enables(unsigned char enables)
{
	int retval;

	if (!prox)
		return -ENODEV;

	prox->proximity_enables = enables;

	retval = prox_set_enables();
	if (retval < 0)
		return retval;

	return 0;
}
EXPORT_SYMBOL(synaptics_rmi4_prox_enables);

static void synaptics_rmi4_prox_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (!prox)
		return;

	if (prox->intr_mask & intr_mask)
		prox_finger_report();

	return;
}

static int synaptics_rmi4_prox_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char attr_count;

	prox = kzalloc(sizeof(*prox), GFP_KERNEL);
	if (!prox) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for prox\n",
				__func__);
		retval = -ENOMEM;
		goto exit;
	}

	prox->fn_ptr = kzalloc(sizeof(*(prox->fn_ptr)), GFP_KERNEL);
	if (!prox->fn_ptr) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for fn_ptr\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_prox;
	}

	prox->finger_data = kzalloc(sizeof(*(prox->finger_data)), GFP_KERNEL);
	if (!prox->finger_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for finger_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_fn_ptr;
	}

	prox->rmi4_data = rmi4_data;
	prox->fn_ptr->read = rmi4_data->i2c_read;
	prox->fn_ptr->write = rmi4_data->i2c_write;
	prox->fn_ptr->enable = rmi4_data->irq_enable;

	retval = prox_scan_pdt();
	if (retval < 0)
		goto exit_free_finger_data;

	prox->proximity_enables = F51_PROXIMITY_ENABLES;
	prox->general_control = F51_GENERAL_CONTROL;

	retval = prox_set_enables();
	if (retval < 0)
		return retval;

	prox->prox_dev = input_allocate_device();
	if (prox->prox_dev == NULL) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to allocate proximity device\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_finger_data;
	}

	prox->prox_dev->name = DRIVER_NAME;
	prox->prox_dev->phys = PROX_PHYS_NAME;
	prox->prox_dev->id.product = SYNAPTICS_DSX_DRIVER_PRODUCT;
	prox->prox_dev->id.version = SYNAPTICS_DSX_DRIVER_VERSION;
	prox->prox_dev->id.bustype = BUS_I2C;
	prox->prox_dev->dev.parent = &rmi4_data->i2c_client->dev;
	input_set_drvdata(prox->prox_dev, rmi4_data);

	set_bit(EV_KEY, prox->prox_dev->evbit);
	set_bit(EV_ABS, prox->prox_dev->evbit);
	set_bit(BTN_TOUCH, prox->prox_dev->keybit);
	set_bit(BTN_TOOL_FINGER, prox->prox_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, prox->prox_dev->propbit);
#endif

	prox_set_params();

	setup_timer(&prox->finger_lift_timer,
			prox_finger_lift_timer,
			(unsigned long)prox);

	retval = input_register_device(prox->prox_dev);
	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register proximity device\n",
				__func__);
		goto exit_free_input_device;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto exit_free_sysfs;
		}
	}

	return 0;

exit_free_sysfs:
	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	input_unregister_device(prox->prox_dev);
	prox->prox_dev = NULL;

exit_free_input_device:
	if (prox->prox_dev)
		input_free_device(prox->prox_dev);

exit_free_finger_data:
	kfree(prox->finger_data);

exit_free_fn_ptr:
	kfree(prox->fn_ptr);

exit_free_prox:
	kfree(prox);
	prox = NULL;

exit:
	return retval;
}

static void synaptics_rmi4_prox_remove(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char attr_count;

	if (!prox)
		goto exit;

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	input_unregister_device(prox->prox_dev);
	kfree(prox->finger_data);
	kfree(prox->fn_ptr);
	kfree(prox);
	prox = NULL;

exit:
	complete(&prox_remove_complete);

	return;
}

static void synaptics_rmi4_prox_reset(struct synaptics_rmi4_data *rmi4_data)
{
	if (!prox)
		return;

	prox_finger_lift();

	prox_scan_pdt();

	prox_set_enables();

	prox_set_params();

	return;
}

static void synaptics_rmi4_prox_reinit(struct synaptics_rmi4_data *rmi4_data)
{
	if (!prox)
		return;

	prox_finger_lift();

	prox_set_enables();

	return;
}

static void synaptics_rmi4_prox_e_suspend(struct synaptics_rmi4_data *rmi4_data)
{
	if (!prox)
		return;

	prox_finger_lift();

	return;
}

static void synaptics_rmi4_prox_suspend(struct synaptics_rmi4_data *rmi4_data)
{
	if (!prox)
		return;

	prox_finger_lift();

	return;
}

static struct synaptics_rmi4_exp_fn proximity_module = {
	.fn_type = RMI_PROXIMITY,
	.init = synaptics_rmi4_prox_init,
	.remove = synaptics_rmi4_prox_remove,
	.reset = synaptics_rmi4_prox_reset,
	.reinit = synaptics_rmi4_prox_reinit,
	.early_suspend = synaptics_rmi4_prox_e_suspend,
	.suspend = synaptics_rmi4_prox_suspend,
	.resume = NULL,
	.late_resume = NULL,
	.attn = synaptics_rmi4_prox_attn,
};

static int __init rmi4_proximity_module_init(void)
{
	synaptics_rmi4_new_function(&proximity_module, true);

	return 0;
}

static void __exit rmi4_proximity_module_exit(void)
{
	synaptics_rmi4_new_function(&proximity_module, false);

	wait_for_completion(&prox_remove_complete);

	return;
}

module_init(rmi4_proximity_module_init);
module_exit(rmi4_proximity_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX Proximity Module");
MODULE_LICENSE("GPL v2");
