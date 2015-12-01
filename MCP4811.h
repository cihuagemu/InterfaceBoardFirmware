#ifndef MCP4811_def
#define MCP4811_def
      
#include "compiler_defs.h"
#include "C8051F700_defs.h"
#include "types.h"

extern U8 Micron_CheckID();
extern U8 Micron_Init();
extern U8 Micron_EnableWrite();
extern U8 Micron_WriteFlashBuffer();
extern U8 Micron_ReadFlashBuffer();
extern U8 Micron_EraseDevice();
extern void Micron_SetAddress(U32 CurrentAddress);

extern U8 xdata MainRegister[MAIN_REGISTER_LENGTH];  


#endif
