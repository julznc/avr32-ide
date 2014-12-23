/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
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
  This file is based on work from Grbl v0.8, distributed under the
  terms of the MIT-license. See COPYING for more details.
    Copyright (c) 2009-2011 Simen Svale Skogsrud
    Copyright (c) 2011-2012 Sungeun K. Jeon
*/

#include "Grbl.h"
#include <math.h>
#include <bsp_clock.h>

#define TICKS_PER_MICROSECOND (TARGET_MCUCLK_FREQ_HZ/1000000)

// Some useful constants.
#define DT_SEGMENT (1.0/(ACCELERATION_TICKS_PER_SECOND*60.0)) // min/segment
#define REQ_MM_INCREMENT_SCALAR 1.25
#define RAMP_ACCEL 0
#define RAMP_CRUISE 1
#define RAMP_DECEL 2

// Define Adaptive Multi-Axis Step-Smoothing(AMASS) levels and cutoff frequencies. The highest level
// frequency bin starts at 0Hz and ends at its cutoff frequency. The next lower level frequency bin
// starts at the next higher cutoff frequency, and so on. The cutoff frequencies for each level must
// be considered carefully against how much it over-drives the stepper ISR, the accuracy of the 16-bit
// timer, and the CPU overhead. Level 0 (no AMASS, normal operation) frequency bin starts at the
// Level 1 cutoff frequency and up to as fast as the CPU allows (over 30kHz in limited testing).
// NOTE: AMASS cutoff frequency multiplied by ISR overdrive factor must not exceed maximum step frequency.
// NOTE: Current settings are set to overdrive the ISR to no more than 16kHz, balancing CPU overhead
// and timer accuracy.  Do not alter these settings unless you know what you are doing.
#define MAX_AMASS_LEVEL 3
// AMASS_LEVEL0: Normal operation. No AMASS. No upper cutoff frequency. Starts at LEVEL1 cutoff frequency.
#define AMASS_LEVEL1 (TARGET_MCUCLK_FREQ_HZ/8000) // Over-drives ISR (x2). Defined as F_CPU/(Cutoff frequency in Hz)
#define AMASS_LEVEL2 (TARGET_MCUCLK_FREQ_HZ/4000) // Over-drives ISR (x4)
#define AMASS_LEVEL3 (TARGET_MCUCLK_FREQ_HZ/2000) // Over-drives ISR (x8)


/*    BLOCK VELOCITY PROFILE DEFINITION
          __________________________
         /|                        |\     _________________         ^
        / |                        | \   /|               |\        |
       /  |                        |  \ / |               | \       s
      /   |                        |   |  |               |  \      p
     /    |                        |   |  |               |   \     e
    +-----+------------------------+---+--+---------------+----+    e
    |               BLOCK 1            ^      BLOCK 2          |    d
                                       |
                  time ----->      EXAMPLE: Block 2 entry speed is at max junction velocity

  The planner block buffer is planned assuming constant acceleration velocity profiles and are
  continuously joined at block junctions as shown above. However, the planner only actively computes
  the block entry speeds for an optimal velocity plan, but does not compute the block internal
  velocity profiles. These velocity profiles are computed ad-hoc as they are executed by the
  stepper algorithm and consists of only 7 possible types of profiles: cruise-only, cruise-
  deceleration, acceleration-cruise, acceleration-only, deceleration-only, full-trapezoid, and
  triangle(no cruise).

                                        maximum_speed (< nominal_speed) ->  +
                    +--------+ <- maximum_speed (= nominal_speed)          /|\
                   /          \                                           / | \
 current_speed -> +            \                                         /  |  + <- exit_speed
                  |             + <- exit_speed                         /   |  |
                  +-------------+                     current_speed -> +----+--+
                   time -->  ^  ^                                           ^  ^
                             |  |                                           |  |
                decelerate_after(in mm)                             decelerate_after(in mm)
                    ^           ^                                           ^  ^
                    |           |                                           |  |
                accelerate_until(in mm)                             accelerate_until(in mm)

  The step segment buffer computes the executing block velocity profile and tracks the critical
  parameters for the stepper algorithm to accurately trace the profile. These critical parameters
  are shown and defined in the above illustration.
*/


