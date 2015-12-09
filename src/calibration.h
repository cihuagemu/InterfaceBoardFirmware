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

extern U16 code calibrationValues[1024];
extern U16 code calibrationScaling;
extern U16 code calibrationBaseline;

#endif /* CALIBRATION_H */
