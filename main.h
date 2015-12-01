//-----------------------------------------------------------------------------
//                                                                            
//  Copyright (c) 2015 All Right Reserved                                      
//  Pressure Profile Systems                                                   
//  www.pressureprofile.com                                                    
//  V1.0                                                                       
//                                                                             
//-----------------------------------------------------------------------------

#ifndef _MAIN_H_
#define _MAIN_H_
//-----------------------------------------------------------------------------
#include "compiler_defs.h"
#include "C8051F700_defs.h"

//#include "Calibration.h"
//-----------------------------------------------------------------------------
// Commands
#define CMD_READ			      0x01
#define CMD_WRITE					0x02
#define CMD_WRITE_CAL	      0x04


//Memory Layout
#define SETTINGS_LOCATION 0
#define PARAMSLOCATION 112
#define SENSOR_DATA_LOCATION 128
#define MAIN_REGISTER_LENGTH 192
#define SENSOR_CALIBRATION_DATA_LENGTH 512


extern volatile U8 xdata MainRegister[MAIN_REGISTER_LENGTH];

extern U8 ChannelIndex;

extern bit IsScanning;
extern UU16 FrameCounter;
extern U8 SmbAddress;

#define TRUE 1
#define FALSE 0

//SMB States
#define SMB_IDLE               0
#define SMB_RECEIVING          1
#define SMB_RECEIVED           2
#define SMB_SENDING            3
#define SMB_SENT               4
#define SMB_ERROR              5


#define SMB_BUF_SIZE_IN       32
#define SLAVE_ADDR_DEF        4
#define SERIAL_NUM_DEF        0x3333

#define FLASH_CHECK_ADDRESS      0x3000
#define FLASH_ADDR_SETUP         0x3003
#define FLASH_ADDR_CAL_CHECK     0x2000
#define FLASH_ADDR_CAL_SLOPE     0x2010
#define FLASH_ADDR_CAL_OFFSET     0x2020
#define FLASH_ADDR_CAL1           0x2200
#define FLASH_ADDR_CAL2           0x2400
#define FLASH_ADDR_CAL3           0x2600
#define FLASH_ADDR_CAL4           0x2800



// SMBus status vector - top 4 bits only
#define  SMB_SRADD            0x20           // (SR) slave address received
//    (also could be a lost
//    arbitration)
#define  SMB_SRSTO            0x10           // (SR) STOP detected while SR or ST,
//    or lost arbitration
#define  SMB_SRDB             0x00           // (SR) data byte received, or
//    lost arbitration
#define  SMB_STDB             0x40           // (ST) data byte transmitted
#define  SMB_STSTO            0x50           // (ST) STOP detected during a
//    transaction; bus error


//Memory Layout
enum
{
   SET_SMBUSADDRESS,
   SET_SERIALNUMBERMSB,
   SET_SERIALNUMBERLSB,
   SET_PWMPINMODE,
   SET_SENSORMAPPINGPWM_ANALOGUE,
   SET_ACCUMULATOR,
   SET_REFERENCEGAIN,
   SET_CONVERSIONSIZE,
   SET_DISCHARGETIME,
   SET_OUTPUTCURRENT,
   SET_SCALINGANALOGUEOUTMSB,
   SET_SCALINGANALOGUEOUTLSB,
   SET_NUMBERELEMENTS,
   SET_RESERVED,
   SET_SETTINGSPARAMCOUNT,
};

enum 
{
   SET_SCANLIST0 = SET_SETTINGSPARAMCOUNT+1,
   SET_SCANLIST1,
   SET_SCANLIST2,
   SET_SCANLIST3,
   SET_SCANLIST4,
   SET_SCANLIST5,
   SET_SCANLIST6,
   SET_SCANLIST7,
   SET_SCANLIST8,
   SET_SCANLIST9,
   SET_SCANLIST10,
   SET_SCANLIST11,
   SET_SCANLIST12,
   SET_SCANLIST13,
   SET_SCANLIST14,
   SET_SCANLIST15,
   SET_SCANLIST16,
   SET_SCANLIST17,
   SET_SCANLIST18,
   SET_SCANLIST19,
   SET_SCANLIST20,
   SET_SCANLIST21,
   SET_SCANLIST22,
   SET_SCANLIST23,
   SET_SCANLIST24,
   SET_LENGTHSCANLIST,
};

