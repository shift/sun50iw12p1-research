// SPDX-License-Identifier: GPL-2.0+
/*
 * HY300 Keystone Motor Control Driver
 *
 * Controls the electronic keystone correction stepper motor for the HY300 projector.
 * Based on reverse-engineered factory firmware motor control sequences.
 *
 * Copyright (C) 2025 HY300 Linux Porting Project
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/ktime.h>

/* Include motor control sequences from reverse engineering */
#include "hy300-motor-control.h"

#define DRIVER_NAME "hy300-keystone-motor"
#define DRIVER_VERSION "1.1"

/* External reference to hy300_class from MIPS loader */
extern struct class *hy300_class;

/**
 * struct hy300_motor_metrics - Motor metrics data for Prometheus export
 * @total_movements: Total number of movements executed
 * @total_steps: Total number of individual steps executed
 * @direction_changes: Number of direction changes
 * @homing_operations: Number of homing operations performed
 * @calibration_events: Number of calibration events
 * @limit_switch_triggers: Number of limit switch activations
 * @gpio_phase_changes: Total GPIO phase changes
 * @last_step_duration_ns: Duration of last step execution
 * @last_movement_duration_ns: Duration of last movement operation
 * @average_step_timing_ns: Running average of step timing
 * @error_count: Number of errors encountered
 */
struct hy300_motor_metrics {
	u64 total_movements;
	u64 total_steps;
	u64 direction_changes;
	u64 homing_operations;
	u64 calibration_events;
	u64 limit_switch_triggers;
	u64 gpio_phase_changes;
	u64 last_step_duration_ns;
	u64 last_movement_duration_ns;
	u64 average_step_timing_ns;
	u64 error_count;
	int last_direction;
};

/**
 * struct hy300_motor - Motor control device data
 * @dev: Device pointer
 * @phase_gpios: Array of GPIO descriptors for motor phases (PH4-PH7)
 * @limit_gpio: GPIO descriptor for limit switch (PH14)
 * @limit_irq: IRQ number for limit switch
 * @position: Current motor position (steps from home)
 * @max_position: Maximum allowed position (steps)
 * @step_delay_ms: Delay between steps (from DTB)
 * @phase_delay_us: Delay between phase changes (from DTB)
 * @motor_lock: Mutex to protect motor operations
 * @step_work: Work structure for stepping operations
 * @target_position: Target position for background stepping
 * @homed: Flag indicating if motor has been homed
 * @metrics: Prometheus metrics data
 * @metrics_dev: Device for metrics sysfs interface
 */
struct hy300_motor {
	struct device *dev;
	struct gpio_desc *phase_gpios[MOTOR_PHASE_GPIO_COUNT];
	struct gpio_desc *limit_gpio;
	int limit_irq;
	
	int position;
	int max_position;
	int step_delay_ms;
	int phase_delay_us;
	
	struct mutex motor_lock;
	struct work_struct step_work;
	int target_position;
	bool homed;
	
	struct hy300_motor_metrics metrics;
	struct device *metrics_dev;
};

/**
 * hy300_motor_set_all_phases - Set all motor phases to off state
 * @motor: Motor control device
 */
static void hy300_motor_set_all_phases(struct hy300_motor *motor, bool state)
{
	int i;
	
	for (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
		gpiod_set_value(motor->phase_gpios[i], state ? 1 : 0);
	}
}

/**
 * hy300_motor_step - Execute one motor step (from generated code)
 * @motor: Motor control device
 * @direction: Direction (0=CW, 1=CCW)
 * @step_num: Step number in sequence
 */
static void hy300_motor_step(struct hy300_motor *motor, int direction, int step_num)
{
	const u8 *sequence = direction ? motor_ccw_sequence : motor_cw_sequence;
	int max_steps = direction ? ARRAY_SIZE(motor_ccw_sequence) : ARRAY_SIZE(motor_cw_sequence);
	u8 phase_pattern;
	int i;
	ktime_t start_time, end_time;

	if (step_num >= max_steps) {
		dev_err(motor->dev, "Step number %d exceeds maximum %d\n", step_num, max_steps);
		motor->metrics.error_count++;
		return;
	}

	start_time = ktime_get();
	
	phase_pattern = sequence[step_num];

	/* Set GPIO states for each phase */
	for (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
		int gpio_state = (phase_pattern >> i) & 1;
		gpiod_set_value(motor->phase_gpios[i], gpio_state);
		motor->metrics.gpio_phase_changes++;
	}

	/* Phase change delay */
	udelay(motor->phase_delay_us);
	
	end_time = ktime_get();
	motor->metrics.last_step_duration_ns = ktime_to_ns(ktime_sub(end_time, start_time));
	
	/* Update running average (simple moving average) */
	if (motor->metrics.total_steps > 0) {
		motor->metrics.average_step_timing_ns = 
			(motor->metrics.average_step_timing_ns * 7 + motor->metrics.last_step_duration_ns) / 8;
	} else {
		motor->metrics.average_step_timing_ns = motor->metrics.last_step_duration_ns;
	}
	
	motor->metrics.total_steps++;
	
	/* Track direction changes */
	if (motor->metrics.last_direction != -1 && motor->metrics.last_direction != direction) {
		motor->metrics.direction_changes++;
	}
	motor->metrics.last_direction = direction;
}