// Stepper state initialization. Cycle should only start if the st.cycle_start flag is
// enabled. Startup init and limits call this function but shouldn't start the cycle.
void Grbl::Stepper::wake_up()
{
  // Enable stepper drivers.
  // avr32 to do: enable drivers

  if (parent->m_system.sys.state & (STATE_CYCLE | STATE_HOMING)){
    // Initialize stepper output bits
    st.dir_outbits = dir_port_invert_mask;
    st.step_outbits = step_port_invert_mask;

    // Initialize step pulse timing from settings. Here to ensure updating after re-writing.
    // Set step pulse time. Ad hoc computation from oscilloscope. Uses two's complement.
    st.step_pulse_time = -(((parent->m_settings.settings.pulse_microseconds-2)*TICKS_PER_MICROSECOND) >> 3);

    // Enable Stepper Driver Interrupt
    // avr32 to do: enable interrupt
  }
}

// Stepper shutdown
void Grbl::Stepper::go_idle()
{
  // Disable Stepper Driver Interrupt. Allow Stepper Port Reset Interrupt to finish, if active.
  // avr32 to do: disable interrupt
  busy = false;
  
  // Set stepper driver idle state, disabled or enabled, depending on settings and circumstances.
  bool pin_state = false; // Keep enabled.
  if (((parent->m_settings.settings.stepper_idle_lock_time != 0xff) || bit_istrue(parent->m_system.sys.execute,EXEC_ALARM)) && parent->m_system.sys.state != STATE_HOMING) {
    // Force stepper dwell to lock axes for a defined amount of time to ensure the axes come to a complete
    // stop and not drift from residual inertial forces at the end of the last movement.
    delay_ms(parent->m_settings.settings.stepper_idle_lock_time);
    pin_state = true; // Override. Disable steppers.
  }
  // avr32 to do: disable stepper drivers
}




// Generates the step and direction port invert masks used in the Stepper Interrupt Driver.
void Grbl::Stepper::generate_step_dir_invert_masks()
{  
  uint8_t idx;
  step_port_invert_mask = 0;
  dir_port_invert_mask = 0;
  for (idx=0; idx<N_AXIS; idx++) {
    if (bit_istrue(parent->m_settings.settings.step_invert_mask,bit(idx))) { step_port_invert_mask |= parent->m_settings.get_step_pin_mask(idx); }
    if (bit_istrue(parent->m_settings.settings.dir_invert_mask,bit(idx))) { dir_port_invert_mask |= parent->m_settings.get_direction_pin_mask(idx); }
  }
}

// Reset and clear stepper subsystem variables
void Grbl::Stepper::reset()
{
  // Initialize stepper driver idle state.
  go_idle();
  
  // Initialize stepper algorithm variables.
  memset(&prep, 0, sizeof(prep));
  memset(&st, 0, sizeof(st));
  st.exec_segment = NULL;
  pl_block = NULL;  // Planner block pointer used by segment buffer
  segment_buffer_tail = 0;
  segment_buffer_head = 0; // empty = tail
  segment_next_head = 1;
  busy = false;
  
  generate_step_dir_invert_masks();
      
  // Initialize step and direction port pins.
  // avr32 to do: step and direction port pins.
}


// Initialize and start the stepper motor subsystem
void Grbl::Stepper::init()
{
  // Configure step and direction interface pins
  // avr32 to do: step and direction interface pins

  // Configure Timer 1: Stepper Driver Interrupt
  // avr32 to do:
  
  // Configure Timer 0: Stepper Port Reset Interrupt
  // avr32 to do:
}


// Called by planner_recalculate() when the executing block is updated by the new plan.
void Grbl::Stepper::update_plan_block_parameters()
{
  if (pl_block != NULL) { // Ignore if at start of a new block.
    prep.flag_partial_block = true;
    pl_block->entry_speed_sqr = prep.current_speed*prep.current_speed; // Update entry speed.
    pl_block = NULL; // Flag st_prep_segment() to load new velocity profile.
  }
}


