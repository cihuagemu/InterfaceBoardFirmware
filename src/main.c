//-----------------------------------------------------------------------------
//  Copyright (c) 2015 Pressure Profile Systems
//
//  Licensed under the MIT license. This file may not be copied, modified, or
//  distributed except according to those terms.
//-----------------------------------------------------------------------------


#include "compiler_defs.h"
#include "C8051F700_defs.h"
#include "init.h"
#include "main.h"
#include "F700_FlashUtils.h"
#include "MCP4811.h"
//#include "calibration.h"

U8 SmbAddress;
volatile U8 SmbState = SMB_IDLE;
bit IsScanReady = 0;
bit IsScanning;
UU16 FrameCounter;

volatile U8 ChannelIndex = 0; //Index of the channel we are scanning
volatile bit IsBaselineSettled = 0;
volatile bit IsCalibrated = 0;
volatile bit IsSensorSizeDetected = 0;
volatile bit IsStartup = 0;
volatile bit IsFirstFrame = 0;

const U8 CHANNEL_LIST[] = {16,17,18,19,20,21,22,23,36,37}; //Conversion to encoding on datasheet pg 97

U8 code FlashCheck[3] _at_ FLASH_CHECK_ADDRESS;  //Just a check that the flash has been programmed
U8 code SetupInfo[N_SETTINGS+1] _at_ FLASH_ADDR_SETUP;  //Our flash data



// Global holder for SMBus data.
// All receive data is written here
// NUM_BYTES_WR used because an SMBus write is Master->Slave
volatile U8 data SmbBufferIn[SMB_BUF_SIZE_IN];
volatile U8 data * const data SmbBufferInBegin = &SmbBufferIn[0];
volatile U8 data * const data SmbBufferInEnd = &SmbBufferIn[SMB_BUF_SIZE_IN];

//Our sensor buffer (+1 for itr +1 for timestamp)
volatile U8 xdata SensorRawBuffer[SET_MAX_NUM_SENSORS];

//Main register used for settings and SMbus
volatile U8 xdata MainRegister[MAIN_REGISTER_LENGTH];
volatile U8 SmbReadLocation = SENSOR_DATA_LOCATION;  //Location in main register (defaults to read location)

//Function Prototypes
void SMBus_ISR (void);
void CapSense_Isr(void);
void ProcessCommandFast();
void ProcessCommand();
bit LoadFlash();
void SaveFlash();
void SetUpCapSense();
void PerformDACFunction();


// CapSense Controls
#define CapSenseStart()       (CS0CN |= 0x10)
#define CapSenseClearInt()    (CS0CN &= 0xDF)
#define CapSenseEnable()      (CS0CN |= 0x80)
#define CapSenseDisable()     (CS0CN &= ~0x80)
#define ResetMcu()			      (RSTSRC |= 0x10);

