#ifndef GRBL_H
#define GRBL_H

#include <avr32/io.h>
#include <compiler.h>
#include <string.h>
#include <stdarg.h>
#include <bsp_uart.h>

#include "utils.h"
#include "config.h"
#include "defaults.h"
#include "report.h"
#include "settings.h"
#include "system.h"
#include "gcode.h"
#include "planner.h"
#include "stepper.h"

class Grbl
{
public:
	class GrblChild
	{
	protected: Grbl *parent;
	public: GrblChild(Grbl *grbl) : parent(grbl) { }
	};

	class Settings : GrblChild
	{
	public:
		Settings(Grbl *grbl) : GrblChild(grbl) { }
		settings_t settings;

		// Initialize the configuration subsystem (load settings from EEPROM)
		void init();

		// Helper functions to clear and restore EEPROM defaults
		void restore_global_settings();
		void clear_parameters();
		void clear_startup_lines();
		void clear_build_info();

		// A helper method to set new settings from command line
		uint8_t store_global_setting(uint8_t parameter, float value);

		// Stores the protocol line variable as a startup line in EEPROM
		void store_startup_line(uint8_t n, char *line);

		// Reads an EEPROM startup line to the protocol line variable
		uint8_t read_startup_line(uint8_t n, char *line);

		// Stores build info user-defined string
		void store_build_info(char *line);

		// Reads build info user-defined string
		uint8_t read_build_info(char *line);

		// Writes selected coordinate data to EEPROM
		void write_coord_data(uint8_t coord_select, float *coord_data);

		// Method to store Grbl global settings struct and version number into EEPROM
		void write_global_settings();

		// Reads selected coordinate data from EEPROM
		uint8_t read_coord_data(uint8_t coord_select, float *coord_data);

		// Returns the step pin mask according to Grbl's internal axis numbering
		uint8_t get_step_pin_mask(uint8_t i);

		// Returns the direction pin mask according to Grbl's internal axis numbering
		uint8_t get_direction_pin_mask(uint8_t i);

		// Returns the limit pin mask according to Grbl's internal axis numbering
		uint8_t get_limit_pin_mask(uint8_t i);

		uint8_t read_global_settings();
	};

	class Report : public GrblChild
	{
	public:
		Report(Grbl *grbl) : GrblChild(grbl) { }
		// Prints system status messages.
		void status_message(uint8_t status_code);

		// Prints system alarm messages.
		void alarm_message(int8_t alarm_code);

		// Prints miscellaneous feedback messages.
		void feedback_message(uint8_t message_code);

		// Prints welcome message
		void init_message();

		// Prints Grbl help and current global settings
		void grbl_help();

		// Prints Grbl global settings
		void grbl_settings();

		// Prints realtime status report
		void realtime_status();

		// Prints recorded probe position
		void probe_parameters();

		// Prints Grbl NGC parameters (coordinate offsets, probe)
		void ngc_parameters();

		// Prints current g-code parser mode state
		void gcode_modes();

		// Prints startup line
		void startup_line(uint8_t n, char *line);

		// Prints build info and user info
		void build_info(char *line);
	};

	class Stepper : public GrblChild
	{
	public:
		Stepper(Grbl *grbl) : GrblChild(grbl) { }
		// Initialize and setup the stepper motor subsystem
		void init();

		// Enable steppers, but cycle does not start unless called by motion control or runtime command.
		void wake_up();

		// Immediately disables steppers
		void go_idle();

		// Generate the step and direction port invert masks.
		void generate_step_dir_invert_masks();

		// Reset the stepper subsystem variables
		void reset();

		// Reloads step segment buffer. Called continuously by runtime execution system.
		void prep_buffer();

		// Called by planner_recalculate() when the executing block is updated by the new plan.
		void update_plan_block_parameters();

		// Called by runtime status reporting if realtime rate reporting is enabled in config.h.
		#ifdef REPORT_REALTIME_RATE
		float get_realtime_rate();
		#endif

	private:
		st_block_t st_block_buffer[SEGMENT_BUFFER_SIZE-1];
		segment_t segment_buffer[SEGMENT_BUFFER_SIZE];
		stepper_t st;

		// Step segment ring buffer indices
		volatile uint8_t segment_buffer_tail;
		uint8_t segment_buffer_head;
		uint8_t segment_next_head;

		// Step and direction port invert masks.
		uint8_t step_port_invert_mask;
		uint8_t dir_port_invert_mask;

		// Used to avoid ISR nesting of the "Stepper Driver Interrupt". Should never occur though.
		volatile uint8_t busy;

