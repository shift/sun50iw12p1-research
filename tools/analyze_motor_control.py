#!/usr/bin/env python3
"""
Motor Control Analysis Tool - HY300 Project
Analyzes factory motor control sequences and generates Linux driver implementation.

Based on factory DTB motor control sequences:
- motor-cw-table: <0x9080c04 0x6020301>
- motor-ccw-table: <0x1030206 0x40c0809>

Each sequence controls 4 GPIO phases: PH4, PH5, PH6, PH7
"""

def decode_motor_sequence(hex_values, label):
    """
    Decode motor control sequence from DTB hex values.
    Each 32-bit value contains 8 steps of 4-bit phase patterns.
    
    Args:
        hex_values: List of hex values from DTB
        label: Description of the sequence (CW/CCW)
    """
    print(f"\n=== {label} Motor Sequence Analysis ===")
    
    all_steps = []
    
    for i, hex_val in enumerate(hex_values):
        print(f"\nHex Value {i+1}: 0x{hex_val:08x}")
        
        # Extract 8 steps from 32-bit value (4 bits per step)
        for step in range(8):
            # Extract 4 bits for this step (rightmost 4 bits first)
            phase_pattern = (hex_val >> (step * 4)) & 0xF
            
            # Convert to individual GPIO states
            ph4 = (phase_pattern >> 0) & 1  # Bit 0 -> PH4
            ph5 = (phase_pattern >> 1) & 1  # Bit 1 -> PH5  
            ph6 = (phase_pattern >> 2) & 1  # Bit 2 -> PH6
            ph7 = (phase_pattern >> 3) & 1  # Bit 3 -> PH7
            
            step_global = i * 8 + step
            all_steps.append((ph4, ph5, ph6, ph7))
            
            print(f"  Step {step_global:2d}: 0x{phase_pattern:X} -> PH4={ph4} PH5={ph5} PH6={ph6} PH7={ph7}")
    
    return all_steps

def analyze_stepper_pattern(steps, label):
    """Analyze stepper motor pattern for electrical characteristics."""
    print(f"\n=== {label} Pattern Analysis ===")
    
    # Count phases that are active in each step
    active_phases = []
    for step, (ph4, ph5, ph6, ph7) in enumerate(steps):
        active_count = sum([ph4, ph5, ph6, ph7])
        active_phases.append(active_count)
        
        # Identify which phases are active
        active_list = []
        if ph4: active_list.append("PH4")
        if ph5: active_list.append("PH5") 
        if ph6: active_list.append("PH6")
        if ph7: active_list.append("PH7")
        
        active_str = ", ".join(active_list) if active_list else "None"
        print(f"  Step {step:2d}: {active_count} phases active ({active_str})")
    
    # Determine stepper motor type
    max_active = max(active_phases)
    min_active = min(active_phases)
    
    print(f"\nMotor Type Analysis:")
    print(f"  Max simultaneous phases: {max_active}")
    print(f"  Min simultaneous phases: {min_active}")
    
    if max_active == 1:
        print("  -> Single-phase stepping (wave drive)")
    elif max_active == 2 and min_active == 1:
        print("  -> Mixed single/dual phase stepping")
    elif max_active == 2 and min_active == 2:
        print("  -> Dual-phase stepping (full-step)")
    else:
        print("  -> Complex stepping pattern")
    
    return len(steps)

