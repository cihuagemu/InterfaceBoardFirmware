//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#include "main.h"
//-----------------------------------------------------------------------------
U8 loadEeprom(void)
{
  U8 i, loadDebug[eeParamCount + 2];
  U8 retVal;
  UU16 crc1, crc2;
  SFRPAGE = CONFIG_PAGE;

  EECNTL = 0x84;

  retVal = (0x03 != (EEKEY & 0x03));
  SFRPAGE = LEGACY_PAGE;

  for (i = 0; i < eeParamCount + 2; ++i)
  {
    loadDebug[i] = readEeprom(i);
  }

  crc1.U16 = getEepromChecksum();
  crc2.U8[MSB] = readEeprom(eeParamCount);
  crc2.U8[LSB] = readEeprom(eeParamCount + 1);

  return ( retVal && (crc1.U16 == crc2.U16) );
}
//-----------------------------------------------------------------------------
U8 saveEeprom(void)
{
  U8 i, saveDebug[eeParamCount + 2];
  U8 retVal;
  UU16 crc;

  writeEeprom(eeSmbAddress, SmbAddress);
  writeEeprom(eeSerialMsb, SerialNumber.U8[MSB]);
  writeEeprom(eeSerialLsb, SerialNumber.U8[LSB]);
  writeEeprom(eeCs0Config, CS0CF);
  writeEeprom(eeCs0Mode1, CS0MD1);
  SFRPAGE = CONFIG_PAGE;
  writeEeprom(eeCs0Mode2, CS0MD2);
  SFRPAGE = LEGACY_PAGE;
  writeEeprom(eeRefVal1, RefVal1);
  writeEeprom(eeRefVal2, RefVal2);
  writeEeprom(eeRefVal3, RefVal3);
  writeEeprom(eeRefVal4, RefVal4);

  // Calculate and store our Checksum
  crc.U16 = getEepromChecksum();

  writeEeprom(eeParamCount, crc.U8[MSB]);
  writeEeprom(eeParamCount + 1, crc.U8[LSB]);

  for (i = 0; i < eeParamCount + 2; ++i)
  {
    saveDebug[i] = readEeprom(i);
  }

  SFRPAGE = CONFIG_PAGE;
  switch (EEKEY & 0x03)
  {
    case 0x00:
      EEKEY = 0x55;
      // Fall-through on purpose
      // to unlock for writing

    case 0x01:
      EEKEY = 0xAA;
      // Fall-through on purpose
      // to unlock for writing

    case 0x02:
      // Store EEPROM values in FLASH
      EECNTL = 0x82;
      break;

    case 0x03:
    default:
      // Write-erase are disabled, so we
      // can't do anything
      break;
  }
  retVal = (0x03 != (EEKEY & 0x03));
  SFRPAGE = LEGACY_PAGE;

  return retVal;
}
//-----------------------------------------------------------------------------
U8 readEeprom(U8 addr)
{
  EEADDR = addr;
  return EEDATA;
}
//-----------------------------------------------------------------------------
void writeEeprom(U8 addr, U8 value)
{
  EEADDR = addr;
  EEDATA = value;
}
//-----------------------------------------------------------------------------
U16 getEepromChecksum(void)
{
  U8 i;
  UU16 crc;

  SFRPAGE = CONFIG_PAGE;

  // Calculate our Checksum
  CRC0CN |= 0x10;             // 16-bit CRC
  CRC0CN |= 0x04;             // Initialize to 0xFFFF
  CRC0CN |= 0x08;             // Prime CRC engine
  for (i = 0; i < eeParamCount; ++i)
  {
    CRC0IN = readEeprom(i);
  }

  // Store the CRC result in our EEPROM
  CRC0CN &= 0x03;             // Start accessing bits 7-0 of CRC
  CRC0CN |= 0x02;

  crc.U8[LSB] = CRC0DATA;
  crc.U8[MSB] = CRC0DATA;

  SFRPAGE = LEGACY_PAGE;

  return crc.U16;
}
//-----------------------------------------------------------------------------
