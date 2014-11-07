/*
  report.c - reporting and messaging methods
  Part of Grbl v0.9

  Copyright (c) 2012-2014 Sungeun K. Jeon  

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

/* 
  This file functions as the primary feedback interface for Grbl. Any outgoing data, such 
  as the protocol status messages, feedback messages, and status reports, are stored here.
  For the most part, these functions primarily are called from protocol.c methods. If a 
  different style feedback is desired (i.e. JSON), then a user can change these following 
  methods to accomodate their needs.
*/

#include "Grbl.h"
#include <bsp_clock.h>

// Handles the primary confirmation protocol response for streaming interfaces and human-feedback.
// For every incoming line, this method responds with an 'ok' for a successful command or an 
// 'error:'  to indicate some error event with the line or some critical system error during 
// operation. Errors events can originate from the g-code parser, settings module, or asynchronously
// from a critical error, such as a triggered hard limit. Interface should always monitor for these
// responses.
// NOTE: In silent mode, all error codes are greater than zero.
// TODO: Install silent mode to return only numeric values, primarily for GUIs.
void Grbl::Report::status_message(uint8_t status_code) 
{
  if (status_code == 0) { // STATUS_OK
    parent->printString("ok\r\n");
  } else {
    parent->printString("error: ");
    switch(status_code) {          
      case STATUS_EXPECTED_COMMAND_LETTER:
      parent->printString("Expected command letter"); break;
      case STATUS_BAD_NUMBER_FORMAT:
      parent->printString("Bad number format"); break;
      case STATUS_INVALID_STATEMENT:
      parent->printString("Invalid statement"); break;
      case STATUS_NEGATIVE_VALUE:
      parent->printString("Value < 0"); break;
      case STATUS_SETTING_DISABLED:
      parent->printString("Setting disabled"); break;
      case STATUS_SETTING_STEP_PULSE_MIN:
      parent->printString("Value < 3 usec"); break;
      case STATUS_SETTING_READ_FAIL:
      parent->printString("EEPROM read fail. Using defaults"); break;
      case STATUS_IDLE_ERROR:
      parent->printString("Not idle"); break;
      case STATUS_ALARM_LOCK:
      parent->printString("Alarm lock"); break;
      case STATUS_SOFT_LIMIT_ERROR:
      parent->printString("Homing not enabled"); break;
      case STATUS_OVERFLOW:
      parent->printString("Line overflow"); break; 
      
      // Common g-code parser errors.
      case STATUS_GCODE_MODAL_GROUP_VIOLATION:
      parent->printString("Modal group violation"); break;
      case STATUS_GCODE_UNSUPPORTED_COMMAND:
      parent->printString("Unsupported command"); break;
      case STATUS_GCODE_UNDEFINED_FEED_RATE:
      parent->printString("Undefined feed rate"); break;
      default:
        // Remaining g-code parser errors with error codes
        parent->printString("Invalid gcode ID:");
        parent->print_uint8_base10(status_code); // Print error code for user reference
    }
    parent->printString("\r\n");
  }
}

// Prints alarm messages.
void Grbl::Report::alarm_message(int8_t alarm_code)
{
  parent->printString("ALARM: ");
  switch (alarm_code) {
    case ALARM_LIMIT_ERROR: 
    parent->printString("Hard/soft limit"); break;
    case ALARM_ABORT_CYCLE: 
    parent->printString("Abort during cycle"); break;
    case ALARM_PROBE_FAIL:
    parent->printString("Probe fail"); break;
  }
  parent->printString("\r\n");
  delay_ms(500); // Force delay to ensure message clears serial write buffer.
}

// Prints feedback messages. This serves as a centralized method to provide additional
// user feedback for things that are not of the status/alarm message protocol. These are
// messages such as setup warnings, switch toggling, and how to exit alarms.
// NOTE: For interfaces, messages are always placed within brackets. And if silent mode
// is installed, the message number codes are less than zero.
// TODO: Install silence feedback messages option in settings
void Grbl::Report::feedback_message(uint8_t message_code)
{
  parent->printString("[");
  switch(message_code) {
    case MESSAGE_CRITICAL_EVENT:
    parent->printString("Reset to continue"); break;
    case MESSAGE_ALARM_LOCK:
    parent->printString("'$H'|'$X' to unlock"); break;
    case MESSAGE_ALARM_UNLOCK:
    parent->printString("Caution: Unlocked"); break;
    case MESSAGE_ENABLED:
    parent->printString("Enabled"); break;
    case MESSAGE_DISABLED:
    parent->printString("Disabled"); break; 
  }
  parent->printString("]\r\n");
}


