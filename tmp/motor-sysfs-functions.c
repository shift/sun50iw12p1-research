/* Sysfs attribute functions for Prometheus metrics */

static ssize_t position_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE,
		"# HELP hy300_motor_position_steps Current motor position in steps\n"
		"# TYPE hy300_motor_position_steps gauge\n"
		"hy300_motor_position_steps %d\n"
		"# HELP hy300_motor_max_position_steps Maximum configured position\n"
		"# TYPE hy300_motor_max_position_steps gauge\n"
		"hy300_motor_max_position_steps %d\n"
		"# HELP hy300_motor_homed Motor homing status\n"
		"# TYPE hy300_motor_homed gauge\n"
		"hy300_motor_homed %d\n",
		atomic_read(&motor->metrics.current_position),
		motor->max_position,
		atomic_read(&motor->metrics.homed_status));
}

static ssize_t movement_counters_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE,
		"# HELP hy300_motor_movements_total Total motor movements executed\n"
		"# TYPE hy300_motor_movements_total counter\n"
		"hy300_motor_movements_total %llu\n"
		"# HELP hy300_motor_steps_total Total individual steps taken\n"
		"# TYPE hy300_motor_steps_total counter\n"
		"hy300_motor_steps_total %llu\n"
		"# HELP hy300_motor_position_changes_total Position change commands\n"
		"# TYPE hy300_motor_position_changes_total counter\n"
		"hy300_motor_position_changes_total %llu\n",
		atomic64_read(&motor->metrics.movements_total),
		atomic64_read(&motor->metrics.steps_total),
		atomic64_read(&motor->metrics.position_changes_total));
}

static ssize_t calibration_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE,
		"# HELP hy300_motor_homing_attempts_total Total homing sequence attempts\n"
		"# TYPE hy300_motor_homing_attempts_total counter\n"
		"hy300_motor_homing_attempts_total %llu\n"
		"# HELP hy300_motor_homing_successes_total Successful homing sequences\n"
		"# TYPE hy300_motor_homing_successes_total counter\n"
		"hy300_motor_homing_successes_total %llu\n"
		"# HELP hy300_motor_limit_switch_triggers_total Limit switch activations\n"
		"# TYPE hy300_motor_limit_switch_triggers_total counter\n"
		"hy300_motor_limit_switch_triggers_total %llu\n",
		atomic64_read(&motor->metrics.homing_attempts_total),
		atomic64_read(&motor->metrics.homing_successes_total),
		atomic64_read(&motor->metrics.limit_switch_triggers_total));
}

static ssize_t gpio_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hy300_motor *motor = dev_get_drvdata(dev);
	
	return scnprintf(buf, PAGE_SIZE,
		"# HELP hy300_motor_gpio_phase_transitions_total GPIO phase state transitions\n"
		"# TYPE hy300_motor_gpio_phase_transitions_total counter\n"
		"hy300_motor_gpio_phase_transitions_total %llu\n"
		"# HELP hy300_motor_step_errors_total Failed step operations\n"
		"# TYPE hy300_motor_step_errors_total counter\n"
		"hy300_motor_step_errors_total %llu\n"
		"# HELP hy300_motor_phase_delay_us_configured Phase delay timing configuration\n"
		"# TYPE hy300_motor_phase_delay_us_configured gauge\n"
		"hy300_motor_phase_delay_us_configured %d\n"
		"# HELP hy300_motor_step_delay_ms_configured Step delay timing configuration\n"
		"# TYPE hy300_motor_step_delay_ms_configured gauge\n"
		"hy300_motor_step_delay_ms_configured %d\n",
		atomic64_read(&motor->metrics.gpio_phase_transitions_total),
		atomic64_read(&motor->metrics.step_errors_total),
		motor->phase_delay_us,
		motor->step_delay_ms);
}

/* Prometheus metrics device attributes */
static DEVICE_ATTR_RO(position_status);
static DEVICE_ATTR_RO(movement_counters);
static DEVICE_ATTR_RO(calibration_state);
static DEVICE_ATTR_RO(gpio_status);

static struct attribute *motor_metrics_attrs[] = {
	&dev_attr_position_status.attr,
	&dev_attr_movement_counters.attr,
	&dev_attr_calibration_state.attr,
	&dev_attr_gpio_status.attr,
	NULL,
};

static const struct attribute_group motor_metrics_attr_group = {
	.attrs = motor_metrics_attrs,
};

static const struct attribute_group *motor_metrics_attr_groups[] = {
	&motor_metrics_attr_group,
	NULL,
};

/* Forward declaration for interrupt handler */
static irqreturn_t hy300_motor_limit_isr(int irq, void *dev_id);