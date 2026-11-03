/*
 * Lenovo A690 HI351 YUV sensor, reconstructed from the working official
 * Android 2.3.6 kernel (Oct 18 2012). I2C ABI and sensor registration use
 * the existing MT6575 framework. Register programming and timing originate
 * from the official binary, not the unrelated imported HI351 driver.
 * Evidence: Work/hi-camera/03-ida/official-gb in the analysis workspace.
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#define HI351_ID 0xa4
#define HI351_I2C_ID 0x40
#define HI351_LOG(fmt, args...) printk(KERN_INFO "[HI351-A690-3.4-v1] " fmt, ##args)
extern int iReadRegI2C(u8 *, u16, u8 *, u16, u16);
extern int iWriteRegI2C(u8 *, u16, u16);
static MSDK_SENSOR_CONFIG_STRUCT hi351_config;
static unsigned int hi351_i2c_errors;
static unsigned int hi351_pixel_clock = 260; /* original units: 100 kHz */
static bool hi351_capturing;
struct hi351_reg { u16 reg; u16 value; };
#include "hi351_official_init.h"
#include "hi351_official_modes.h"

static void HI351_write_cmos_sensor(unsigned int reg, unsigned int value)
{
    u8 data[2] = { reg, value };
    int rc = iWriteRegI2C(data, 2, HI351_I2C_ID);
    if (rc < 0 && hi351_i2c_errors++ < 8)
        HI351_LOG("I2C write %02x=%02x failed: %d\n", reg, value & 255, rc);
}
static u8 HI351_read_cmos_sensor(unsigned int reg)
{
    u8 addr = reg, value = 0;
    int rc = iReadRegI2C(&addr, 1, &value, 1, HI351_I2C_ID);
    if (rc < 0 && hi351_i2c_errors++ < 8)
        HI351_LOG("I2C read %02x failed: %d\n", reg, rc);
    return value;
}
static void hi351_apply(const struct hi351_reg *regs, unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; i++) {
        if (regs[i].reg == 0x100)
            mdelay(regs[i].value);
        else
            HI351_write_cmos_sensor(regs[i].reg, regs[i].value);
    }
}
#define HI351_APPLY(a) hi351_apply(a, ARRAY_SIZE(a))