		// Pointers for the step segment being prepped from the planner buffer. Accessed only by the
		// main program. Pointers may be planning segments or planner blocks ahead of what being executed.
		plan_block_t *pl_block;     // Pointer to the planner block being prepped
		st_block_t *st_prep_block;  // Pointer to the stepper block data being prepped

		st_prep_t prep;
	};

	class System : public GrblChild
	{
	public:
		System(Grbl *grbl) : GrblChild(grbl) { }
		system_t sys;

		// Initialize the serial protocol
		void init();

		// Executes an internal system command, defined as a string starting with a '$'
		uint8_t execute_line(char *line);

		// Checks and executes a runtime command at various stop points in main program
		void execute_runtime();

		// Execute the startup script lines stored in EEPROM upon initialization
		void execute_startup(char *line);

	};

	class GCode : public GrblChild
	{
	public:
		GCode(Grbl *grbl) : GrblChild(grbl) { }
		parser_state_t gc_state;
		parser_block_t gc_block;

		// Initialize the parser
		void init();

		// Execute one block of rs275/ngc/g-code
		uint8_t execute_line(char *line);

		// Set g-code parser position. Input in steps.
		void sync_position();

	private:
		uint8_t check_same_position(float *pos_a, float *pos_b);
	};

	class Spindle : public GrblChild
	{
	public:
		Spindle(Grbl *grbl) : GrblChild(grbl) { }
		// Initializes spindle pins and hardware PWM, if enabled.
		void init();

		// Sets spindle direction and spindle rpm via PWM, if enabled.
		void run(uint8_t direction, float rpm);

		// Kills spindle.
		void stop();
	};

	class Coolant : public GrblChild
	{
	public:
		Coolant(Grbl *grbl) : GrblChild(grbl) { }
		void init();
		void stop();
		void run(uint8_t mode);
	};

	class Limits : public GrblChild
	{
	public:
		Limits(Grbl *grbl) : GrblChild(grbl) { }
		// Initialize the limits module
		void init();

		void disable();

		// Perform one portion of the homing cycle based on the input settings.
		void go_home(uint8_t cycle_mask);

		// Check for soft limit violations
		void soft_check(float *target);
	};

	class Probe : public GrblChild
	{
	public:
		// Values that define the probing state machine.
		#define PROBE_OFF     0 // No probing. (Must be zero.)
		#define PROBE_ACTIVE  1 // Actively watching the input pin.
		Probe(Grbl *grbl) : GrblChild(grbl) { }
		// Probe pin initialization routine.
		void init();

		// Returns probe pin state.
		uint8_t get_state();

		// Monitors probe pin state and records the system position when detected. Called by the
		// stepper ISR per ISR tick.
		void state_monitor();
	};

	class Planner : public GrblChild
	{
	public:
		Planner(Grbl *grbl) : GrblChild(grbl) { }
		// Initialize and reset the motion plan subsystem
		void reset();

		// Add a new linear movement to the buffer. target[N_AXIS] is the signed, absolute target position
		// in millimeters. Feed rate specifies the speed of the motion. If feed rate is inverted, the feed
		// rate is taken to mean "frequency" and would complete the operation in 1/feed_rate minutes.
		#ifdef USE_LINE_NUMBERS
		  void buffer_line(float *target, float feed_rate, uint8_t invert_feed_rate, int32_t line_number);
		#else
		  void buffer_line(float *target, float feed_rate, uint8_t invert_feed_rate);
		#endif

		// Called when the current block is no longer needed. Discards the block and makes the memory
		// available for new blocks.
		void discard_current_block();

		// Gets the current block. Returns NULL if buffer empty
		plan_block_t *get_current_block();

		// Called periodically by step segment buffer. Mostly used internally by planner.
		uint8_t next_block_index(uint8_t block_index);

		// Called by step segment buffer when computing executing block velocity profile.
		float get_exec_block_exit_speed();

		// Reset the planner position vector (in steps)
		void sync_position();

		// Reinitialize plan with a partially completed block
		void cycle_reinitialize();

		// Returns the number of active blocks are in the planner buffer.
		uint8_t get_block_buffer_count();

		// Returns the status of the block ring buffer. True, if buffer is full.
		uint8_t check_full_buffer();

	private:
		plan_block_t block_buffer[BLOCK_BUFFER_SIZE];  // A ring buffer for motion instructions
		uint8_t block_buffer_tail;     // Index of the block to process now
		uint8_t block_buffer_head;     // Index of the next block to be pushed
		uint8_t next_buffer_head;      // Index of the next buffer head
		uint8_t block_buffer_planned;  // Index of the optimally planned block