//Chip select for analogue output
sbit PinAnalogCS = P2^1;
sbit PinDigitalOut = P2^4;

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
//
// Main routine performs all configuration tasks, then waits for SMBus
// communication.
//
//-----------------------------------------------------------------------------
void main (void)
{
  U8 i, n;

  // Disable watchdog timer
  WDTCN = 0xDE;
  WDTCN = 0xAD;

  PinAnalogCS = 1;  //Disable chip select

  // Detect FLASH error by first making sure we're not recovering
  // from a power issue (since that means other flags are random)
  if ( (RSTSRC & 0x02) != 0x02 )
  {
    // Power is okay, so check FLASH
    if ( (RSTSRC & 0x40) == 0x40 )
    {
      // Oops. FLASH error. Life is bad.
      while (1)
      {
        i = 0;
      }
    }
  }

  SmbAddress = (SLAVE_ADDR_DEF << 1);

  Oscillator_Init();
  VddMon_Init();
  Port_Init();
  Timers_Init();
  SMBus_Init();
  SPI_Init();

  if (LoadFlash() ) //Flash good
  {
    // Load in our address and other things from EEPROM
    SmbAddress = (MainRegister[SET_SMBUSADDRESS] << 1);
  }
  else //Flash bad (most likely a clean chip)7
  {
    for(i = 0; i < MAIN_REGISTER_LENGTH; i++)
    {
      MainRegister[i] = 0; //Initialise the main register

      if(i < SET_MAX_NUM_SENSORS)
      {
        SensorRawBuffer[i] = 0;//Initialise sensor buffer
      }
    }

    MainRegister[SET_SMBUSADDRESS] = 0x04; //Default
    SmbAddress = (MainRegister[SET_SMBUSADDRESS] << 1);

    MainRegister[SET_SERIALNUMBERMSB] = SERIAL_NUM_DEF >> 8;
    MainRegister[SET_SERIALNUMBERLSB] = SERIAL_NUM_DEF & 0xFF;

    MainRegister[SET_NUMBERELEMENTS] = 1;
    MainRegister[SET_SCALINGANALOGUEOUTMSB] = 0x01;
    MainRegister[SET_SCALINGANALOGUEOUTLSB] = 0x2C;

    //Capacitor Sensing Settings - Refer to CY8051F70x datasheet, chapter 15 for details
    MainRegister[SET_REFERENCEGAIN] = 1;
    MainRegister[SET_DISCHARGETIME] = 3;
    MainRegister[SET_ACCUMULATOR] = 5; //32x

    MainRegister[SET_SETTINGSPARAMCOUNT] = 0xFF;
    MainRegister[SET_LENGTHSCANLIST] = 0xFF;
    MainRegister[SET_MAX_NUM_SENSORS] = 0xFF;
    MainRegister[N_SETTINGS] = 0xFF;
  }

  CapSense_Init();

  #ifdef CALIBRATION
  if ( CheckCalFlash() ) //Sensor has been calibrated
  {
    // We have the calibration table stored, and prevent potential writing to flash
    IsCalibrated = 1;
    MainRegister[SET_RESERVED] = 1;
  }
  else //
  {
      IsCalibrated = 0;
  }
  
  #endif

  IsBaselineSettled = 0;
  IsStartup = 0;

  // Enable the SMBus interrupt
  EIE1 |= 0x01;

  // Global interrupt enable
  EA = 1;

  FrameCounter.U16 = 0; //Reset

  CapSenseEnable();
  SetUpCapSense();
  CapSenseClearInt();
  IsScanning = TRUE;

  

  // Main dispatch loop
  while (1)
  {
    switch (SmbState)
    {
      case SMB_RECEIVING:
        break;

      case SMB_RECEIVED:
        ProcessCommand();
        SmbState = SMB_IDLE;
        break;

      case SMB_SENDING:
        break;

      case SMB_SENT:
        SmbReadLocation = SENSOR_DATA_LOCATION; //Reset SMBus read itr
        SmbState = SMB_IDLE;

        if(!IsFirstFrame)
        {
            IsBaselineSettled = 0;
            IsFirstFrame = 1;
        }

        break;

      case SMB_ERROR:
        SmbState = SMB_IDLE;
        break;

      case SMB_IDLE:
        if (IsScanReady)
        {
          // Copy latest data to outgoing buffer
          EA = 0;

          for (i = 0, n = 4; i < n; ++i) //4 bytes for frame index and timestamp
          {
            MainRegister[SENSOR_DATA_LOCATION+i] = SensorRawBuffer[i];
          }
           
          for (i = 0, n = MainRegister[SET_NUMBERELEMENTS]; i < n; ++i)
          {
            UU32 newValue;
            U32 oldValue = 0;
            //U32 a, b, c;

            newValue.U32 = (SensorRawBuffer[2*i+4]<<8) + SensorRawBuffer[2*i+5];
            newValue.U32 = newValue.U32 * 50;

            oldValue = (MainRegister[SENSOR_DATA_LOCATION+i+4]<<8) + MainRegister[SENSOR_DATA_LOCATION+i+5];
            oldValue = oldValue * 50;

            newValue.U32 = (newValue.U32 + oldValue) /100;
            MainRegister[SENSOR_DATA_LOCATION+i+4] = newValue.U8[2];
            MainRegister[SENSOR_DATA_LOCATION+i+5] = newValue.U8[3];

            SensorRawBuffer[2*i+4] = newValue.U8[2];
            SensorRawBuffer[2*i+5] = newValue.U8[3];

            
          }

          
          
          PerformDACFunction();

          IsScanReady = 0;

          EA = 1;  //Reenable interrupts

          //Start the next scan
          SetUpCapSense();
        }
        break;
    }
  } // End of main processing loop
}