/**
 * hy300_motor_move_steps - Move motor by specified number of steps
 * @motor: Motor control device
 * @steps: Number of steps to move (positive = CW, negative = CCW)
 */
static int hy300_motor_move_steps(struct hy300_motor *motor, int steps)
{
	int direction = (steps < 0) ? 1 : 0;  /* 1 = CCW, 0 = CW */
	int abs_steps = abs(steps);
	int sequence_pos = 0;
	int i;
	ktime_t start_time, end_time;
	
	dev_dbg(motor->dev, "Moving %d steps (direction=%s)\n", 
		abs_steps, direction ? "CCW" : "CW");
	
	start_time = ktime_get();
	
	for (i = 0; i < abs_steps; i++) {
		/* Check limit switch for CCW movement (negative steps) */
		if (direction == 1 && gpiod_get_value(motor->limit_gpio)) {
			dev_info(motor->dev, "Limit switch activated, stopping at step %d\n", i);
			motor->position = 0;  /* We're at home position */
			motor->homed = true;
			motor->metrics.limit_switch_triggers++;
			break;
		}
		
		/* Execute one step in 16-step sequence */
		hy300_motor_step(motor, direction, sequence_pos);
		
		/* Move to next step in sequence (wrap around) */
		sequence_pos = (sequence_pos + 1) % MOTOR_STEPS_PER_REVOLUTION;
		
		/* Update position */
		motor->position += direction ? -1 : 1;
		
		/* Inter-step delay */
		if (motor->step_delay_ms > 0) {
			msleep(motor->step_delay_ms);
		}
	}
	
	end_time = ktime_get();
	motor->metrics.last_movement_duration_ns = ktime_to_ns(ktime_sub(end_time, start_time));
	motor->metrics.total_movements++;
	
	/* Turn off all phases after movement */
	hy300_motor_set_all_phases(motor, false);
	
	return i;  /* Return number of steps actually moved */
}

/**
 * hy300_motor_home - Home the motor to limit switch
 * @motor: Motor control device
 */
static int hy300_motor_home(struct hy300_motor *motor)
{
	int steps_moved;
	
	dev_info(motor->dev, "Homing motor to limit switch\n");
	
	motor->metrics.homing_operations++;
	
	/* Move towards limit switch (CCW direction) */
	/* Use a large negative number to ensure we reach the limit */
	steps_moved = hy300_motor_move_steps(motor, -1000);
	
	if (motor->homed) {
		dev_info(motor->dev, "Motor homed successfully after %d steps\n", steps_moved);
		motor->metrics.calibration_events++;
		return 0;
	} else {
		dev_err(motor->dev, "Failed to reach limit switch during homing\n");
		motor->metrics.error_count++;
		return -EIO;
	}
}

/**
 * hy300_motor_set_position - Move motor to absolute position
 * @motor: Motor control device
 * @position: Target position (0 = home, positive = CW from home)
 */
static int hy300_motor_set_position(struct hy300_motor *motor, int position)
{
	int steps_needed;
	int result;
	
	if (!motor->homed) {
		dev_warn(motor->dev, "Motor not homed, homing first\n");
		result = hy300_motor_home(motor);
		if (result < 0)
			return result;
	}
	
	if (position < 0 || position > motor->max_position) {
		dev_err(motor->dev, "Position %d outside valid range (0-%d)\n", 
			position, motor->max_position);
		motor->metrics.error_count++;
		return -EINVAL;
	}
	
	steps_needed = position - motor->position;
	if (steps_needed == 0) {
		dev_dbg(motor->dev, "Already at target position %d\n", position);
		return 0;
	}
	
	dev_info(motor->dev, "Moving from position %d to %d (%d steps)\n",
		motor->position, position, steps_needed);
	
	hy300_motor_move_steps(motor, steps_needed);
	
	return 0;
}