		// Define planner variables
		typedef struct {
		  int32_t position[N_AXIS];          // The planner position of the tool in absolute steps. Kept separate
											 // from g-code position for movements requiring multiple line motions,
											 // i.e. arcs, canned cycles, and backlash compensation.
		  float previous_unit_vec[N_AXIS];   // Unit vector of previous path line segment
		  float previous_nominal_speed_sqr;  // Nominal speed of previous path line segment
		} planner_t;
		planner_t pl;

		// Returns the index of the previous block in the ring buffer
		uint8_t prev_block_index(uint8_t block_index);
		void recalculate();
	};


	class Protocol : public GrblChild
	{
	// Line buffer size from the serial input stream to be executed.
	#ifndef LINE_BUFFER_SIZE
	  #define LINE_BUFFER_SIZE (256+1)
	#endif
	public:
		Protocol(Grbl *grbl) : GrblChild(grbl) { }
		// Starts Grbl main loop. It handles all incoming characters from the serial port and executes
		// them as they complete. It is also responsible for finishing the initialization procedures.
		void main_loop();

		// Checks and executes a runtime command at various stop points in main program
		void execute_runtime();

		// Notify the stepper subsystem to start executing the g-code program in buffer.
		// void cycle_start();

		// Reinitializes the buffer after a feed hold for a resume.
		// void cycle_reinitialize();

		// Initiates a feed hold of the running program
		// void feed_hold();

		// Executes the auto cycle feature, if enabled.
		void auto_cycle_start();

		// Block until all buffered steps are executed
		void buffer_synchronize();

	private:
		char line[LINE_BUFFER_SIZE]; // Line to be executed. Zero-terminated.
		void execute_line(char *line);
	};

	class MotionControl : public GrblChild
	{
	public:
		MotionControl(Grbl *grbl) : GrblChild(grbl) { }
		// Execute linear motion in absolute millimeter coordinates. Feed rate given in millimeters/second
		// unless invert_feed_rate is true. Then the feed_rate means that the motion should be completed in
		// (1 minute)/feed_rate time.
		#ifdef USE_LINE_NUMBERS
		void line(float *target, float feed_rate, uint8_t invert_feed_rate, int32_t line_number);
		#else
		void line(float *target, float feed_rate, uint8_t invert_feed_rate);
		#endif

		// Execute an arc in offset mode format. position == current xyz, target == target xyz,
		// offset == offset from current xyz, axis_XXX defines circle plane in tool space, axis_linear is
		// the direction of helical travel, radius == circle radius, isclockwise boolean. Used
		// for vector transformation direction.
		#ifdef USE_LINE_NUMBERS
		void arc(float *position, float *target, float *offset, float radius, float feed_rate,
		  uint8_t invert_feed_rate, uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, int32_t line_number);
		#else
		void arc(float *position, float *target, float *offset, float radius, float feed_rate,
		  uint8_t invert_feed_rate, uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear);
		#endif

		// Dwell for a specific number of seconds
		void dwell(float seconds);

		// Perform homing cycle to locate machine zero. Requires limit switches.
		void homing_cycle();

		// Perform tool length probe cycle. Requires probe switch.
		#ifdef USE_LINE_NUMBERS
		void probe_cycle(float *target, float feed_rate, uint8_t invert_feed_rate, int32_t line_number);
		#else
		void probe_cycle(float *target, float feed_rate, uint8_t invert_feed_rate);
		#endif

		// Performs system reset. If in motion state, kills all motion and sets system alarm.
		void reset();
	};

public:
	Grbl(HardwareUart *serial);
	void init();
	void reset();
	void loop();

	/* print.h */
	void printString(const char *s);
	void printInteger(long n);
	void print_uint32_base10(uint32_t n);
	void print_uint8_base2(uint8_t n);
	void print_uint8_base10(uint8_t n);
	void printFloat(float n, uint8_t decimal_places);
	void printFloat_CoordValue(float n);
	void printFloat_RateValue(float n);
	void printFloat_SettingValue(float n);
	void printFreeMemory();

private:
	HardwareUart *m_serial;
	Settings m_settings;
	Report m_report;
	Stepper m_stepper;
	System m_system;
	GCode m_gc;
	Spindle m_spindle;
	Coolant m_coolant;
	Limits m_limits;
	Probe m_probe;
	Planner m_plan;
	Protocol m_protocol;
	MotionControl m_mc;
};

#endif // GRBL_H
