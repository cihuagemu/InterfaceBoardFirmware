//-----------------------------------------------------------------------------
//                                                                            
//  Copyright (c) 2015 All Right Reserved                                      
//  Pressure Profile Systems                                                   
//  www.pressureprofile.com                                                    
//  V1.0                                                                       
//                                                                             
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
#include "init.h"
#include "main.h"
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Initialization Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// OSCILLATOR_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the internal precision
// oscillator with spread spectrum enabled.
//
//-----------------------------------------------------------------------------
void Oscillator_Init(void)
{
   // Save the current SFRPAGE
   U8 SFRPAGE_save = SFRPAGE;

   SFRPAGE = CONFIG_PAGE;

   //OSCICN |= 0x83;                   // Enable the precision internal osc.  //24.5 MHz

   OSCICN |= 0x80;                     // Enable the precision internal osc.  //24.5 MHz / 8 = 3MHz

   RSTSRC = 0x06;                      // Enable missing clock detector and
   // leave VDD Monitor enabled.

   CLKSEL = 0x00;                      // Select precision internal osc. 
   // divided by 1 as the system clock
   SFRPAGE = SFRPAGE_save;
}



//-----------------------------------------------------------------------------
// SMBus_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// SMBus configured as follows:
// - SMBus enabled
// - Slave mode not inhibited
// - Setup and hold time extensions enabled
//
//-----------------------------------------------------------------------------
void SMBus_Init(void)
{
   // Save the current SFRPAGE
   U8 SFRPAGE_save = SFRPAGE;


   //See datasheet 225
   SFRPAGE = LEGACY_PAGE; 
   SMB0CF = 0x11;                      // Use extended setup and hold times
   SMB0CF |= 0x80;                     // Enable SMBus;

   SFRPAGE = SFRPAGE_save;
}




//-----------------------------------------------------------------------------
// SPI_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// SMBus configured as follows:
// - Master mode
// - SPI is enabled
// - Used exclusivly to communicate with MCP4811 10bit DAC
//
//-----------------------------------------------------------------------------

void SPI_Init(void)
{
   U8 SFRPAGE_save = SFRPAGE;

   SFRPAGE = CONFIG_PAGE;
   SPI0CKR = 0x00; //Max speed - calc on datasheet page 250 

   SFRPAGE = LEGACY_PAGE;
   SPI0CFG = 0x40; //Master mode - see datasheet pg 248
   SPI0CN = 0x01; //SPI Enabled, clear all other flags - see datasheet pg 249

   SFRPAGE = SFRPAGE_save;
}




//-----------------------------------------------------------------------------
// Port_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the pins for SPI and SMbus and capacitive sensing 
// See circuit for pinout
//
//-----------------------------------------------------------------------------

