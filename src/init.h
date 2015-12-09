//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------

#ifndef _INIT_H_
#define _INIT_H_
//-----------------------------------------------------------------------------
#include "compiler_defs.h"
#include "C8051F700_defs.h"
//-----------------------------------------------------------------------------
#define  SYSCLK         24500000
#define  SMB_FREQUENCY  100000
//-----------------------------------------------------------------------------
extern void SMBus_Init(void);
extern void Oscillator_Init(void);
extern void CapSense_Init(void);
extern void VddMon_Init(void);
extern void Port_Init(void);
extern void Timers_Init(void);
extern void SPI_Init(void);

#endif /* _INIT_H_ */