static UINT32 HI351Open(void)
{
    unsigned int i, id = 0;
    BUILD_BUG_ON(sizeof(MSDK_SENSOR_CONFIG_STRUCT) != 56);
    BUILD_BUG_ON(offsetof(MSDK_SENSOR_INFO_STRUCT, CaptureDelayFrame) != 124);
    BUILD_BUG_ON(offsetof(MSDK_SENSOR_INFO_STRUCT, SensorGrabStartX) != 136);
    hi351_i2c_errors = 0;
    hi351_capturing = false;
    hi351_pixel_clock = 260;
    HI351_write_cmos_sensor(3, 0);
    for (i = 0; i < 2; i++) {
        HI351_write_cmos_sensor(1, 0xf1);
        HI351_write_cmos_sensor(1, 0xf3);
        HI351_write_cmos_sensor(1, 0xf1);
    }
    for (i = 0; i < 3; i++) {
        id = HI351_read_cmos_sensor(4);
        if (id == HI351_ID)
            break;
    }
    mdelay(20);
    HI351_LOG("open sensor ID=%02x, errors=%u\n", id, hi351_i2c_errors);
    if (id != HI351_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
    HI351_APPLY(hi351_official_init);
    HI351_write_cmos_sensor(3, 0);
    HI351_write_cmos_sensor(0x11, (HI351_read_cmos_sensor(0x11) & 0xfc) | 3);
    HI351_LOG("official init complete, entries=%u, errors=%u\n",
              (unsigned int)ARRAY_SIZE(hi351_official_init), hi351_i2c_errors);
    return hi351_i2c_errors ? ERROR_SENSOR_CONNECT_FAIL : ERROR_NONE;
}
static UINT32 HI351Close(void) { return ERROR_NONE; }

static UINT32 HI351GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *r)
{
    r->SensorPreviewWidth = 632; r->SensorPreviewHeight = 474;
    r->SensorFullWidth = 2016; r->SensorFullHeight = 1512;
    return ERROR_NONE;
}
static UINT32 HI351GetInfo(MSDK_SCENARIO_ID_ENUM scenario,
        MSDK_SENSOR_INFO_STRUCT *i, MSDK_SENSOR_CONFIG_STRUCT *c)
{
    i->SensorPreviewResolutionX = 632; i->SensorPreviewResolutionY = 474;
    i->SensorFullResolutionX = 2016; i->SensorFullResolutionY = 1512;
    i->SensorClockFreq = 26;
    i->SensorCameraPreviewFrameRate = 30; i->SensorVideoFrameRate = 30;
    i->SensorStillCaptureFrameRate = 10; i->SensorWebCamCaptureFrameRate = 15;
    i->SensorClockPolarity = 1; i->SensorClockFallingPolarity = 1;
    i->SensorClockRisingCount = 0; i->SensorClockFallingCount = 2;
    i->SensorClockDividCount = 3; i->SensorPixelClockCount = 3;
    i->SensorDataLatchCount = 2; i->SensorHsyncPolarity = 1;
    i->SensorVsyncPolarity = 1; i->SensorInterruptDelayLines = 1;
    i->SensorResetActiveHigh = 0; i->SensorResetDelayCount = 1;
    i->SensroInterfaceType = SENSOR_INTERFACE_TYPE_PARALLEL;
    i->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_YUYV;
    i->CaptureDelayFrame = 2; i->PreviewDelayFrame = 3; i->VideoDelayFrame = 20;
    i->SensorDrivingCurrent = 3;
    i->SensorMasterClockSwitch = 0;
    if (scenario == MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG ||
        scenario == MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM) {
        i->SensorGrabStartX = 16; i->SensorGrabStartY = 12;
    } else {
        i->SensorGrabStartX = 4; i->SensorGrabStartY = 3;
    }
    memcpy(c, &hi351_config, sizeof(*c));
    return ERROR_NONE;
}
static UINT32 HI351Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *w,
        MSDK_SENSOR_CONFIG_STRUCT *c)
{
    hi351_capturing = false;
    hi351_pixel_clock = 390;
    HI351_APPLY(hi351_preview_regs);
    w->GrabStartX = 0; w->GrabStartY = 0;
    w->ExposureWindowWidth = 624; w->ExposureWindowHeight = 468;
    msleep(10);
    memcpy(&hi351_config, c, sizeof(*c));
    HI351_LOG("preview, errors=%u\n", hi351_i2c_errors);
    return ERROR_NONE;
}
static UINT32 HI351Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *w,
        MSDK_SENSOR_CONFIG_STRUCT *c)
{
    u32 shutter;
    hi351_capturing = true;
    if (w->ImageTargetWidth > 640 || w->ImageTargetHeight > 480) {
        HI351_write_cmos_sensor(3, 0x20);
        shutter = (u32)HI351_read_cmos_sensor(0xa4) << 24;
        shutter |= (u32)HI351_read_cmos_sensor(0xa5) << 16;
        shutter |= (u32)HI351_read_cmos_sensor(0xa6) << 8;
        shutter |= HI351_read_cmos_sensor(0xa7);
        HI351_APPLY(hi351_capture_regs);
        if (!shutter) shutter = 1;
        HI351_write_cmos_sensor(3, 0x20);
        HI351_write_cmos_sensor(0x20, shutter >> 24);
        HI351_write_cmos_sensor(0x21, shutter >> 16);
        HI351_write_cmos_sensor(0x22, shutter >> 8);
        HI351_write_cmos_sensor(0x23, shutter);
        HI351_write_cmos_sensor(3, 0);
        HI351_write_cmos_sensor(1, 0xf0);
        HI351_write_cmos_sensor(3, 0xc4);
        HI351_write_cmos_sensor(0x10, 0xeb);
        HI351_write_cmos_sensor(3, 0xc5);
        HI351_write_cmos_sensor(0x10, 0xb1);
        HI351_write_cmos_sensor(3, 0xcf);
        HI351_write_cmos_sensor(0x10, 0xaf);
        HI351_write_cmos_sensor(3, 0xc0);
        HI351_write_cmos_sensor(0x33, 0);
        HI351_write_cmos_sensor(0x32, 1);
        w->ExposureWindowWidth = 2032; w->ExposureWindowHeight = 1524;
        HI351_LOG("capture shutter=%u, errors=%u\n", shutter, hi351_i2c_errors);
    } else {
        w->ExposureWindowWidth = 624; w->ExposureWindowHeight = 468;
    }
    w->GrabStartX = 1; w->GrabStartY = 1;
    mdelay(10);
    memcpy(&hi351_config, c, sizeof(*c));
    return ERROR_NONE;
}
static void HI351_night_mode(int enable)
{
    if (hi351_capturing) return;
    if (enable) HI351_APPLY(hi351_night_on_regs);
    else HI351_APPLY(hi351_night_off_regs);
    HI351_APPLY(hi351_night_tail_regs);
    msleep(10);
}
static int HI351_set_param_wb(int a1)
{
  int result; // r0
  char v2; // r1
  char v3; // r1

  switch ( a1 )
  {
    case 1:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 164);
      HI351_write_cmos_sensor(18, 151);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 235);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 85);
      HI351_write_cmos_sensor(3, 198);
      v2 = 70;
      goto LABEL_7;
    case 2:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 160);
      HI351_write_cmos_sensor(18, 7);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 240);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 200);
      HI351_write_cmos_sensor(3, 198);
      HI351_write_cmos_sensor(24, 117);
      HI351_write_cmos_sensor(25, 118);
      HI351_write_cmos_sensor(26, 106);
      v3 = 107;
      goto LABEL_9;
    case 3:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 160);
      HI351_write_cmos_sensor(18, 7);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 255);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 240);
      HI351_write_cmos_sensor(3, 198);
      HI351_write_cmos_sensor(24, 144);
      HI351_write_cmos_sensor(25, 146);
      HI351_write_cmos_sensor(26, 95);
      v3 = 97;
      goto LABEL_9;
    case 6:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 160);
      HI351_write_cmos_sensor(18, 7);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 180);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 130);
      HI351_write_cmos_sensor(3, 198);
      HI351_write_cmos_sensor(24, 93);
      HI351_write_cmos_sensor(25, 94);
      HI351_write_cmos_sensor(26, 148);
      HI351_write_cmos_sensor(27, 149);
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(16, 177);
      return 1;
    case 8:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 160);
      HI351_write_cmos_sensor(18, 7);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 130);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 80);
      HI351_write_cmos_sensor(3, 198);
      v2 = 68;