enum 
{
   SET_BASELINE0  = SET_LENGTHSCANLIST+1,
   SET_BASELINE0b,
   SET_BASELINE1,
   SET_BASELINE1a,
   SET_BASELINE2,
   SET_BASELINE2b,
   SET_BASELINE3,
   SET_BASELINE3b,
   SET_BASELINE4,
   SET_BASELINE4b,
   SET_BASELINE5,
   SET_BASELINE5b,
   SET_BASELINE6,
   SET_BASELINE6b,
   SET_BASELINE7,
   SET_BASELINE7b,
   SET_BASELINE8,
   SET_BASELINE8b,
   SET_BASELINE9,
   SET_BASELINE9b,
   SET_BASELINE10,
   SET_BASELINE10b,
   SET_BASELINE11,
   SET_BASELINE11b,
   SET_BASELINE12,
   SET_BASELINE12b,
   SET_BASELINE13,
   SET_BASELINE13b,
   SET_BASELINE14,
   SET_BASELINE14b,
   SET_BASELINE15,
   SET_BASELINE15b,
   SET_BASELINE16,
   SET_BASELINE16b,
   SET_BASELINE17,
   SET_BASELINE17b,
   SET_BASELINE18,
   SET_BASELINE18b,
   SET_BASELINE19,
   SET_BASELINE19b,
   SET_BASELINE20,
   SET_BASELINE20b,
   SET_BASELINE21,
   SET_BASELINE21b,
   SET_BASELINE22,
   SET_BASELINE22b,
   SET_BASELINE23,
   SET_BASELINE23b,
   SET_BASELINE24,
   SET_BASELINE24b,
   N_SETTINGS
};


enum
{
   PARAM_IS_SCANNING = PARAMSLOCATION,
   PARAM_ERROR,
   PARAM_NUMPARAMS
};

enum
{
   SET_ITR_MSB= 0,
   SET_ITR_LSB,
   SET_TIMESTAMP_MSB,
   SET_TIMESTAMP_LSB,
   SET_SENSOR0,
   SET_SENSOR0b,
   SET_SENSOR1,
   SET_SENSOR1b,
   SET_SENSOR2,
   SET_SENSOR3b,
   SET_SENSOR4,
   SET_SENSOR4b,
   SET_SENSOR5,
   SET_SENSOR5b,
   SET_SENSOR6,
   SET_SENSOR6b,
   SET_SENSOR7,
   SET_SENSOR7b,
   SET_SENSOR8,
   SET_SENSOR8b,
   SET_SENSOR9,
   SET_SENSOR9b,
   SET_SENSOR10,
   SET_SENSOR10b,
   SET_SENSOR11,
   SET_SENSOR11b,
   SET_SENSOR12,
   SET_SENSOR12b,
   SET_SENSOR13,
   SET_SENSOR13b,
   SET_SENSOR14,
   SET_SENSOR14b,
   SET_SENSOR15,
   SET_SENSOR15b,
   SET_SENSOR16,
   SET_SENSOR16b,
   SET_SENSOR17,
   SET_SENSOR17b,
   SET_SENSOR18,
   SET_SENSOR18b,
   SET_SENSOR19,
   SET_SENSOR19b,
   SET_SENSOR20,
   SET_SENSOR20b,
   SET_SENSOR21,
   SET_SENSOR21b,
   SET_SENSOR22,
   SET_SENSOR22b,
   SET_SENSOR23,
   SET_SENSOR23b,
   SET_SENSOR24,
   SET_SENSOR24b,
   SET_MAX_NUM_SENSORS
};

enum
{
   ERROR_TRUNCATEDPACKET,
   ERROR_BUFFEROVERWRITE,
   ERROR_REQUESTEDTOOMUCHDATA,
   ERROR_ANALOGPWMOUTPUTINDEXINVALID
};


//-----------------------------------------------------------------------------
#endif // _MAIN_H_
