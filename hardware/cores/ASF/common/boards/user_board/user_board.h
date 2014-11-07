/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>

/*! \name Oscillator Definitions
 */
//! @{

// Osc0 crystal is not mounted by default
#define FOSC0           12000000                                //!< Osc0 frequency: Hz.
#define OSC0_STARTUP    AVR32_SCIF_OSCCTRL0_STARTUP_2048_RCOSC  //!< Osc0 startup time: RCOsc periods.


#define FOSC32          32768                                   //!< Osc32 frequency: Hz.
#define OSC32_STARTUP   AVR32_SCIF_OSCCTRL32_STARTUP_8192_RCOSC //!< Osc32 startup time: RCOsc periods.

#define BOARD_OSC0_IS_XTAL       true
#define BOARD_OSC0_HZ            FOSC0
#define BOARD_OSC0_STARTUP_US    OSC_STARTUP_2048

#define BOARD_OSC32_HZ           FOSC32
#define BOARD_OSC32_STARTUP_US   OSC32_STARTUP_65536


//! @}


#endif // USER_BOARD_H