void Port_Init(void)
{

   // Save the current SFRPAGE
   U8 SFRPAGE_save = SFRPAGE;
   SFRPAGE = CONFIG_PAGE;

   //Summary of settings

   // P0.4  -  SCK (SPI0),     Push-pull, Digital
   // P0.5  -  MISO (SPI0),    Open-Drain, Digital

   // P2.0  -  MOSI (SPI0),    Push-pull, Digital   
   // P2.1  -  CS  (SPI0),     Push-pull, Digital   
   // P2.2  -  SDA (SMBUS0),   Open-Drain, Digital
   // P2.3  -  SCL (SMBUS0),   Open-Drain, Digital
   // P2.4  -  PWM OUT  	,   Push-pull, Digital
   // P2.5  -  Unused
   // P2.6  -  Unused 
   // P2.7  -  DAC_EN,  	    Push-pull, Digital 

   // P4.0  -  Sense1 (CS0),   Open-Drain, Analog
   // P4.1  -  Sense2 (CS0),   Open-Drain, Analog
   // P4.2  -  Sense3 (CS0),   Open-Drain, Analog 
   // P4.3  -  Sense4 (CS0),   Open-Drain, Analog 
   // P4.4  -  Sense5 (CS0),   Open-Drain, Analog 
   // P4.5  -  Sense6 (CS0),   Open-Drain, Analog 
   // P4.6  -  Sense7 (CS0),   Open-Drain, Analog 
   // P4.7  -  Sense8 (CS0),   Open-Drain, Analog 

   // P6.4  -  Sense9 (CS0),   Open-Drain, Analog
   // P4.5  -  Sense10(CS0),   Open-Drain, Analog

   //Everything else not wired to I/O so set as a analogue inputs.


   //Configure pin outputs - note the following
   //PxMOUT 0 = open drain, 1 = push pull
   //PxMDIN 0 = analogue, 1 = digital

   //Inputs
   P0MDIN = 0x30; // All digital
   P1MDIN = 0x00;  //Not on wired to IO, configued as digital
   P2MDIN = 0xFF; //All digital
   P3MDIN = 0x00;  //Not on wired to IO, configued as digital
   P4MDIN = 0x00; //All analogue
   P5MDIN = 0x00;   //Not on wired to IO, configued as digital
   P6MDIN = 0x00; //All analogue

   //Outputs
   P0MDOUT = 0x10; //All open drai
   P1MDOUT= 0x00;  //All open drain
   P2MDOUT = 0x93; //P2.0,1 4 and 7 are push-pull outputs
   P2 = 0xFF;      //Set all to 1
   P3MDOUT= 0x00;  //All open drain
   P4MDOUT= 0x00;  //All open drain
   P5MDOUT= 0x00;  //All open drain
   P6MDOUT= 0x00;  //All open drain


   //Cross Bar
   P0SKIP = 0xCF;  //Configure crossbar to match circuit, assuming 3 pin SPI 
   P1SKIP = 0xFF;
   P2SKIP = 0x02; 

   //Configure the crossbar
   XBR0 = 0x06;                        // SMBus (4) , SPI (2)

   // Enable crossbar
   XBR1 = 0x40;                        

   SFRPAGE = SFRPAGE_save;
}