//-----------------------------------------------------------------------------
// PerformDACFunction
//-----------------------------------------------------------------------------
//
// Send selected value to the DAC
//
//-----------------------------------------------------------------------------
void PerformDACFunction()
{
  U8 sensorIndex;
  U16 dacOut;
  UU16 analogueValue;

  //This is the sensor we are outputing
  sensorIndex = MainRegister[SET_SENSORMAPPINGPWM_ANALOGUE];

  //Cap it if it has gone too high (corrupted flash)
  if(sensorIndex > 24)
  {
    MainRegister[PARAM_ERROR] = ERROR_ANALOGPWMOUTPUTINDEXINVALID;
    sensorIndex = 0;
  }

  //Retrieve the sensor value
  analogueValue.U8[0] = SensorRawBuffer[sensorIndex*2 + SET_SENSOR0];
  analogueValue.U8[1] = SensorRawBuffer[sensorIndex*2 + SET_SENSOR0b];

  analogueValue.U16 = (analogueValue.U16 < 1024 ? analogueValue.U16 : 1024);

  //Format data in the right format for the DAC chip (setting DAC enabled, gain = 1x)
  dacOut = (analogueValue.U16 << 2) | 0x3000; //  /GA = 1 | /SHDN = 1 = 0x3xxx;

  //Send of SPI to DAC chip
  SFRPAGE = LEGACY_PAGE;
  PinAnalogCS = 0; //CS low

  SPI0DAT = (dacOut >> 8); //MSByte byte - 0x03 sets DAC options
  while (SPI0CN & 0x02 == 0); //Send first byte

  SPI0DAT = (dacOut & 0xFF); ///LSByte byte
  while (SPI0CN & 0x02 == 0); //Send second byte

  while ((SPI0CFG & 0x80)); //Wait for write to complete

  PinAnalogCS = 1; //CS high

  // Wait 100 convertions and we tare our baseline after we startup
   if(!IsStartup && MainRegister[SET_BASELINE23] > 100)
   {
      IsBaselineSettled = 0;
      IsStartup = 1;
   } else
   {
      MainRegister[SET_BASELINE23]++;
   }

}

//-----------------------------------------------------------------------------
// Process SMBus command (these command(s) are fast enough to occur in the interupt
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
void processCommandFast()
{
  EA = 0;

  switch (SmbBufferIn[0])
  {
    case CMD_READ:
      SmbReadLocation = + SmbBufferIn[1];

      if(&MainRegister[0] + SmbBufferIn[2] > MAIN_REGISTER_LENGTH)
      {
        MainRegister[PARAM_ERROR] = ERROR_REQUESTEDTOOMUCHDATA;
      }
      break;
  }
  EA = 1;
}

