# InterfaceBoardFirmware

This repo contains the firmware for driving the SingleTact sensors.

# Setting up the development environment

## Clone this repo
In the command prompt (or otherwise), run:

```
git clone git@github.com:SingleTact/InterfaceBoardFirmware.git
```

## Building...
Programming the C8051F717 requires an 8 bit programmer which can be purchased from Digikey (336-1182-ND).

Programming requires the Silicon Labs IDE and free Keil compiler which can be downloaded from: http://www.silabs.com/products/mcu/Pages/8-bit-microcontroller-software.aspx

More information can be found in the SingeTact Manual

Note, the source code for calibration is too large for the free Keil license.   To use this code, purchase a Keil license, then include "calibration.h" from main.c and make sure "calibration.c" is added to the build list.

# Found a bug?
If you have been so lucky to discover a bug, or would like to suggest a
feature, or just discuss stuff, feel free to
[submit an issue](https://github.com/SingleTact/InterfaceBoardFirmware/issues).

# License
Standard MIT license applies. See [LICENSE.md](LICENSE.md) for details.
