#include "compiler_defs.h"
#include "C8051F700_defs.h"
#include "main.h"
#include "F700_FlashUtils.h"

//Flash Memory
//The settings
U8 code FlashCheckSettings[3] _at_ FLASH_CHECK_ADDRESS_SETTINGS;  //Just a check that the flash has been programmed
U8 code SetupInfo[nSettings+1] _at_ FLASH_ADDR_SETUP;  //Our settings in flash

//The main descriptor table
U8 code FlashCheckDesc[3] _at_ FLASH_CHECK_ADDRESS_DESC;  //Just a check that the flash has been programmed
U8 code RecordingDescriptors[129] _at_ FLASH_DESCRIPTOR_TABLE; //Our descriptor table

bool_t loadFlashSettings(void)
{
   U8 i, index;

   index = 0;
      

	//Check that our flash header is ok
	if(FlashCheckSettings[0] == 0xFF && FlashCheckSettings[1] == 0xFE && FlashCheckSettings[2] == 0xFD)
	{
	   // And now all of our other setup stuff
	   for (i = 0; i < nSettings; ++i)
	   {
	      MainRegister[i] = SetupInfo[i];
	   }

	   return TRUE;
   }
   else
   {
		return FALSE;
   }
}


bool_t loadFlashDescTable(void)
{
   U8 i, index;

   index = 0;
      

	//Check that our flash header is ok
	if(FlashCheckDesc[0] == 0xFF && FlashCheckDesc[1] == 0xFE && FlashCheckDesc[2] == 0xFD)
	{
	   // And now all of our other setup stuff
	   for (i = 0; i < 128; ++i)
	   {
	      MainRegister[i+SENSOR_MICRON_BUFFER_LOCATION] = RecordingDescriptors[i];
	   }

	   return TRUE;
   }
   else
   {
		return FALSE;
   }
}



void saveSettingsToFlash(void)
{

	U8 FlashCheckWrite[3] = {0xFF,0xFE,0xFD};

   // Erase our configuration Flash page
   FLASH_PageErase( (FLADDR)(&FlashCheckSettings[0]) );


   // And copy our updated data into it
   FLASH_Write( (FLADDR)(&FlashCheckSettings[0]), 
                (U8 *)(&FlashCheckWrite[0]), 
                3);

   // And copy our updated data into it
   FLASH_Write( (FLADDR)(&SetupInfo[0]), 
                (U8 xdata *)(&MainRegister[0]), 
                nSettings);

}


void saveDescriptorsToFlash(void)
{

	U8 FlashCheckWrite[3] = {0xFF,0xFE,0xFD};

   // Erase our configuration Flash page
   FLASH_PageErase( (FLADDR)(&FlashCheckDesc[0]) );


   // And copy our updated data into it
   FLASH_Write( (FLADDR)(&FlashCheckDesc[0]), 
                (U8 *)(&FlashCheckWrite[0]), 
                3);

   // And copy our updated data into it
   FLASH_Write( (FLADDR)(&RecordingDescriptors[0]), 
                (U8 xdata *)(&MainRegister[SENSOR_MICRON_BUFFER_LOCATION]), 
                128);

}


void EraseFlashDescriptors(void)
{
   FLASH_PageErase( (FLADDR)(&FlashCheckDesc[0]) );
}