/**
 * hy300_motor_limit_isr - Interrupt handler for limit switch
 * @irq: IRQ number
 * @data: Motor device pointer
 */
static irqreturn_t hy300_motor_limit_isr(int irq, void *data)
{
	struct hy300_motor *motor = data;
	
	dev_info(motor->dev, "Limit switch triggered\n");
	
	motor->metrics.limit_switch_triggers++;
	
	/* Emergency stop: turn off all phases immediately */
	hy300_motor_set_all_phases(motor, false);
	
	/* Mark as homed if we were moving towards home */
	if (motor->position < 0) {
		motor->position = 0;
		motor->homed = true;
	}
	
	return IRQ_HANDLED;
}

/*
 * Prometheus metrics sysfs attributes
 */

static ssize_t position_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev->parent);
	
	return sprintf(buf,
		"# HELP hy300_motor_position_steps Current motor position in steps\n"
		"# TYPE hy300_motor_position_steps gauge\n"
		"hy300_motor_position_steps %d\n"
		"# HELP hy300_motor_target_position Target motor position in steps\n"
		"# TYPE hy300_motor_target_position gauge\n"
		"hy300_motor_target_position %d\n"
		"# HELP hy300_motor_max_position Maximum allowed motor position\n"
		"# TYPE hy300_motor_max_position gauge\n"
		"hy300_motor_max_position %d\n"
		"# HELP hy300_motor_homed Motor homing status\n"
		"# TYPE hy300_motor_homed gauge\n"
		"hy300_motor_homed %d\n",
		motor->position, motor->target_position, motor->max_position, motor->homed ? 1 : 0);
}

static ssize_t calibration_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev->parent);
	
	return sprintf(buf,
		"# HELP hy300_motor_homing_operations_total Number of homing operations performed\n"
		"# TYPE hy300_motor_homing_operations_total counter\n"
		"hy300_motor_homing_operations_total %llu\n"
		"# HELP hy300_motor_calibration_events_total Number of calibration events\n"
		"# TYPE hy300_motor_calibration_events_total counter\n"
		"hy300_motor_calibration_events_total %llu\n"
		"# HELP hy300_motor_limit_switch_triggers_total Number of limit switch activations\n"
		"# TYPE hy300_motor_limit_switch_triggers_total counter\n"
		"hy300_motor_limit_switch_triggers_total %llu\n",
		motor->metrics.homing_operations, motor->metrics.calibration_events,
		motor->metrics.limit_switch_triggers);
}

static ssize_t movement_counters_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev->parent);
	
	return sprintf(buf,
		"# HELP hy300_motor_movements_total Total motor movements executed\n"
		"# TYPE hy300_motor_movements_total counter\n"
		"hy300_motor_movements_total %llu\n"
		"# HELP hy300_motor_steps_total Total individual steps executed\n"
		"# TYPE hy300_motor_steps_total counter\n"
		"hy300_motor_steps_total %llu\n"
		"# HELP hy300_motor_direction_changes_total Number of direction changes\n"
		"# TYPE hy300_motor_direction_changes_total counter\n"
		"hy300_motor_direction_changes_total %llu\n",
		motor->metrics.total_movements, motor->metrics.total_steps,
		motor->metrics.direction_changes);
}

static ssize_t gpio_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev->parent);
	int i, pos = 0;
	
	pos += sprintf(buf + pos,
		"# HELP hy300_motor_gpio_phase_changes_total Total GPIO phase changes\n"
		"# TYPE hy300_motor_gpio_phase_changes_total counter\n"
		"hy300_motor_gpio_phase_changes_total %llu\n",
		motor->metrics.gpio_phase_changes);
	
	for (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
		int gpio_value = gpiod_get_value(motor->phase_gpios[i]);
		pos += sprintf(buf + pos,
			"# HELP hy300_motor_gpio_phase%d_state Current state of GPIO phase %d\n"
			"# TYPE hy300_motor_gpio_phase%d_state gauge\n"
			"hy300_motor_gpio_phase%d_state %d\n",
			i, i, i, i, gpio_value);
	}
	
	pos += sprintf(buf + pos,
		"# HELP hy300_motor_limit_switch_state Current limit switch state\n"
		"# TYPE hy300_motor_limit_switch_state gauge\n"
		"hy300_motor_limit_switch_state %d\n",
		gpiod_get_value(motor->limit_gpio));
	
	return pos;
}

