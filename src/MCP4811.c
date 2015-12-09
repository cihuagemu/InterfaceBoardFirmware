//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------

#include "main.h"

#include "MCP4811.h"

const U8 ENTER_4BYTE_ADDRESS = 0xB7;

const U8 WRITE_ENABLED = 0x06;
const U8 WRITE_DISABLED = 0x04;

const U8 PAGE_PROGRAM = 0x02;
const U8 READ = 0x03;
const U8 READ_ID = 0x9E;
const U8 FOUR_BYTE_MODE = 0xB7;
const U8 READ_FLAG_STATUS_REG = 0x70;
const U8 READ_STATUS_REG = 0x05;
const U8 CLEAR_FLAG_STATUS_REG = 0x50;
const U8 DIE_ERASE = 0xC4;
const U8 SECTOR_ERASE = 0xD8;
const U8 EXTENDED_ADDRESS_CHANGE = 0xC5;

volatile UU32 CurrentAddress;

//Chip select for analogue output
sbit ANALOG_OUT_CS = P2^1;

//Functions

U8 sendrcvSPI(unsigned char value);

void DeselectChip()
{
  //time_t timeOut = 0;

  //while (++timeOut < IO_WAIT_ITERATIONS/1000); //A short delay

  //timeOut = 0;

  //P2 = P2 | 0x02;

  ANALOG_OUT_CS = 1;

  //while (++timeOut < IO_WAIT_ITERATIONS/1000); //A short delay
}

void SelectChip()
{
  //time_t timeOut = 0;
  //P2 = P2 & 0x0D;
  ANALOG_OUT_CS = 0;

  //while (++timeOut < IO_WAIT_ITERATIONS/1000); //A short delay
}


