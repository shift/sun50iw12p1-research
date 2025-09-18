/* HY300 Motor Control Driver - Generated Code */
/* Based on factory DTB motor control sequences */


/* Motor control constants */
#define MOTOR_STEPS_PER_REVOLUTION 16
#define MOTOR_PHASE_DELAY_US 1000    /* From DTB: phase-delay-us */
#define MOTOR_STEP_DELAY_MS 2        /* From DTB: step-delay-ms */

/* GPIO phase mappings (from DTB) */
#define MOTOR_PHASE_GPIO_COUNT 4
static const char * const motor_phase_names[] = {
	"phase0-gpio", /* PH4 */
	"phase1-gpio", /* PH5 */
	"phase2-gpio", /* PH6 */
	"phase3-gpio", /* PH7 */
};

static const u8 motor_cw_sequence[] = {
	0x04, 	0x00, 	0x0c, 	0x00, 	0x08, 	0x00, 	0x09, 	0x00,  /* Steps  0- 7 */
	0x01, 	0x00, 	0x03, 	0x00, 	0x02, 	0x00, 	0x06, 	0x00,  /* Steps  8-15 */
};

static const u8 motor_ccw_sequence[] = {
	0x06, 	0x00, 	0x02, 	0x00, 	0x03, 	0x00, 	0x01, 	0x00,  /* Steps  0- 7 */
	0x09, 	0x00, 	0x08, 	0x00, 	0x0c, 	0x00, 	0x04, 	0x00,  /* Steps  8-15 */
};


/**
 * hy300_motor_step - Execute one motor step
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

	if (step_num >= max_steps) {
		dev_err(motor->dev, "Step number %d exceeds maximum %d\n", step_num, max_steps);
		return;
	}

	phase_pattern = sequence[step_num];

	/* Set GPIO states for each phase */
	for (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
		int gpio_state = (phase_pattern >> i) & 1;
		gpiod_set_value(motor->phase_gpios[i], gpio_state);
	}

	/* Phase change delay */
	udelay(MOTOR_PHASE_DELAY_US);
}