def generate_linux_driver_code(cw_steps, ccw_steps):
    """Generate Linux kernel driver code for motor control."""
    
    print(f"\n=== Linux Driver Code Generation ===")
    
    # Generate C arrays for stepping sequences
    cw_array = "static const u8 motor_cw_sequence[] = {\n"
    for i, (ph4, ph5, ph6, ph7) in enumerate(cw_steps):
        # Pack 4 GPIO states into a single byte
        step_byte = ph4 | (ph5 << 1) | (ph6 << 2) | (ph7 << 3)
        cw_array += f"\t0x{step_byte:02x},"
        if (i + 1) % 8 == 0:
            cw_array += f"  /* Steps {i-7:2d}-{i:2d} */\n"
        else:
            cw_array += " "
    
    if len(cw_steps) % 8 != 0:
        cw_array += f"  /* Steps {(len(cw_steps)//8)*8}-{len(cw_steps)-1} */\n"
    cw_array += "};\n"
    
    ccw_array = "static const u8 motor_ccw_sequence[] = {\n"
    for i, (ph4, ph5, ph6, ph7) in enumerate(ccw_steps):
        step_byte = ph4 | (ph5 << 1) | (ph6 << 2) | (ph7 << 3)
        ccw_array += f"\t0x{step_byte:02x},"
        if (i + 1) % 8 == 0:
            ccw_array += f"  /* Steps {i-7:2d}-{i:2d} */\n"
        else:
            ccw_array += " "
    
    if len(ccw_steps) % 8 != 0:
        ccw_array += f"  /* Steps {(len(ccw_steps)//8)*8}-{len(ccw_steps)-1} */\n"
    ccw_array += "};\n"
    
    # Generate driver constants
    driver_constants = f"""
/* Motor control constants */
#define MOTOR_STEPS_PER_REVOLUTION {max(len(cw_steps), len(ccw_steps))}
#define MOTOR_PHASE_DELAY_US 1000    /* From DTB: phase-delay-us */
#define MOTOR_STEP_DELAY_MS 2        /* From DTB: step-delay-ms */

/* GPIO phase mappings (from DTB) */
#define MOTOR_PHASE_GPIO_COUNT 4
static const char * const motor_phase_names[] = {{
\t"phase0-gpio", /* PH4 */
\t"phase1-gpio", /* PH5 */
\t"phase2-gpio", /* PH6 */
\t"phase3-gpio", /* PH7 */
}};
"""
    
    # Generate step function
    step_function = """
/**
 * hy300_motor_step - Execute one motor step
 * @motor: Motor control device
 * @direction: Direction (0=CW, 1=CCW)
 * @step_num: Step number in sequence
 */
static void hy300_motor_step(struct hy300_motor *motor, int direction, int step_num)
{
\tconst u8 *sequence = direction ? motor_ccw_sequence : motor_cw_sequence;
\tint max_steps = direction ? ARRAY_SIZE(motor_ccw_sequence) : ARRAY_SIZE(motor_cw_sequence);
\tu8 phase_pattern;
\tint i;

\tif (step_num >= max_steps) {
\t\tdev_err(motor->dev, "Step number %d exceeds maximum %d\\n", step_num, max_steps);
\t\treturn;
\t}

\tphase_pattern = sequence[step_num];

\t/* Set GPIO states for each phase */
\tfor (i = 0; i < MOTOR_PHASE_GPIO_COUNT; i++) {
\t\tint gpio_state = (phase_pattern >> i) & 1;
\t\tgpiod_set_value(motor->phase_gpios[i], gpio_state);
\t}

\t/* Phase change delay */
\tudelay(MOTOR_PHASE_DELAY_US);
}
"""
    
    print("Generated Linux driver code components:")
    print("1. Motor stepping sequences (cw_sequence, ccw_sequence)")
    print("2. Driver constants (timing, GPIO mappings)")
    print("3. Step execution function (hy300_motor_step)")
    
    return {
        'cw_array': cw_array,
        'ccw_array': ccw_array,
        'constants': driver_constants,
        'step_function': step_function
    }

def main():
    """Main analysis function."""
    print("HY300 Motor Control Analysis")
    print("=" * 50)
    
    # Factory DTB motor control sequences
    # motor-cw-table: <0x9080c04 0x6020301>
    # motor-ccw-table: <0x1030206 0x40c0809>
    
    cw_hex = [0x9080c04, 0x6020301]
    ccw_hex = [0x1030206, 0x40c0809]
    
    # Decode sequences
    cw_steps = decode_motor_sequence(cw_hex, "Clockwise (CW)")
    ccw_steps = decode_motor_sequence(ccw_hex, "Counter-Clockwise (CCW)")
    
    # Analyze patterns  
    cw_step_count = analyze_stepper_pattern(cw_steps, "Clockwise")
    ccw_step_count = analyze_stepper_pattern(ccw_steps, "Counter-Clockwise")
    
    print(f"\n=== Summary ===")
    print(f"Clockwise sequence: {cw_step_count} steps")
    print(f"Counter-clockwise sequence: {ccw_step_count} steps")
    
    if cw_step_count == ccw_step_count:
        print(f"-> Symmetric stepping pattern ({cw_step_count} steps per direction)")
    
    # Generate Linux driver code
    driver_code = generate_linux_driver_code(cw_steps, ccw_steps)
    
    # Write driver code to file
    driver_file = "/home/shift/code/android_projector/drivers/misc/hy300-motor-control.h"
    with open(driver_file, 'w') as f:
        f.write("/* HY300 Motor Control Driver - Generated Code */\n")
        f.write("/* Based on factory DTB motor control sequences */\n\n")
        f.write(driver_code['constants'])
        f.write("\n")
        f.write(driver_code['cw_array'])
        f.write("\n")
        f.write(driver_code['ccw_array'])
        f.write("\n")
        f.write(driver_code['step_function'])
    
    print(f"\nGenerated driver header: {driver_file}")
    print("Driver code ready for integration into Linux kernel module.")

if __name__ == "__main__":
    main()