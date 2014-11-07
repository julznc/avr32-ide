
#include "Grbl.h"

Grbl::Grbl(HardwareUart *serial)
  : m_serial(serial), m_settings(this),
    m_report(this), m_stepper(this),
    m_system(this), m_gc(this),
    m_spindle(this), m_coolant(this),
    m_limits(this), m_probe(this),
    m_plan(this), m_protocol(this),
    m_mc(this)
{
  
}

void Grbl::init()
{
  // Initialize system upon power-up.
  m_serial->begin(GRBL_BAUD_RATE);   // Setup serial baud rate and interrupts
  m_settings.init(); // Load grbl settings from EEPROM
  m_stepper.init();  // Configure stepper pins and interrupt timers
  m_system.init();   // Configure pinout pins and pin-change interrupt
  
  memset(&m_system.sys, 0, sizeof(m_system.sys));  // Clear all system variables
  m_system.sys.abort = true;   // Set abort to complete initialization
  
  // Check for power-up and set system alarm if homing is enabled to force homing cycle
  // by setting Grbl's alarm state. Alarm locks out all g-code commands, including the
  // startup scripts, but allows access to settings and internal commands. Only a homing
  // cycle '$H' or kill alarm locks '$X' will disable the alarm.
  // NOTE: The startup script will run after successful completion of the homing cycle, but
  // not after disabling the alarm locks. Prevents motion startup blocks from crashing into
  // things uncontrollably. Very bad.
  #ifdef HOMING_INIT_LOCK
    if (bit_istrue(m_settings.settings.flags,BITFLAG_HOMING_ENABLE)) { m_system.sys.state = STATE_ALARM; }
  #endif
}

void Grbl::reset()
{
  // Reset Grbl primary systems.
  m_serial->flushRx(); // Clear serial read buffer
  m_serial->flushTx();
  m_gc.init(); // Set g-code parser to default state
  m_spindle.init();
  m_coolant.init();
  m_limits.init();
  m_probe.init();
  m_plan.reset(); // Clear block buffer and planner variables
  m_stepper.reset(); // Clear stepper subsystem variables.
  
  // Sync cleared gcode and planner positions to current system position.
  m_plan.sync_position();
  m_gc.sync_position();
  
  // Reset system variables.
  m_system.sys.abort = false;
  m_system.sys.execute = 0;
  if (bit_istrue(m_settings.settings.flags,BITFLAG_AUTO_START)) { m_system.sys.auto_start = true; }
  else { m_system.sys.auto_start = false; }
}

void Grbl::loop()
{
  // Start Grbl main loop. Processes program inputs and executes them.
  m_protocol.main_loop();
}