U8 Micron_CheckID()
{
  U8 i, foo;
  U8 ID[3];

  SelectChip();

  sendrcvSPI(READ_ID);
  for( i = 0; i < 3; i++)
  {
    ID[i] = sendrcvSPI(READ_ID);
    foo ++;
  }

  DeselectChip();

  if(ID[0] == 0x20 && ID[1] == 0xba && ID[2] == 0x20) //From datasheet page 40
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

U8 Micron_Init()
{
  U8 flag;

  SelectChip();
  sendrcvSPI(CLEAR_FLAG_STATUS_REG);  ///Clear flag status reg
  flag = sendrcvSPI(0);
  DeselectChip();

  CurrentAddress.U32 = 0;

  SelectChip();
  sendrcvSPI(WRITE_ENABLED);  //Enable writing
  DeselectChip();

  SelectChip(); //Reset extended address
  sendrcvSPI(EXTENDED_ADDRESS_CHANGE);
  sendrcvSPI(CurrentAddress.U8[3]);
  DeselectChip();

  return TRUE;
}

void Micron_SetAddress(U32 Address)
{
  U8 temp = CurrentAddress.U8[3];
  //Check if we have changed the extended address byte, if so update
  if(CurrentAddress.U8[0] != ((Address >>24) & 0xFF))
  {
    SelectChip();
    sendrcvSPI(WRITE_ENABLED);  //Enable writing
    DeselectChip();

    SelectChip();
    sendrcvSPI(EXTENDED_ADDRESS_CHANGE);
    sendrcvSPI(CurrentAddress.U8[0]);
    DeselectChip();
  }

  CurrentAddress.U32 = Address;
}

//Send a single byte over I2C
U8 sendrcvSPI(unsigned char value)
{
  register unsigned char inBuf = 0x33;
  time_t timeOut = 0;

  // Send the data to the SPI output buffer
  SPI0DAT = value;

  // Wait for the send to finish
  while (!SPIF);/*
  {
    if (++timeOut > IO_WAIT_ITERATIONS)
    {
      return 0;
    }
  }*/

  // Flush the buffer since we always read when we write
  inBuf = SPI0DAT;

  // Reset SPI finished flag
  SPIF = 0;

  return inBuf;
}

U8 Micron_ReadFlashBuffer()
{
  U8 i;

  SelectChip();
  sendrcvSPI(READ);  //Read

  sendrcvSPI(CurrentAddress.U8[1]); //Address
  sendrcvSPI(CurrentAddress.U8[2]);
  sendrcvSPI(CurrentAddress.U8[3]);

  for(i = 0; i < SENSOR_MICRON_BUFFER_LENGTH; i++)
  {
    MainRegister[SENSOR_MICRON_BUFFER_LOCATION +i] = sendrcvSPI(0);
  }

  DeselectChip();

  return TRUE;
}

U8 Micron_WriteFlashBuffer()
{
  U8 i, flag, status;

  /*
  for(i = 0; i < SENSOR_MICRON_BUFFER_LENGTH; i++)
  {
    MainRegister[SENSOR_MICRON_BUFFER_LOCATION +i] = CurrentAddress.U8[3]; //TODO - Debug code
  }
  */

  SelectChip();
  sendrcvSPI(READ_STATUS_REG);  ///Read flag status reg
  status = sendrcvSPI(0);
  DeselectChip();

  SelectChip();
  sendrcvSPI(WRITE_ENABLED);  //Enable writing
  DeselectChip();

  SelectChip();
  sendrcvSPI(PAGE_PROGRAM);  //Write
  sendrcvSPI(CurrentAddress.U8[1]); //Address
  sendrcvSPI(CurrentAddress.U8[2]);
  sendrcvSPI(CurrentAddress.U8[3]);

  //toSend = MicronFlashBuffer[i];
  for(i = 0; i < SENSOR_MICRON_BUFFER_LENGTH; i++)
  {
    sendrcvSPI(MainRegister[SENSOR_MICRON_BUFFER_LOCATION +i]);
  }

  DeselectChip();

  SelectChip();
  sendrcvSPI(READ_FLAG_STATUS_REG);  ///Read flag status reg
  flag = sendrcvSPI(0);
  DeselectChip();

  flag = 0;

  while((flag & 0x80) == 0)  // Wait for erase to complete
  {
    SelectChip();
    sendrcvSPI(READ_FLAG_STATUS_REG);  ///Read flag status reg
    flag = sendrcvSPI(0);
    DeselectChip();
  }

  SelectChip();
  sendrcvSPI(CLEAR_FLAG_STATUS_REG);  ///Clear flag status reg
  flag = sendrcvSPI(0);
  DeselectChip();

  return TRUE;
}

//Just erasing the first Sector for now - it takes 4 - 8 minutes to erase the full device!!!
U8 Micron_EraseDevice()
{
  volatile U8 flag;
  U8 status;

  SelectChip();
  sendrcvSPI(READ_STATUS_REG);  ///Read flag status reg
  status = sendrcvSPI(0);
  DeselectChip();

  SelectChip();
  sendrcvSPI(WRITE_ENABLED);  //Enable writing
  DeselectChip();

  SelectChip();

  sendrcvSPI(SECTOR_ERASE);  //Write

  sendrcvSPI(CurrentAddress.U8[1]); //Address
  sendrcvSPI(CurrentAddress.U8[2]);
  sendrcvSPI(CurrentAddress.U8[3]);

  DeselectChip();

  SelectChip();
  sendrcvSPI(READ_FLAG_STATUS_REG);  ///Read flag status reg
  flag = sendrcvSPI(0);
  DeselectChip();

  flag = 0;

  while((flag & 0x80) == 0)  // Wait for erase to complete
  {
    SelectChip();
    sendrcvSPI(READ_FLAG_STATUS_REG);  ///Read flag status reg
    flag = sendrcvSPI(0);
    DeselectChip();
  }

  SelectChip();
  sendrcvSPI(CLEAR_FLAG_STATUS_REG);  ///Clear flag status reg
  flag = sendrcvSPI(0);
  DeselectChip();

  return TRUE;
}
