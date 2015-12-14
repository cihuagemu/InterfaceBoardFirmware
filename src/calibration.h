//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "compiler_defs.h"
#include "C8051F700_defs.h"

#define CALIBRATION

#define TRUE 1
#define FALSE 0

#define SENSOR_CALIBRATION_DATA_LENGTH 512


#define FLASH_ADDR_CAL_CHECK      0x2000
#define FLASH_ADDR_CAL_SLOPE      0x2010
#define FLASH_ADDR_CAL_OFFSET     0x2020
#define FLASH_ADDR_CAL1           0x2200
#define FLASH_ADDR_CAL2           0x2400
#define FLASH_ADDR_CAL3           0x2600
#define FLASH_ADDR_CAL4           0x2800


extern void Cal_Init();
extern void SaveCalibrationToFlash(U16 add_off, U8 xdata * writeAddress, U8 packet_length);
extern bit CheckCalFlash();
extern void CalCalibrationTable(void);

extern U8* code CalInfo1;
extern volatile U8 xdata CalibrationData[8];


#endif /* CALIBRATION_H */