// Welcome message
void Grbl::Report::init_message()
{
  parent->printString("\r\nGrbl " GRBL_VERSION " ['$' for help]\r\n");
}

// Grbl help message
void Grbl::Report::grbl_help() {
  parent->printString("$$ (view Grbl settings)\r\n"
                      "$# (view # parameters)\r\n"
                      "$G (view parser state)\r\n"
                      "$I (view build info)\r\n"
                      "$N (view startup blocks)\r\n"
                      "$x=value (save Grbl setting)\r\n"
                      "$Nx=line (save startup block)\r\n"
                      "$C (check gcode mode)\r\n"
                      "$X (kill alarm lock)\r\n"
                      "$H (run homing cycle)\r\n"
                      "~ (cycle start)\r\n"
                      "! (feed hold)\r\n"
                      "? (current status)\r\n"
                      "ctrl-x (reset Grbl)\r\n");
}


// Grbl global settings print out.
// NOTE: The numbering scheme here must correlate to storing in settings.c
void Grbl::Report::grbl_settings() {
  // Print Grbl settings.
  parent->printString("$0="); parent->print_uint8_base10(parent->m_settings.settings.pulse_microseconds);
  parent->printString(" (step pulse, usec)\r\n$1="); parent->print_uint8_base10(parent->m_settings.settings.stepper_idle_lock_time);
  parent->printString(" (step idle delay, msec)\r\n$2="); parent->print_uint8_base10(parent->m_settings.settings.step_invert_mask); 
  parent->printString(" (step port invert mask:"); parent->print_uint8_base2(parent->m_settings.settings.step_invert_mask);  
  parent->printString(")\r\n$3="); parent->print_uint8_base10(parent->m_settings.settings.dir_invert_mask); 
  parent->printString(" (dir port invert mask:"); parent->print_uint8_base2(parent->m_settings.settings.dir_invert_mask);  
  parent->printString(")\r\n$4="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_INVERT_ST_ENABLE));
  parent->printString(" (step enable invert, bool)\r\n$5="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_INVERT_LIMIT_PINS));
  parent->printString(" (limit pins invert, bool)\r\n$6="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_INVERT_PROBE_PIN));
  parent->printString(" (probe pin invert, bool)\r\n$10="); parent->print_uint8_base10(parent->m_settings.settings.status_report_mask);
  parent->printString(" (status report mask:"); parent->print_uint8_base2(parent->m_settings.settings.status_report_mask);
  parent->printString(")\r\n$11="); parent->printFloat_SettingValue(parent->m_settings.settings.junction_deviation);
  parent->printString(" (junction deviation, mm)\r\n$12="); parent->printFloat_SettingValue(parent->m_settings.settings.arc_tolerance);
  parent->printString(" (arc tolerance, mm)\r\n$13="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_REPORT_INCHES));
  parent->printString(" (report inches, bool)\r\n$14="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_AUTO_START));
  parent->printString(" (auto start, bool)\r\n$20="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_SOFT_LIMIT_ENABLE));
  parent->printString(" (soft limits, bool)\r\n$21="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_HARD_LIMIT_ENABLE));
  parent->printString(" (hard limits, bool)\r\n$22="); parent->print_uint8_base10(bit_istrue(parent->m_settings.settings.flags,BITFLAG_HOMING_ENABLE));
  parent->printString(" (homing cycle, bool)\r\n$23="); parent->print_uint8_base10(parent->m_settings.settings.homing_dir_mask);
  parent->printString(" (homing dir invert mask:"); parent->print_uint8_base2(parent->m_settings.settings.homing_dir_mask);  
  parent->printString(")\r\n$24="); parent->printFloat_SettingValue(parent->m_settings.settings.homing_feed_rate);
  parent->printString(" (homing feed, mm/min)\r\n$25="); parent->printFloat_SettingValue(parent->m_settings.settings.homing_seek_rate);
  parent->printString(" (homing seek, mm/min)\r\n$26="); parent->print_uint8_base10(parent->m_settings.settings.homing_debounce_delay);
  parent->printString(" (homing debounce, msec)\r\n$27="); parent->printFloat_SettingValue(parent->m_settings.settings.homing_pulloff);
  parent->printString(" (homing pull-off, mm)\r\n");

  // Print axis settings
  uint8_t idx, set_idx;
  uint8_t val = AXIS_SETTINGS_START_VAL;
  for (set_idx=0; set_idx<AXIS_N_SETTINGS; set_idx++) {
    for (idx=0; idx<N_AXIS; idx++) {
      parent->printString("$");
      parent->print_uint8_base10(val+idx);
      parent->printString("=");
      switch (set_idx) {
        case 0: parent->printFloat_SettingValue(parent->m_settings.settings.steps_per_mm[idx]); break;
        case 1: parent->printFloat_SettingValue(parent->m_settings.settings.max_rate[idx]); break;
        case 2: parent->printFloat_SettingValue(parent->m_settings.settings.acceleration[idx]/(60*60)); break;
        case 3: parent->printFloat_SettingValue(-parent->m_settings.settings.max_travel[idx]); break;
      }
      parent->printString(" (");
      switch (idx) {
        case X_AXIS: parent->printString("x"); break;
        case Y_AXIS: parent->printString("y"); break;
        case Z_AXIS: parent->printString("z"); break;
      }
      switch (set_idx) {
        case 0: parent->printString(", step/mm"); break;
        case 1: parent->printString(" max rate, mm/min"); break;
        case 2: parent->printString(" accel, mm/sec^2"); break;
        case 3: parent->printString(" max travel, mm"); break;
      }      
      parent->printString(")\r\n");
    }
    val += AXIS_SETTINGS_INCREMENT;
  }  
}