/* Prepares step segment buffer. Continuously called from main program. 

   The segment buffer is an intermediary buffer interface between the execution of steps
   by the stepper algorithm and the velocity profiles generated by the planner. The stepper
   algorithm only executes steps within the segment buffer and is filled by the main program
   when steps are "checked-out" from the first block in the planner buffer. This keeps the
   step execution and planning optimization processes atomic and protected from each other.
   The number of steps "checked-out" from the planner buffer and the number of segments in
   the segment buffer is sized and computed such that no operation in the main program takes
   longer than the time it takes the stepper algorithm to empty it before refilling it. 
   Currently, the segment buffer conservatively holds roughly up to 40-50 msec of steps.
   NOTE: Computation units are in steps, millimeters, and minutes.
*/
void Grbl::Stepper::prep_buffer()
{
  while (segment_buffer_tail != segment_next_head) { // Check if we need to fill the buffer.

    // Determine if we need to load a new planner block or if the block has been replanned. 
    if (pl_block == NULL) {
      pl_block = parent->m_plan.get_current_block(); // Query planner for a queued block
      if (pl_block == NULL) { return; } // No planner blocks. Exit.
                      
      // Check if the segment buffer completed the last planner block. If so, load the Bresenham
      // data for the block. If not, we are still mid-block and the velocity profile was updated. 
      if (prep.flag_partial_block) {
        prep.flag_partial_block = false; // Reset flag
      } else {
        // Increment stepper common data index to store new planner block data. 
        if ( ++prep.st_block_index == (SEGMENT_BUFFER_SIZE-1) ) { prep.st_block_index = 0; }
        
        // Prepare and copy Bresenham algorithm segment data from the new planner block, so that
        // when the segment buffer completes the planner block, it may be discarded when the 
        // segment buffer finishes the prepped block, but the stepper ISR is still executing it. 
        st_prep_block = &st_block_buffer[prep.st_block_index];
        st_prep_block->direction_bits = pl_block->direction_bits;
        #ifndef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING
          st_prep_block->steps[X_AXIS] = pl_block->steps[X_AXIS];
          st_prep_block->steps[Y_AXIS] = pl_block->steps[Y_AXIS];
          st_prep_block->steps[Z_AXIS] = pl_block->steps[Z_AXIS];
          st_prep_block->step_event_count = pl_block->step_event_count;
        #else
          // With AMASS enabled, simply bit-shift multiply all Bresenham data by the max AMASS 
          // level, such that we never divide beyond the original data anywhere in the algorithm.
          // If the original data is divided, we can lose a step from integer roundoff.
          st_prep_block->steps[X_AXIS] = pl_block->steps[X_AXIS] << MAX_AMASS_LEVEL;
          st_prep_block->steps[Y_AXIS] = pl_block->steps[Y_AXIS] << MAX_AMASS_LEVEL;
          st_prep_block->steps[Z_AXIS] = pl_block->steps[Z_AXIS] << MAX_AMASS_LEVEL;
          st_prep_block->step_event_count = pl_block->step_event_count << MAX_AMASS_LEVEL;
        #endif
        
        // Initialize segment buffer data for generating the segments.
        prep.steps_remaining = pl_block->step_event_count;
        prep.step_per_mm = prep.steps_remaining/pl_block->millimeters;
        prep.req_mm_increment = REQ_MM_INCREMENT_SCALAR/prep.step_per_mm;
        
        prep.dt_remainder = 0.0; // Reset for new planner block

        if (parent->m_system.sys.state == STATE_HOLD) {
          // Override planner block entry speed and enforce deceleration during feed hold.
          prep.current_speed = prep.exit_speed; 
          pl_block->entry_speed_sqr = prep.exit_speed*prep.exit_speed; 
        }
        else { prep.current_speed = sqrt(pl_block->entry_speed_sqr); }
      }
     
      /* --------------------------------------------------------------------------------- 
         Compute the velocity profile of a new planner block based on its entry and exit
         speeds, or recompute the profile of a partially-completed planner block if the 
         planner has updated it. For a commanded forced-deceleration, such as from a feed 
         hold, override the planner velocities and decelerate to the target exit speed.
      */
      prep.mm_complete = 0.0; // Default velocity profile complete at 0.0mm from end of block.
      float inv_2_accel = 0.5/pl_block->acceleration;
      if (parent->m_system.sys.state == STATE_HOLD) { // [Forced Deceleration to Zero Velocity]
        // Compute velocity profile parameters for a feed hold in-progress. This profile overrides
        // the planner block profile, enforcing a deceleration to zero speed.
        prep.ramp_type = RAMP_DECEL;
        // Compute decelerate distance relative to end of block.
        float decel_dist = pl_block->millimeters - inv_2_accel*pl_block->entry_speed_sqr;
        if (decel_dist < 0.0) {
          // Deceleration through entire planner block. End of feed hold is not in this block.
          prep.exit_speed = sqrt(pl_block->entry_speed_sqr-2*pl_block->acceleration*pl_block->millimeters);
        } else {
          prep.mm_complete = decel_dist; // End of feed hold.
          prep.exit_speed = 0.0;
        }
      } else { // [Normal Operation]
        // Compute or recompute velocity profile parameters of the prepped planner block.
        prep.ramp_type = RAMP_ACCEL; // Initialize as acceleration ramp.
        prep.accelerate_until = pl_block->millimeters; 
        prep.exit_speed = parent->m_plan.get_exec_block_exit_speed();   
        float exit_speed_sqr = prep.exit_speed*prep.exit_speed;
        float intersect_distance =
                0.5*(pl_block->millimeters+inv_2_accel*(pl_block->entry_speed_sqr-exit_speed_sqr));
        if (intersect_distance > 0.0) {
          if (intersect_distance < pl_block->millimeters) { // Either trapezoid or triangle types
            // NOTE: For acceleration-cruise and cruise-only types, following calculation will be 0.0.
            prep.decelerate_after = inv_2_accel*(pl_block->nominal_speed_sqr-exit_speed_sqr);
            if (prep.decelerate_after < intersect_distance) { // Trapezoid type
              prep.maximum_speed = sqrt(pl_block->nominal_speed_sqr);
              if (pl_block->entry_speed_sqr == pl_block->nominal_speed_sqr) { 
                // Cruise-deceleration or cruise-only type.
                prep.ramp_type = RAMP_CRUISE;
              } else {
                // Full-trapezoid or acceleration-cruise types
                prep.accelerate_until -= inv_2_accel*(pl_block->nominal_speed_sqr-pl_block->entry_speed_sqr); 
              }
            } else { // Triangle type
              prep.accelerate_until = intersect_distance;
              prep.decelerate_after = intersect_distance;
              prep.maximum_speed = sqrt(2.0*pl_block->acceleration*intersect_distance+exit_speed_sqr);
            }          
          } else { // Deceleration-only type
            prep.ramp_type = RAMP_DECEL;
            // prep.decelerate_after = pl_block->millimeters;
            prep.maximum_speed = prep.current_speed;
          }
        } else { // Acceleration-only type
          prep.accelerate_until = 0.0;
          // prep.decelerate_after = 0.0;
          prep.maximum_speed = prep.exit_speed;
        }
      }  
    }

    // Initialize new segment
    segment_t *prep_segment = &segment_buffer[segment_buffer_head];

    // Set new segment to point to the current segment data block.
    prep_segment->st_block_index = prep.st_block_index;

    /*------------------------------------------------------------------------------------
        Compute the average velocity of this new segment by determining the total distance
      traveled over the segment time DT_SEGMENT. The following code first attempts to create 
      a full segment based on the current ramp conditions. If the segment time is incomplete 
      when terminating at a ramp state change, the code will continue to loop through the
      progressing ramp states to fill the remaining segment execution time. However, if 
      an incomplete segment terminates at the end of the velocity profile, the segment is 
      considered completed despite having a truncated execution time less than DT_SEGMENT.
        The velocity profile is always assumed to progress through the ramp sequence:
      acceleration ramp, cruising state, and deceleration ramp. Each ramp's travel distance
      may range from zero to the length of the block. Velocity profiles can end either at 
      the end of planner block (typical) or mid-block at the end of a forced deceleration, 
      such as from a feed hold.
    */
    float dt_max = DT_SEGMENT; // Maximum segment time
    float dt = 0.0; // Initialize segment time
    float time_var = dt_max; // Time worker variable
    float mm_var; // mm-Distance worker variable
    float speed_var; // Speed worker variable   
    float mm_remaining = pl_block->millimeters; // New segment distance from end of block.
    float minimum_mm = mm_remaining-prep.req_mm_increment; // Guarantee at least one step.
    if (minimum_mm < 0.0) { minimum_mm = 0.0; }

    do {
      switch (prep.ramp_type) {
        case RAMP_ACCEL: 
          // NOTE: Acceleration ramp only computes during first do-while loop.
          speed_var = pl_block->acceleration*time_var;
          mm_remaining -= time_var*(prep.current_speed + 0.5*speed_var);
          if (mm_remaining < prep.accelerate_until) { // End of acceleration ramp.
            // Acceleration-cruise, acceleration-deceleration ramp junction, or end of block.
            mm_remaining = prep.accelerate_until; // NOTE: 0.0 at EOB
            time_var = 2.0*(pl_block->millimeters-mm_remaining)/(prep.current_speed+prep.maximum_speed);
            if (mm_remaining == prep.decelerate_after) { prep.ramp_type = RAMP_DECEL; }
            else { prep.ramp_type = RAMP_CRUISE; }
            prep.current_speed = prep.maximum_speed;
          } else { // Acceleration only. 
            prep.current_speed += speed_var;
          }
          break;
        case RAMP_CRUISE: 
          // NOTE: mm_var used to retain the last mm_remaining for incomplete segment time_var calculations.
          // NOTE: If maximum_speed*time_var value is too low, round-off can cause mm_var to not change. To 
          //   prevent this, simply enforce a minimum speed threshold in the planner.
          mm_var = mm_remaining - prep.maximum_speed*time_var;
          if (mm_var < prep.decelerate_after) { // End of cruise. 
            // Cruise-deceleration junction or end of block.
            time_var = (mm_remaining - prep.decelerate_after)/prep.maximum_speed;
            mm_remaining = prep.decelerate_after; // NOTE: 0.0 at EOB
            prep.ramp_type = RAMP_DECEL;
          } else { // Cruising only.         
            mm_remaining = mm_var; 
          } 
          break;
        default: // case RAMP_DECEL:
          // NOTE: mm_var used as a misc worker variable to prevent errors when near zero speed.
          speed_var = pl_block->acceleration*time_var; // Used as delta speed (mm/min)
          if (prep.current_speed > speed_var) { // Check if at or below zero speed.
            // Compute distance from end of segment to end of block.
            mm_var = mm_remaining - time_var*(prep.current_speed - 0.5*speed_var); // (mm)
            if (mm_var > prep.mm_complete) { // Deceleration only.
              mm_remaining = mm_var;
              prep.current_speed -= speed_var;
              break; // Segment complete. Exit switch-case statement. Continue do-while loop.
            }
          } // End of block or end of forced-deceleration.
          time_var = 2.0*(mm_remaining-prep.mm_complete)/(prep.current_speed+prep.exit_speed);
          mm_remaining = prep.mm_complete; 
      }
      dt += time_var; // Add computed ramp time to total segment time.
      if (dt < dt_max) { time_var = dt_max - dt; } // **Incomplete** At ramp junction.
      else {
        if (mm_remaining > minimum_mm) { // Check for very slow segments with zero steps.
          // Increase segment time to ensure at least one step in segment. Override and loop
          // through distance calculations until minimum_mm or mm_complete.
          dt_max += DT_SEGMENT;
          time_var = dt_max - dt;
        } else { 
          break; // **Complete** Exit loop. Segment execution time maxed.
        }
      }
    } while (mm_remaining > prep.mm_complete); // **Complete** Exit loop. Profile complete.

   
    /* -----------------------------------------------------------------------------------
       Compute segment step rate, steps to execute, and apply necessary rate corrections.
       NOTE: Steps are computed by direct scalar conversion of the millimeter distance 
       remaining in the block, rather than incrementally tallying the steps executed per
       segment. This helps in removing floating point round-off issues of several additions. 
       However, since floats have only 7.2 significant digits, long moves with extremely 
       high step counts can exceed the precision of floats, which can lead to lost steps.
       Fortunately, this scenario is highly unlikely and unrealistic in CNC machines
       supported by Grbl (i.e. exceeding 10 meters axis travel at 200 step/mm).
    */
    float steps_remaining = prep.step_per_mm*mm_remaining; // Convert mm_remaining to steps
    float n_steps_remaining = ceil(steps_remaining); // Round-up current steps remaining
    float last_n_steps_remaining = ceil(prep.steps_remaining); // Round-up last steps remaining
    prep_segment->n_step = last_n_steps_remaining-n_steps_remaining; // Compute number of steps to execute.
    
    // Bail if we are at the end of a feed hold and don't have a step to execute.
    if (prep_segment->n_step == 0) {
      if (parent->m_system.sys.state == STATE_HOLD) {

        // Less than one step to decelerate to zero speed, but already very close. AMASS 
        // requires full steps to execute. So, just bail.
        prep.current_speed = 0.0;
        prep.dt_remainder = 0.0;
        prep.steps_remaining = n_steps_remaining;
        pl_block->millimeters = prep.steps_remaining/prep.step_per_mm; // Update with full steps.
        parent->m_plan.cycle_reinitialize();         
        parent->m_system.sys.state = STATE_QUEUED; 
        return; // Segment not generated, but current step data still retained.
      }
    }

    // Compute segment step rate. Since steps are integers and mm distances traveled are not,
    // the end of every segment can have a partial step of varying magnitudes that are not 
    // executed, because the stepper ISR requires whole steps due to the AMASS algorithm. To
    // compensate, we track the time to execute the previous segment's partial step and simply
    // apply it with the partial step distance to the current segment, so that it minutely
    // adjusts the whole segment rate to keep step output exact. These rate adjustments are 
    // typically very small and do not adversely effect performance, but ensures that Grbl
    // outputs the exact acceleration and velocity profiles as computed by the planner.
    dt += prep.dt_remainder; // Apply previous segment partial step execute time
    float inv_rate = dt/(last_n_steps_remaining - steps_remaining); // Compute adjusted step rate inverse
    prep.dt_remainder = (n_steps_remaining - steps_remaining)*inv_rate; // Update segment partial step time

    // Compute CPU cycles per step for the prepped segment.
    uint32_t cycles = ceil( (TICKS_PER_MICROSECOND*1000000*60)*inv_rate ); // (cycles/step)    

    #ifdef ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING        
      // Compute step timing and multi-axis smoothing level.
      // NOTE: AMASS overdrives the timer with each level, so only one prescalar is required.
      if (cycles < AMASS_LEVEL1) { prep_segment->amass_level = 0; }
      else {
        if (cycles < AMASS_LEVEL2) { prep_segment->amass_level = 1; }
        else if (cycles < AMASS_LEVEL3) { prep_segment->amass_level = 2; }
        else { prep_segment->amass_level = 3; }    
        cycles >>= prep_segment->amass_level; 
        prep_segment->n_step <<= prep_segment->amass_level;
      }
      if (cycles < (1UL << 16)) { prep_segment->cycles_per_tick = cycles; } // < 65536 (4.1ms @ 16MHz)
      else { prep_segment->cycles_per_tick = 0xffff; } // Just set the slowest speed possible.
    #else 
      // Compute step timing and timer prescalar for normal step generation.
      if (cycles < (1UL << 16)) { // < 65536  (4.1ms @ 16MHz)
        prep_segment->prescaler = 1; // prescaler: 0
        prep_segment->cycles_per_tick = cycles;
      } else if (cycles < (1UL << 19)) { // < 524288 (32.8ms@16MHz)
        prep_segment->prescaler = 2; // prescaler: 8
        prep_segment->cycles_per_tick = cycles >> 3;
      } else { 
        prep_segment->prescaler = 3; // prescaler: 64
        if (cycles < (1UL << 22)) { // < 4194304 (262ms@16MHz)
          prep_segment->cycles_per_tick =  cycles >> 6;
        } else { // Just set the slowest speed possible. (Around 4 step/sec.)
          prep_segment->cycles_per_tick = 0xffff;
        }
      }
    #endif

    // Segment complete! Increment segment buffer indices.
    segment_buffer_head = segment_next_head;
    if ( ++segment_next_head == SEGMENT_BUFFER_SIZE ) { segment_next_head = 0; }

    // Setup initial conditions for next segment.
    if (mm_remaining > prep.mm_complete) { 
      // Normal operation. Block incomplete. Distance remaining in block to be executed.
      pl_block->millimeters = mm_remaining;      
      prep.steps_remaining = steps_remaining;  
    } else { 
      // End of planner block or forced-termination. No more distance to be executed.
      if (mm_remaining > 0.0) { // At end of forced-termination.
        // Reset prep parameters for resuming and then bail.
        // NOTE: Currently only feed holds qualify for this scenario. May change with overrides.       
        prep.current_speed = 0.0;
        prep.dt_remainder = 0.0;
        prep.steps_remaining = ceil(steps_remaining);
        pl_block->millimeters = prep.steps_remaining/prep.step_per_mm; // Update with full steps.
        parent->m_plan.cycle_reinitialize(); 
        parent->m_system.sys.state = STATE_QUEUED; // End cycle.        

        return; // Bail!
// TODO: Try to move QUEUED setting into cycle re-initialize.

      } else { // End of planner block
        // The planner block is complete. All steps are set to be executed in the segment buffer.
        pl_block = NULL;
        parent->m_plan.discard_current_block();
      }
    }

  }
}


// Called by runtime status reporting to fetch the current speed being executed. This value
// however is not exactly the current speed, but the speed computed in the last step segment
// in the segment buffer. It will always be behind by up to the number of segment blocks (-1)
// divided by the ACCELERATION TICKS PER SECOND in seconds. 
#ifdef REPORT_REALTIME_RATE
  float Grbl::Stepper::get_realtime_rate()
  {
     if (parent->m_system.sys.state & (STATE_CYCLE | STATE_HOMING | STATE_HOLD)){
       return prep.current_speed;
     }
    return 0.0f;
  }
#endif