LABEL_7:
      HI351_write_cmos_sensor(24, v2);
      HI351_write_cmos_sensor(25, 144);
      HI351_write_cmos_sensor(26, 64);
      v3 = -92;
      goto LABEL_9;
    case 9:
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(17, 160);
      HI351_write_cmos_sensor(18, 7);
      HI351_write_cmos_sensor(38, 0);
      HI351_write_cmos_sensor(39, 130);
      HI351_write_cmos_sensor(40, 0);
      HI351_write_cmos_sensor(41, 80);
      HI351_write_cmos_sensor(3, 198);
      HI351_write_cmos_sensor(24, 71);
      HI351_write_cmos_sensor(25, 72);
      HI351_write_cmos_sensor(26, 164);
      v3 = -91;
LABEL_9:
      HI351_write_cmos_sensor(27, v3);
      HI351_write_cmos_sensor(3, 197);
      HI351_write_cmos_sensor(16, 177);
      result = 1;
      break;
    default:
      result = 0;
      break;
  }
  return result;
}

static int HI351_set_param_effect(int a1)
{
  int result; // r0
  char v2; // r1
  char v3; // r0
  char v4; // r1
  char v5; // r1

  switch ( a1 )
  {
    case 0:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      v2 = -16;
      goto LABEL_6;
    case 1:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      HI351_write_cmos_sensor(18, 243);
      HI351_write_cmos_sensor(66, 0);
      HI351_write_cmos_sensor(67, 0);
      HI351_write_cmos_sensor(68, 128);
      HI351_write_cmos_sensor(69, 128);
      return 1;
    case 2:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      HI351_write_cmos_sensor(18, 243);
      HI351_write_cmos_sensor(66, 0);
      HI351_write_cmos_sensor(67, 0);
      HI351_write_cmos_sensor(68, 96);
      v3 = 69;
      v4 = -93;
      goto LABEL_10;
    case 3:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      v2 = -8;
LABEL_6:
      HI351_write_cmos_sensor(18, v2);
      HI351_write_cmos_sensor(66, 0);
      HI351_write_cmos_sensor(67, 0);
      HI351_write_cmos_sensor(68, 128);
      v3 = 69;
      v4 = 0x80;
      goto LABEL_10;
    case 9:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      HI351_write_cmos_sensor(18, 243);
      HI351_write_cmos_sensor(66, 0);
      HI351_write_cmos_sensor(67, 0);
      HI351_write_cmos_sensor(68, 88);
      v5 = 64;
      goto LABEL_9;
    case 10:
      HI351_write_cmos_sensor(3, 16);
      HI351_write_cmos_sensor(17, 3);
      HI351_write_cmos_sensor(18, 243);
      HI351_write_cmos_sensor(66, 0);
      HI351_write_cmos_sensor(67, 0);
      HI351_write_cmos_sensor(68, 192);
      v5 = 0x80;
LABEL_9:
      HI351_write_cmos_sensor(69, v5);
      HI351_write_cmos_sensor(3, 20);
      v3 = 0x80;
      v4 = 32;
LABEL_10:
      HI351_write_cmos_sensor(v3, v4);
      result = 1;
      break;
    default:
      result = 0;
      break;
  }
  return result;
}