// Prints current probe parameters. Upon a probe command, these parameters are updated upon a
// successful probe or upon a failed probe with the G38.3 without errors command (if supported). 
// These values are retained until Grbl is power-cycled, whereby they will be re-zeroed.
void Grbl::Report::probe_parameters()
{
  uint8_t i;
  float print_position[N_AXIS];
 
  // Report in terms of machine position.
  parent->printString("[PRB:"); 
  for (i=0; i< N_AXIS; i++) {
    print_position[i] = parent->m_system.sys.probe_position[i]/parent->m_settings.settings.steps_per_mm[i];
    parent->printFloat_CoordValue(print_position[i]);
    if (i < (N_AXIS-1)) { parent->printString(","); }
  }  
  parent->printString("]\r\n");
}


// Prints Grbl NGC parameters (coordinate offsets, probing)
void Grbl::Report::ngc_parameters()
{
  float coord_data[N_AXIS];
  uint8_t coord_select, i;
  for (coord_select = 0; coord_select <= SETTING_INDEX_NCOORD; coord_select++) { 
    if (!(parent->m_settings.read_coord_data(coord_select,coord_data))) { 
      status_message(STATUS_SETTING_READ_FAIL); 
      return;
    } 
    parent->printString("[G");
    switch (coord_select) {
      case 6: parent->printString("28"); break;
      case 7: parent->printString("30"); break;
      default: parent->print_uint8_base10(coord_select+54); break; // G54-G59
    }  
    parent->printString(":");         
    for (i=0; i<N_AXIS; i++) {
      parent->printFloat_CoordValue(coord_data[i]);
      if (i < (N_AXIS-1)) { parent->printString(","); }
      else { parent->printString("]\r\n"); }
    } 
  }
  parent->printString("[G92:"); // Print G92,G92.1 which are not persistent in memory
  for (i=0; i<N_AXIS; i++) {
    parent->printFloat_CoordValue(parent->m_gc.gc_state.coord_offset[i]);
    if (i < (N_AXIS-1)) { parent->printString(","); }
    else { parent->printString("]\r\n"); }
  } 
  parent->printString("[TLO:"); // Print tool length offset value
  parent->printFloat_CoordValue(parent->m_gc.gc_state.tool_length_offset);
  parent->printString("]\r\n");
  probe_parameters(); // Print probe parameters. Not persistent in memory.
}