//-----------------------------------------------------------------------------
// Process SMBus command
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
void ProcessCommand()
{
  U8 iBuffer;
  U8 writeLocation;
  U8 packetLength;

  EA = 0;

  // Process new Incoming Command
  switch (SmbBufferIn[0])
  {
    case CMD_WRITE:   // 0x02
      writeLocation = SmbBufferIn[1];
      packetLength = SmbBufferIn[2];

      if(SmbBufferIn[packetLength+3] != 0xFF) //Truncated packet
      {
        SmbBufferIn[packetLength+3] = 0;  //Clear the flag
        MainRegister[PARAM_ERROR] = ERROR_TRUNCATEDPACKET;
        return;
      }

      SmbBufferIn[packetLength+3] = 0;   //Clear the flag

      if(writeLocation + packetLength > PARAMSLOCATION)
      {
        MainRegister[PARAM_ERROR] = ERROR_BUFFEROVERWRITE;
        return; // Too much data
      }

      for(iBuffer = 0; iBuffer < packetLength; iBuffer++)
      {
        MainRegister[iBuffer + writeLocation] = SmbBufferIn[iBuffer+3];
      }

      SaveFlash();
      ResetMcu();
      break;

    #ifdef CALIBRATION

    // Write the Calibration data into the Flash
    case CMD_WRITE_CAL:   // 0x04
      writeLocation = SmbBufferIn[1];
      packetLength = SmbBufferIn[2];

      if(SmbBufferIn[packetLength+3] != 0xFF) //Truncated packet
      {
         SmbBufferIn[packetLength+3] = 0;  //Clear the flag
         return;
      }

      SmbBufferIn[packetLength+3] = 0;   //Clear the flag

      for(iBuffer = 0; iBuffer < packetLength; iBuffer++)
      {
         // Seems directly using SMB Buffer, the flash will has some wierd function
         MainRegister[SET_SENSOR6 + iBuffer] = SmbBufferIn[iBuffer+3];
      }

      
      SaveCalibrationToFlash(writeLocation, packetLength);
      
      ResetMcu();
      break;

    #endif

    default:
      processCommandFast();
      break;
  }

  EA = 1;
}
//-----

//-----------------------------------------------------------------------------
// SMBus Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// SMBus ISR state machine
// - Slave only implementation - no master states defined
// - All incoming data is written to global variable <SMB_data_IN>
// - All outgoing data is read from global variable <SMB_data_OUT>
//
//-----------------------------------------------------------------------------
void SMBus_ISR (void) interrupt 7
{
  static U8 data * data dataIn;
  static U8 dataOut;
  volatile U8 data smbDat = SMB0DAT;

  if (0 == ARBLOST)
  {
    switch (SMB0CN & 0xF0)           // Decode the SMBus status vector
    {
      // Slave Receiver: Start+Address received
      case  SMB_SRADD:
        STA = 0;                   // Clear STA bit

        // Check the address for a match
        if ( (smbDat & 0xFE) == (SmbAddress & 0xFE) || (smbDat & 0xFE) == ((SLAVE_ADDR_DEF << 1) & 0xFE))
        {
          if (SMB_RECEIVING == SmbState)
          {
            processCommandFast();
          }

          ACK = 1;

          // Initialize our buffer pointers
          dataIn = SmbBufferInBegin;
          dataOut = SmbReadLocation;

          if (smbDat & 0x01) // If the transfer is a master READ,
          {
            // Host is reading, we are sending
            SmbState = SMB_SENDING;
            SMB0DAT = MainRegister[dataOut++];
          }
          else
          {
            // Host is writing, we are receiving
            SmbState = SMB_RECEIVING;
          }
        }
        else
        {
          // Address doesn't match
          ACK = 0;
        }
        break;

      // Slave Receiver: Data received
      case  SMB_SRDB:
        // Store incoming data if there's still room
        if (dataIn < SmbBufferInEnd)
        {
          *dataIn++ = smbDat;
        }
        ACK = 1;                // ACK received data
        break;

      // Slave Receiver: Stop received while either a Slave Receiver or
      // Slave Transmitter
      case  SMB_SRSTO:
        STO = 0;                   // STO must be cleared by software when
        // a STOP is detected as a slave
        //DebugSet();
        if (SMB_RECEIVING == SmbState)
        {
          SmbState = SMB_RECEIVED;
        }
        else if (SMB_SENDING == SmbState)
        {
          SmbState = SMB_SENT;
        }
        else
        {
          // Somehow we got a Stop even though we weren't sending or receiving
          SmbState = SMB_ERROR;
        }
        break;

      // Slave Transmitter: Data byte transmitted
      case  SMB_STDB:
        if (ACK)
        {
          // Send data if it's available
          if (dataOut < (MAIN_REGISTER_LENGTH))
          {
            SMB0DAT = MainRegister[dataOut++];
          }
          else
          {
            // Use dummy data
            SMB0DAT = 0xDA;
          }
        }
        break;

      // Slave Transmitter: Arbitration lost, Stop detected
      //
      // This state will only be entered on a bus error condition.
      // In normal operation, the slave is no longer sending data or has
      // data pending when a STOP is received from the master, so the TXMODE
      // bit is cleared and the slave goes to the SRSTO state.
      case  SMB_STSTO:
        STO = 0;                   // STO must be cleared by software when
        // a STOP is detected as a slave
        SmbState = SMB_ERROR;
        break;

      // Default: all other cases undefined
      default:
        SMB0CF &= ~0x80;           // Reset communication
        SMB0CF |= 0x80;
        STA = 0;
        STO = 0;
        ACK = 0;
        SmbState = SMB_ERROR;
        break;
    }
  }
  // ARBLOST = 1, Abort failed transfer
  else
  {
    STA = 0;
    STO = 0;
    ACK = 0;
    //SmbState = SmbError;
    // For now treat this like a normal stop
    if (SMB_RECEIVING == SmbState)
    {
      SmbState = SMB_RECEIVED;
    }
    else if (SMB_SENDING == SmbState)
    {
      SmbState = SMB_SENT;
    }
    else
    {
      SmbState = SMB_ERROR;
    }
  }

  SI = 0;                             // Clear SMBus interrupt flag
}


