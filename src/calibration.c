//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------

#include "calibration.h"
#include "F700_FlashUtils.h"
#include "main.h"


U8 code CalInfo1[SENSOR_CALIBRATION_DATA_LENGTH+1] _at_ FLASH_ADDR_CAL1;
U8 code CalFlashCheck[3] _at_ FLASH_ADDR_CAL_CHECK;  //Just a check that the flash has been programmed

void SaveCalibrationToFlash(U16 add_off, U8 packet_length)
{
   
   U8 flashCheckWrite[3] = {0xFF,0xFE,0xFD};

   // Erase our configuration Flash page
   // And copy our Calibration data into it

   add_off = add_off << 4;

   if(add_off == 0)
   {
      // Erase our configuration Flash page
      FLASH_PageErase( (FLADDR)(&CalInfo1[0]) );   
      // And copy our updated data into it
      FLASH_Write( (FLADDR)(&CalFlashCheck[0]),
         (U8 *)(&flashCheckWrite[0]), 
         3);     
   }

  
   FLASH_Write( (FLADDR)(&CalInfo1[0]) + add_off, 
         (U8 xdata *)(&MainRegister[SET_SENSOR6]), 
         packet_length);

   
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
bit CheckCalFlash(void)
{
  U8 index;

  // First copy our existing calibration table into scratch space
  index = 0;

  if(CalFlashCheck[0] == 0xFF && CalFlashCheck[1] == 0xFE && CalFlashCheck[2] == 0xFD)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}