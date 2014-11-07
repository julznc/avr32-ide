/*
  probe.c - code pertaining to probing methods
  Part of Grbl v0.9

  Copyright (c) 2014 Sungeun K. Jeon

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Grbl.h"



// Probe pin initialization routine.
void Grbl::Probe::init()
{

}


// Returns the probe pin state. Triggered = true. Called by gcode parser and probe state monitor.
uint8_t Grbl::Probe::get_state() { return 0; }


// Monitors probe pin state and records the system position when detected. Called by the
// stepper ISR per ISR tick.
// NOTE: This function must be extremely efficient as to not bog down the stepper ISR.
void Grbl::Probe::state_monitor()
{

}