//-----------------------------------------------------------------------------
// CS0 Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
INTERRUPT(CapSense_Isr, INTERRUPT_CS0_EOC)
{
  
     UU16 baseline;
     UU32 temp;
     U16 tempScaling;
  
     // Only defined for Calibration Purpose 
     UU16 calTemp1;
     UU16 calTemp2;
     UU16 calTempCal;

     // Disable interrupts so we don't deal with IO while we're
     // processing the new data
     EA = 0;

     PinDigitalOut = 1;  //Set pin on

      // After calibration all the Gain setting shouldn't be detected
      if(!IsCalibrated)
      {
         temp.U32 = CS0D; 
         if(!IsSensorSizeDetected)
         {
            if(MainRegister[SET_REFERENCEGAIN] < 2)
            {
               MainRegister[SET_REFERENCEGAIN] = temp.U32 > 0x8000 ? 0x01 : 0x05;
               CS0MD1 = 0x00 | (MainRegister[SET_REFERENCEGAIN] & 0x07);
            } 

            else
            {
               MainRegister[SET_REFERENCEGAIN] = temp.U32 > 0xFEEE ? 0x01 : 0x05;
               CS0MD1 = 0x00 | (MainRegister[SET_REFERENCEGAIN] & 0x07);
            }
         
            IsSensorSizeDetected = 1;
         }
      }

      // Baseline Tare code
      if(!IsBaselineSettled)
      {
         temp.U32 = CS0D; 
         MainRegister[SET_BASELINE0] = temp.U8[2];
         MainRegister[SET_BASELINE0b] = temp.U8[3];
         IsBaselineSettled = 1;
      
      }

     temp.U32 = MainRegister[SET_BASELINE0+MainRegister[SET_SCANLIST0+ChannelIndex]*2];
     baseline.U16 = (temp.U32 <<8) + MainRegister[SET_BASELINE0b+MainRegister[SET_SCANLIST0+ChannelIndex]*2];
     tempScaling = (MainRegister[SET_SCALINGANALOGUEOUTMSB]<<8) + MainRegister[SET_SCALINGANALOGUEOUTLSB];

     tempScaling = (tempScaling < 1 ? 1: tempScaling); //We don't want a divide by zero error

     SensorRawBuffer[0] = FrameCounter.U8[0];
     SensorRawBuffer[1] = FrameCounter.U8[1];
     SensorRawBuffer[2] = PCA0L;
     SensorRawBuffer[3] = PCA0H;

     temp.U32 = tempScaling;
     temp.U32 = (temp.U32 << 8)/100; //(Add 255)
     temp.U32 += CS0D;

      if(temp.U32 > (baseline.U16 ))
      {
      
         temp.U32 -= baseline.U16;
         temp.U32 = (temp.U32 *100)/ tempScaling;

         #ifdef CALIBRATION
         // Calibration Calculation
         //calTemp1.U8[0] = temp.U8[2];
         //calTemp1.U8[1] = temp.U8[3];


         if(IsCalibrated){

            calTemp1.U8[0] = temp.U8[2];
            calTemp1.U8[1] = temp.U8[3];

            calTempCal.U16 = calTemp1.U16 >> 3;
            calTempCal.U16 = calTempCal.U16 << 1;

            calTemp1.U8[0] = CalInfo1[calTempCal.U16];
            calTemp1.U8[1] = CalInfo1[calTempCal.U16+1];

            calTemp2.U8[0] = CalInfo1[calTempCal.U16+2];
            calTemp2.U8[1] = CalInfo1[calTempCal.U16+3];

            calTemp2.U16 = calTemp2.U16 - calTemp1.U16;
            calTemp2.U16 = (calTemp2.U16 * (temp.U8[2] & 0x07)) >> 3;
            calTemp1.U16 = calTemp2.U16 + calTemp1.U16;

            if(calTemp1.U16 >= 0x3FF) //Cap if it is too large 
               calTemp1.U16 = 0x3FF;

            if(calTemp1.U16 <= 0) //Minimum is zero
               calTemp1.U16 = 0;

            SensorRawBuffer[ChannelIndex*2 + 4] = calTemp1.U8[0]; //Big endian
            SensorRawBuffer[ChannelIndex*2 + 5] = calTemp1.U8[1];
      
         } 
   
         else {
            SensorRawBuffer[ChannelIndex*2 + 4] = temp.U8[2]; //Big endian
            SensorRawBuffer[ChannelIndex*2 + 5] = temp.U8[3];
         }
  

         #else

         SensorRawBuffer[ChannelIndex*2 + 4] = temp.U8[2]; //Big endian
         SensorRawBuffer[ChannelIndex*2 + 5] = temp.U8[3];

         #endif

     }

     else
     {
         SensorRawBuffer[ChannelIndex*2 + 4] = 0x00; //Just send 0x00 to signify that we are out of range
         SensorRawBuffer[ChannelIndex*2 + 5] = 0x00;
     }

     ChannelIndex++;

     if (ChannelIndex >= MainRegister[SET_NUMBERELEMENTS])
     {
       FrameCounter.U16 ++;
       ChannelIndex = 0;
       IsScanReady = 1;
     }

     CapSenseClearInt();

     EA = 1;
     PinDigitalOut = 0;  //Set pin off
}