static int HI351_set_param_exposure(int a1)
{
  char cmos_sensor; // r0
  char v3; // r1

  HI351_write_cmos_sensor(3, 16);
  cmos_sensor = HI351_read_cmos_sensor(18);
  HI351_write_cmos_sensor(18, cmos_sensor | 0x10);
  switch ( a1 )
  {
    case 0:
      HI351_write_cmos_sensor(3, 16);
      v3 = 0x80;
      goto LABEL_9;
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
    case 13:
    case 15:
      return 1;
    case 4:
      HI351_write_cmos_sensor(3, 16);
      v3 = 16;
      goto LABEL_9;
    case 8:
      HI351_write_cmos_sensor(3, 16);
      v3 = 32;
      goto LABEL_9;
    case 12:
      HI351_write_cmos_sensor(3, 16);
      v3 = -112;
      goto LABEL_9;
    case 16:
      HI351_write_cmos_sensor(3, 16);
      v3 = -96;
      goto LABEL_9;
    default:
      HI351_write_cmos_sensor(3, 16);
      v3 = 0x80;
LABEL_9:
      HI351_write_cmos_sensor(64, v3);
      return 1;
  }
}


/* Android 4.0 adds feature/scene/negative-EV enum entries. */
static void hi351_setting(unsigned int feature, unsigned int value)
{
    switch (feature) {
    case FID_SCENE_MODE:
        if (value == SCENE_MODE_OFF || value == SCENE_MODE_NIGHTSCENE)
            HI351_night_mode(value == SCENE_MODE_NIGHTSCENE);
        break;
    case FID_COLOR_EFFECT: HI351_set_param_effect(value); break;
    case FID_AE_FLICKER:
        if (value == AE_FLICKER_MODE_60HZ || value == AE_FLICKER_MODE_50HZ) {
            HI351_write_cmos_sensor(3, 0x20);
            HI351_write_cmos_sensor(0x10, value == AE_FLICKER_MODE_50HZ ? 0x97 : 0x87);
        }
        break;
    case FID_AE_EV:
        switch (value) {
        case AE_EV_COMP_00: HI351_set_param_exposure(0); break;
        case AE_EV_COMP_10: HI351_set_param_exposure(4); break;
        case AE_EV_COMP_20: HI351_set_param_exposure(8); break;
        case AE_EV_COMP_n10: HI351_set_param_exposure(12); break;
        case AE_EV_COMP_n20: HI351_set_param_exposure(16); break;
        default: break;
        }
        break;
    case FID_AWB_MODE: HI351_set_param_wb(value); break;
    default: break;
    }
}
static UINT32 HI351Control(MSDK_SCENARIO_ID_ENUM scenario,
        MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *w, MSDK_SENSOR_CONFIG_STRUCT *c)
{
    switch (scenario) {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        HI351Preview(w,c); return 1;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        HI351Capture(w,c); return 1;
    default: return ERROR_INVALID_SCENARIO_ID;
    }
}
static UINT32 HI351FeatureControl(MSDK_SENSOR_FEATURE_ENUM feature,
        UINT8 *data, UINT32 *length)
{
    UINT16 *p16 = (UINT16 *)data;
    UINT32 *p32 = (UINT32 *)data;
    MSDK_SENSOR_REG_INFO_STRUCT *reg = (MSDK_SENSOR_REG_INFO_STRUCT *)data;
    switch (feature) {
    case SENSOR_FEATURE_CHECK_SENSOR_ID: {
        unsigned int retry, id = 0;
        HI351_write_cmos_sensor(3, 0);
        HI351_write_cmos_sensor(1, 0xf1);
        HI351_write_cmos_sensor(1, 0xf3);
        HI351_write_cmos_sensor(1, 0xf1);
        mdelay(20);
        for (retry = 0; retry < 3; retry++) {
            id = HI351_read_cmos_sensor(4);
            if (id == HI351_ID) break;
        }
        p32[0] = id == HI351_ID ? HI351_ID : 0xffffffff;
        *length = 4;
        HI351_LOG("probe ID=%02x\n", id);
        return id == HI351_ID ? ERROR_NONE : ERROR_SENSOR_CONNECT_FAIL;
    }
    case SENSOR_FEATURE_GET_RESOLUTION:
        p16[0] = 2016; p16[1] = 1512; *length = 4; break;
    case SENSOR_FEATURE_GET_PERIOD:
        p16[0] = 1640; p16[1] = 620; *length = 4; break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        /* ICS ImgSensorDrv expects Hz; the original GB ABI used MHz. */
        p32[0] = hi351_pixel_clock * 100000; *length = 4; break;
    case SENSOR_FEATURE_SET_NIGHTMODE: HI351_night_mode(data[0]); break;
    case SENSOR_FEATURE_SET_REGISTER:
        HI351_write_cmos_sensor(reg->RegAddr, reg->RegData); break;
    case SENSOR_FEATURE_GET_REGISTER:
        reg->RegData = HI351_read_cmos_sensor(reg->RegAddr); break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(data, &hi351_config, sizeof(hi351_config));
        *length = sizeof(hi351_config); break;
    case SENSOR_FEATURE_GET_GROUP_COUNT: p32[0] = 0; *length = 4; break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID: p32[0] = 0xffffffff; *length = 4; break;
    case SENSOR_FEATURE_SET_YUV_CMD: hi351_setting(p32[0],p32[1]); break;
    default: break;
    }
    return ERROR_NONE;
}
UINT32 HI351_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *func)
{
    static SENSOR_FUNCTION_STRUCT ops = {
        HI351Open, HI351GetInfo, HI351GetResolution,
        HI351FeatureControl, HI351Control, HI351Close
    };
    if (func) *func = &ops;
    return ERROR_NONE;
}