//-----------------------------------------------------------------------------
// CapSense_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the CS0 Capacitive Sensing Setup
//
//-----------------------------------------------------------------------------
void CapSense_Init(void)
{
   U8 calcTemp, calcTemp2;
   U8 SFRPAGE_save = SFRPAGE;


   SFRPAGE = LEGACY_PAGE;
   // CS0CF: Capacitive Sense Configuration
   // Bit7:   Unused
   // Bit6:4: Start Mode
   //          111 = Continuous auto-scan after writing 1 to CS0BUSY
   //          000 = Conversion initiated on every write of 1 to CS0BUSY
   // Bit 3:  Unused
   // Bit2:0: Accumulator Level
   //          000 = 1X
   //          001 = 4X
   //          010 = 8X
   //          011 = 16X
   //          100 = 32X
   //          101 = 64X
   //          11x = Reserved         

   //Retrieve from settings table
   calcTemp = (MainRegister[SET_ACCUMULATOR] <= 0x05 ? MainRegister[SET_ACCUMULATOR] : 0x05);
   CS0CF = calcTemp; //Manual start

   // Auto-Scan from P2.0 to P6.5
   //SFRPAGE = CONFIG_PAGE; 
   //CS0SS     = 0x10;  //No used as we manually set the mux
   //CS0SE     = 0x25;
   //SFRPAGE = LEGACY_PAGE;

   // Capacitive Sense Mode 1
   // Bit5:4: Secondary Reset Time
   //          00 = None
   //          01 = 0.75us
   //          10 = 1.5us
   //          11 = 2.25us
   // Bit2:0: Reference Gain
   //          000 = 1X
   //          001 = 2X
   //          010 = 3X
   //          011 = 4X
   //          100 = 5X
   //          101 = 6X
   //          110 = 7X
   //          111 = 8X
   CS0MD1 = 0x00 | (MainRegister[SET_REFERENCEGAIN] & 0x07);

   // Capacitive Sense Mode 2
   // Bit7:6: Conversion Rate / Resolution
   //          00 = 12 clocks / 12 bits
   //          01 = 13 clocks / 13 bits
   //          10 = 14 clocks / 14 bits
   //          11 = 16 clocks / 16 bits
   // Bit5:3: Discharge Time
   //          000 = 0.75us
   //          001 = 1.0us
   //          010 = 1.2us
   //          011 = 1.5us
   //          100 = 2us
   //          101 = 3us
   //          110 = 6us
   //          111 = 12us
   // Bit2:0: Output Current
   //          000 = Full
   //          001 = 1/8
   //          010 = 1/4
   //          011 = 3/8
   //          100 = 1/2
   //          101 = 5/8
   //          110 = 3/4
   //          111 = 7/8

   SFRPAGE = CONFIG_PAGE;
   calcTemp = (MainRegister[SET_DISCHARGETIME] <= 0x07 ? MainRegister[SET_DISCHARGETIME] : 0x07);
   calcTemp = calcTemp << 3;
   calcTemp2 = (MainRegister[SET_OUTPUTCURRENT] <= 0x07 ? MainRegister[SET_OUTPUTCURRENT] : 0x07);
   CS0MD2 = ((0xC0 | calcTemp) | calcTemp2);

   // Capacitive Sense Pin Monitor
   // Bit7:   UART Pin Monitor
   // Bit6:   SPI Pin Monitor
   // Bit5:   SMBus (I2C) Pin Montior
   // Bit4:   PCA Pin Monitor
   // Bit3:   Port I/O Pin Monitor
   // Bit2:   CP0 Pin Monitor
   // Bit1:0: Pin Monitor Mode:
   //          00 = Infinite retry
   //          01 = Retry 2X max
   //          10 = Retry 4X max
   //          11 = Reserved
   CS0PM = 0xFC;

   // Enable CapSense Complete interrupts
   EIE2 |= 0x01;

   SFRPAGE = SFRPAGE_save;
}



//-----------------------------------------------------------------------------
// VddMon_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Enable VDD Monitor
//
//-----------------------------------------------------------------------------
void VddMon_Init(void)
{
   // On both SRF pages so no need to set

   // Enable the VDD Monitor
   VDM0CN = 0x80;

   // Select the VDD Monitor as a reset source
   RSTSRC = 0x02;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Timers_Init 
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
//Timer 0 -> PCA - Timestamp
//Timer 1 - Not used
//Timer 2 - Not used
//
//-----------------------------------------------------------------------------
void Timers_Init (void)
{
   U8 SFRPAGE_save = SFRPAGE;

   //Following SRF values are on both pages
   TMOD = 0x22; // Timer1 in 8-bit auto-reload mode, Timer 0 is 8bit auto-reload mode.

   //Timer 0 - PCA
   CKCON &= ~0x04;     // Use clock prescaler
   CKCON &= ~0x03;     // Prescaler is SYSCLK / 12 (which is 6/12MHz = 500kHz)

   //Run at 10kHz (it is auto reload mode so go from 206 - 256 with a 500kHz clock)
   TH0 = -(SYSCLK / 2 / 10000);  
   TL0 = 0x00;

   TR0 = 1; //Enable Timer0;


   SFRPAGE = CONFIG_PAGE;

   PCA0MD = 0x04;//0x04 //  Triggered on Timer 0 overflow

   PCA0CPM0 = 0x49;//01001001
   PCA0PWM = 0x00;

   SFRPAGE = LEGACY_PAGE;

   PCA0CPL0 = 0x10; //10,000 (1 second if Timer 0 is 1MHz)
   PCA0CPH0 = 0x27;

   CR =1; //PCA timer enabled

   SFRPAGE = SFRPAGE_save;
}