static ssize_t timing_metrics_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev->parent);
	
	return sprintf(buf,
		"# HELP hy300_motor_last_step_duration_ns Duration of last step in nanoseconds\n"
		"# TYPE hy300_motor_last_step_duration_ns gauge\n"
		"hy300_motor_last_step_duration_ns %llu\n"
		"# HELP hy300_motor_last_movement_duration_ns Duration of last movement in nanoseconds\n"
		"# TYPE hy300_motor_last_movement_duration_ns gauge\n"
		"hy300_motor_last_movement_duration_ns %llu\n"
		"# HELP hy300_motor_average_step_timing_ns Running average step timing in nanoseconds\n"
		"# TYPE hy300_motor_average_step_timing_ns gauge\n"
		"hy300_motor_average_step_timing_ns %llu\n"
		"# HELP hy300_motor_step_delay_ms Configured step delay in milliseconds\n"
		"# TYPE hy300_motor_step_delay_ms gauge\n"
		"hy300_motor_step_delay_ms %d\n"
		"# HELP hy300_motor_phase_delay_us Configured phase delay in microseconds\n"
		"# TYPE hy300_motor_phase_delay_us gauge\n"
		"hy300_motor_phase_delay_us %d\n"
		"# HELP hy300_motor_errors_total Number of errors encountered\n"
		"# TYPE hy300_motor_errors_total counter\n"
		"hy300_motor_errors_total %llu\n",
		motor->metrics.last_step_duration_ns, motor->metrics.last_movement_duration_ns,
		motor->metrics.average_step_timing_ns, motor->step_delay_ms,
		motor->phase_delay_us, motor->metrics.error_count);
}

static DEVICE_ATTR_RO(position_status);
static DEVICE_ATTR_RO(calibration_state);
static DEVICE_ATTR_RO(movement_counters);
static DEVICE_ATTR_RO(gpio_status);
static DEVICE_ATTR_RO(timing_metrics);

static struct attribute *hy300_motor_metrics_attrs[] = {
	&dev_attr_position_status.attr,
	&dev_attr_calibration_state.attr,
	&dev_attr_movement_counters.attr,
	&dev_attr_gpio_status.attr,
	&dev_attr_timing_metrics.attr,
	NULL,
};

static struct attribute_group hy300_motor_metrics_group = {
	.attrs = hy300_motor_metrics_attrs,
};

static const struct attribute_group *hy300_motor_metrics_groups[] = {
	&hy300_motor_metrics_group,
	NULL,
};

/*
 * Sysfs attributes for userspace control (existing functionality)
 */

static ssize_t position_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", motor->position);
}

static ssize_t position_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	int position, result;
	
	if (kstrtoint(buf, 0, &position))
		return -EINVAL;
	
	mutex_lock(&motor->motor_lock);
	result = hy300_motor_set_position(motor, position);
	mutex_unlock(&motor->motor_lock);
	
	return result < 0 ? result : count;
}

static ssize_t home_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	int result;
	
	mutex_lock(&motor->motor_lock);
	result = hy300_motor_home(motor);
	mutex_unlock(&motor->motor_lock);
	
	return result < 0 ? result : count;
}

static ssize_t max_position_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", motor->max_position);
}

static ssize_t homed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", motor->homed ? 1 : 0);
}

static DEVICE_ATTR_RW(position);
static DEVICE_ATTR_WO(home);
static DEVICE_ATTR_RO(max_position);
static DEVICE_ATTR_RO(homed);

static struct attribute *hy300_motor_attrs[] = {
	&dev_attr_position.attr,
	&dev_attr_home.attr,
	&dev_attr_max_position.attr,
	&dev_attr_homed.attr,
	NULL,
};

ATTRIBUTE_GROUPS(hy300_motor);

/**
 * hy300_motor_probe - Platform driver probe function
 * @pdev: Platform device
 */
