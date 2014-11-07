
#ifndef STEPPER_H
#define STEPPER_H

#include <avr32/io.h>
#include <compiler.h>

class Stepper
{
public:
	Stepper();
	bool init(int steps_per_revolution, uint32_t motor_pin1, uint32_t motor_pin2, uint32_t motor_pin3=0, uint32_t motor_pin4=0);
	void setSpeed(unsigned long rpm_speed);
	void step(int num_steps);
	
private:
	int m_steps_per_revolution;
	uint32_t m_pin1;
	uint32_t m_pin2;
	uint32_t m_pin3;
	uint32_t m_pin4;
	unsigned long m_stepdelay;
	int m_position; // 0 to 3
};

#endif  // STEPPER_H
