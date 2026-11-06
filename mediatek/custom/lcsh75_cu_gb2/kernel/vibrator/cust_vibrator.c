#include <cust_vibrator.h>
#include <linux/types.h>

static struct vibrator_hw cust_vibrator_hw = {
	/* Match ICS: honor Android short haptic durations without a 50 ms floor. */
	.vib_timer = 10,
	.vib_timer_min = 10,
};

struct vibrator_hw *get_cust_vibrator_hw(void)
{
    return &cust_vibrator_hw;
}