//-----------------------------------------------------------------------------
// SetUpCapSense
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------
void SetUpCapSense()
{
  U8 channel = MainRegister[SET_SCANLIST0+ChannelIndex];

  if(channel >9)
  {
    channel = 9; //Max 10 channels
  }

  CS0MX = CHANNEL_LIST[channel];
  CapSenseStart();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
// Load settings from flash
//-----------------------------------------------------------------------------
bit LoadFlash(void)
{
  U8 i, index;

  index = 0;

  if(FlashCheck[0] == 0xFF && FlashCheck[1] == 0xFE && FlashCheck[2] == 0xFD)
  {
    // And now all of our other setup stuff
    for (i = 0; i < N_SETTINGS; ++i)
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
// Save settings to flash
//-----------------------------------------------------------------------------
void SaveFlash(void)
{
  U8 flashCheckWrite[3] = {0xFF,0xFE,0xFD};

  // Erase our configuration Flash page
  FLASH_PageErase( (FLADDR)(&FlashCheck[0]) );

  // And copy our updated data into it
  FLASH_Write(
    (FLADDR)(&FlashCheck[0]),
    (U8 *)(&flashCheckWrite[0]),
    3
  );

  // And copy our updated data into it
  FLASH_Write(
    (FLADDR)(&SetupInfo[0]),
    (U8 xdata *)(&MainRegister[0]),
    N_SETTINGS
  );
}
