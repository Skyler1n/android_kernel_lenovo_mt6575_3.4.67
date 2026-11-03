#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>
#include <cust_tmd2771.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num = 0,
    .polling_mode_ps = CUST_TMD2771_POLLING_MODE_PS,
    .polling_mode_als = CUST_TMD2771_POLLING_MODE_ALS,
    .power_id = MT65XX_POWER_NONE,
    .power_vol = VOL_DEFAULT,
    .i2c_addr = {CUST_TMD2771_I2C_ADDR, 0x00, 0x00, 0x00},
    .als_level = CUST_TMD2771_ALS_LEVEL,
    .als_value = CUST_TMD2771_ALS_VALUE,
    .ps_threshold_high = CUST_TMD2771_PS_THRESHOLD_HIGH,
    .ps_threshold_low = CUST_TMD2771_PS_THRESHOLD_LOW,
    .ps_threshold = CUST_TMD2771_PS_THRESHOLD,
};

struct alsps_hw *get_cust_alsps_hw(void)
{
    return &cust_alsps_hw;
}

/* A690 validated register values. New common driver exposes them as data. */
int TMD2771_CMM_PPCOUNT_VALUE = CUST_TMD2771_PPCOUNT;
int TMD2771_CMM_CONTROL_VALUE = CUST_TMD2771_CONTROL;
/* Preserve the working 3.0 multiplier: (400*100*40)/(1*1500) ==
 * the 3.4 driver's (400*100*4)/(1*150). */
int ZOOM_TIME = 4;