// Print current gcode parser mode state
void Grbl::Report::gcode_modes()
{
  switch (parent->m_gc.gc_state.modal.motion) {
    case MOTION_MODE_SEEK : parent->printString("[G0"); break;
    case MOTION_MODE_LINEAR : parent->printString("[G1"); break;
    case MOTION_MODE_CW_ARC : parent->printString("[G2"); break;
    case MOTION_MODE_CCW_ARC : parent->printString("[G3"); break;
    case MOTION_MODE_NONE : parent->printString("[G80"); break;
  }

  parent->printString(" G");
  parent->print_uint8_base10(parent->m_gc.gc_state.modal.coord_select+54);
  
  switch (parent->m_gc.gc_state.modal.plane_select) {
    case PLANE_SELECT_XY : parent->printString(" G17"); break;
    case PLANE_SELECT_ZX : parent->printString(" G18"); break;
    case PLANE_SELECT_YZ : parent->printString(" G19"); break;
  }
  
  if (parent->m_gc.gc_state.modal.units == UNITS_MODE_MM) { parent->printString(" G21"); }
  else { parent->printString(" G20"); }
  
  if (parent->m_gc.gc_state.modal.distance == DISTANCE_MODE_ABSOLUTE) { parent->printString(" G90"); }
  else { parent->printString(" G91"); }
  
  if (parent->m_gc.gc_state.modal.feed_rate == FEED_RATE_MODE_INVERSE_TIME) { parent->printString(" G93"); }
  else { parent->printString(" G94"); }
    
  switch (parent->m_gc.gc_state.modal.program_flow) {
    case PROGRAM_FLOW_RUNNING : parent->printString(" M0"); break;
    case PROGRAM_FLOW_PAUSED : parent->printString(" M1"); break;
    case PROGRAM_FLOW_COMPLETED : parent->printString(" M2"); break;
  }

  switch (parent->m_gc.gc_state.modal.spindle) {
    case SPINDLE_ENABLE_CW : parent->printString(" M3"); break;
    case SPINDLE_ENABLE_CCW : parent->printString(" M4"); break;
    case SPINDLE_DISABLE : parent->printString(" M5"); break;
  }
  
  switch (parent->m_gc.gc_state.modal.coolant) {
    case COOLANT_DISABLE : parent->printString(" M9"); break;
    case COOLANT_FLOOD_ENABLE : parent->printString(" M8"); break;
    #ifdef ENABLE_M7
      case COOLANT_MIST_ENABLE : parent->printString(" M7"); break;
    #endif
  }
  
  parent->printString(" T");
  parent->print_uint8_base10(parent->m_gc.gc_state.tool);
  
  parent->printString(" F");
  parent->printFloat_RateValue(parent->m_gc.gc_state.feed_rate);

  parent->printString("]\r\n");
}

// Prints specified startup line
void Grbl::Report::startup_line(uint8_t n, char *line)
{
  parent->printString("$N"); parent->print_uint8_base10(n);
  parent->printString("="); parent->printString(line);
  parent->printString("\r\n");
}


// Prints build info line
void Grbl::Report::build_info(char *line)
{
  parent->printString("[" GRBL_VERSION "." GRBL_VERSION_BUILD ":");
  parent->printString(line);
  parent->printString("]\r\n");
}


 // Prints real-time data. This function grabs a real-time snapshot of the stepper subprogram 
 // and the actual location of the CNC machine. Users may change the following function to their
 // specific needs, but the desired real-time data report must be as short as possible. This is
 // requires as it minimizes the computational overhead and allows grbl to keep running smoothly, 
 // especially during g-code programs with fast, short line segments and high frequency reports (5-20Hz).