static int hy300_motor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct hy300_motor *motor;
	int i, ret;
	u32 val;
	
	dev_info(dev, "HY300 Keystone Motor Driver v%s\n", DRIVER_VERSION);
	
	motor = devm_kzalloc(dev, sizeof(*motor), GFP_KERNEL);
	if (!motor)
		return -ENOMEM;
	
	motor->dev = dev;
	mutex_init(&motor->motor_lock);
	
	/* Initialize metrics */
	memset(&motor->metrics, 0, sizeof(motor->metrics));
	motor->metrics.last_direction = -1;  /* No previous direction */
	
	/* Get motor phase GPIOs */
	for (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
		motor->phase_gpios[i] = devm_gpiod_get_index(dev, "phase", i, GPIOD_OUT_LOW);
		if (IS_ERR(motor->phase_gpios[i])) {
			dev_err(dev, "Failed to get phase GPIO %d: %ld\n", 
				i, PTR_ERR(motor->phase_gpios[i]));
			return PTR_ERR(motor->phase_gpios[i]);
		}
	}
	
	/* Get limit switch GPIO */
	motor->limit_gpio = devm_gpiod_get(dev, "limit", GPIOD_IN);
	if (IS_ERR(motor->limit_gpio)) {
		dev_err(dev, "Failed to get limit GPIO: %ld\n", PTR_ERR(motor->limit_gpio));
		return PTR_ERR(motor->limit_gpio);
	}
	
	/* Set up limit switch interrupt */
	motor->limit_irq = gpiod_to_irq(motor->limit_gpio);
	if (motor->limit_irq < 0) {
		dev_err(dev, "Failed to get limit switch IRQ: %d\n", motor->limit_irq);
		return motor->limit_irq;
	}
	
	ret = devm_request_irq(dev, motor->limit_irq, hy300_motor_limit_isr,
			       IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			       "hy300-motor-limit", motor);
	if (ret) {
		dev_err(dev, "Failed to request limit switch IRQ: %d\n", ret);
		return ret;
	}
	
	/* Get timing parameters from device tree */
	motor->phase_delay_us = MOTOR_PHASE_DELAY_US;  /* Default */
	motor->step_delay_ms = MOTOR_STEP_DELAY_MS;    /* Default */
	
	if (!of_property_read_u32(np, "phase-delay-us", &val))
		motor->phase_delay_us = val;
	
	if (!of_property_read_u32(np, "step-delay-ms", &val))
		motor->step_delay_ms = val;
	
	/* Set maximum position (default to reasonable value) */
	motor->max_position = 1000;  /* Allow 1000 steps from home */
	if (!of_property_read_u32(np, "max-position", &val))
		motor->max_position = val;
	
	dev_info(dev, "Motor configured: phase_delay=%dus, step_delay=%dms, max_pos=%d\n",
		motor->phase_delay_us, motor->step_delay_ms, motor->max_position);
	
	/* Initialize motor position */
	motor->position = 0;
	motor->homed = false;
	
	/* Ensure all phases are off initially */
	hy300_motor_set_all_phases(motor, false);
	
	platform_set_drvdata(pdev, motor);
	
	/* Create metrics device in hy300 class */
	if (hy300_class) {
		motor->metrics_dev = device_create_with_groups(hy300_class, dev,
						      MKDEV(0, 0), motor,
						      hy300_motor_metrics_groups,
						      "motor");
		if (IS_ERR(motor->metrics_dev)) {
			dev_warn(dev, "Failed to create metrics device: %ld\n",
				 PTR_ERR(motor->metrics_dev));
			motor->metrics_dev = NULL;
		} else {
			dev_info(dev, "Motor metrics available at /sys/class/hy300/motor/\n");
		}
	} else {
		dev_warn(dev, "hy300_class not available, metrics device not created\n");
	}
	
	dev_info(dev, "HY300 motor driver loaded successfully\n");
	
	return 0;
}

/**
 * hy300_motor_remove - Platform driver remove function
 * @pdev: Platform device
 */
static int hy300_motor_remove(struct platform_device *pdev)
{
	struct hy300_motor *motor = platform_get_drvdata(pdev);
	
	/* Clean up metrics device */
	if (motor->metrics_dev && hy300_class) {
		device_destroy(hy300_class, MKDEV(0, 0));
	}
	
	/* Turn off all motor phases */
	hy300_motor_set_all_phases(motor, false);
	
	dev_info(&pdev->dev, "HY300 motor driver removed\n");
	
	return 0;
}

static const struct of_device_id hy300_motor_of_match[] = {
	{ .compatible = "hy300,keystone-motor", },
	{ }
};
MODULE_DEVICE_TABLE(of, hy300_motor_of_match);

static struct platform_driver hy300_motor_driver = {
	.probe = hy300_motor_probe,
	.remove = hy300_motor_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = hy300_motor_of_match,
		.dev_groups = hy300_motor_groups,
	},
};

module_platform_driver(hy300_motor_driver);

MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_DESCRIPTION("HY300 Keystone Motor Control Driver with Prometheus Metrics");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);