void Grbl::Report::realtime_status()
{
  // **Under construction** Bare-bones status report. Provides real-time machine position relative to 
  // the system power on location (0,0,0) and work coordinate position (G54 and G92 applied). Eventually
  // to be added are distance to go on block, processed block id, and feed rate. Also a settings bitmask
  // for a user to select the desired real-time data.
  uint8_t i;
  int32_t current_position[N_AXIS]; // Copy current state of the system position variable
  memcpy(current_position,parent->m_system.sys.position,sizeof(parent->m_system.sys.position));
  float print_position[N_AXIS];
 
  // Report current machine state
  switch (parent->m_system.sys.state) {
    case STATE_IDLE: parent->printString("<Idle"); break;
    case STATE_QUEUED: parent->printString("<Queue"); break;
    case STATE_CYCLE: parent->printString("<Run"); break;
    case STATE_HOLD: parent->printString("<Hold"); break;
    case STATE_HOMING: parent->printString("<Home"); break;
    case STATE_ALARM: parent->printString("<Alarm"); break;
    case STATE_CHECK_MODE: parent->printString("<Check"); break;
  }
 
  // If reporting a position, convert the current step count (current_position) to millimeters.
  if (bit_istrue(parent->m_settings.settings.status_report_mask,(BITFLAG_RT_STATUS_MACHINE_POSITION | BITFLAG_RT_STATUS_WORK_POSITION))) {
    for (i=0; i< N_AXIS; i++) { print_position[i] = current_position[i]/parent->m_settings.settings.steps_per_mm[i]; }
  }
  
  // Report machine position
  if (bit_istrue(parent->m_settings.settings.status_report_mask,BITFLAG_RT_STATUS_MACHINE_POSITION)) {
    parent->printString(",MPos:"); 
//     print_position[X_AXIS] = 0.5*current_position[X_AXIS]/settings.steps_per_mm[X_AXIS]; 
//     print_position[Z_AXIS] = 0.5*current_position[Y_AXIS]/settings.steps_per_mm[Y_AXIS]; 
//     print_position[Y_AXIS] = print_position[X_AXIS]-print_position[Z_AXIS]);
//     print_position[X_AXIS] -= print_position[Z_AXIS];    
//     print_position[Z_AXIS] = current_position[Z_AXIS]/settings.steps_per_mm[Z_AXIS];     
    for (i=0; i< N_AXIS; i++) {
      parent->printFloat_CoordValue(print_position[i]);
      if (i < (N_AXIS-1)) { parent->printString(","); }
    }
  }
  
  // Report work position
  if (bit_istrue(parent->m_settings.settings.status_report_mask,BITFLAG_RT_STATUS_WORK_POSITION)) {
    parent->printString(",WPos:"); 
    for (i=0; i< N_AXIS; i++) {
      // Apply work coordinate offsets and tool length offset to current position.
      print_position[i] -= parent->m_gc.gc_state.coord_system[i]+parent->m_gc.gc_state.coord_offset[i];
      if (i == TOOL_LENGTH_OFFSET_AXIS) { print_position[i] -= parent->m_gc.gc_state.tool_length_offset; }    
      parent->printFloat_CoordValue(print_position[i]);
      if (i < (N_AXIS-1)) { parent->printString(","); }
    }
  }
        
  // Returns the number of active blocks are in the planner buffer.
  if (bit_istrue(parent->m_settings.settings.status_report_mask,BITFLAG_RT_STATUS_PLANNER_BUFFER)) {
    parent->printString(",Buf:");
    parent->print_uint8_base10(parent->m_plan.get_block_buffer_count());
  }

  // Report serial read buffer status
  if (bit_istrue(parent->m_settings.settings.status_report_mask,BITFLAG_RT_STATUS_SERIAL_RX)) {
    parent->printString(",RX:");
    //parent->print_uint8_base10(serial_get_rx_buffer_count());
	parent->print_uint8_base10(parent->m_serial->isrx());
  }
    
  #ifdef USE_LINE_NUMBERS
    // Report current line number
    parent->printString(",Ln:"); 
    int32_t ln=0;
    plan_block_t * pb = parent->m_plan.get_current_block();
    if(pb != NULL) {
      ln = pb->line_number;
    } 
    parent->printInteger(ln);
  #endif
    
  #ifdef REPORT_REALTIME_RATE
    // Report realtime rate 
    parent->printString(",F:"); 
    parent->printFloat_RateValue(st_get_realtime_rate());
  #endif    
  
  parent->printString(">\r\n");
}
