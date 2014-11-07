

extern "C"
{
#include <avr32/io.h>
#include <power_clocks_lib.h>
#include <gpio.h>
}

#include <stdint.h>
#include <bsp_clock.h>
#include <bsp_gpio.h>
#include "Stepper.h"

Stepper::Stepper() :
	m_steps_per_revolution(360),
	m_pin1(0), m_pin2(0), m_pin3(0), m_pin4(0),
	m_stepdelay(0), m_position(0)
{ }

bool Stepper::init(int steps_per_revolution, uint32_t motor_pin1, uint32_t motor_pin2, uint32_t motor_pin3, uint32_t motor_pin4)
{
	m_steps_per_revolution = steps_per_revolution;
	m_pin1 = motor_pin1;
	m_pin2 = motor_pin2;
	m_pin3 = motor_pin3;
	m_pin4 = motor_pin4;
	
	if(m_pin1>0) pinMode(m_pin1, OUTPUT);
	if(m_pin2>0) pinMode(m_pin2, OUTPUT);
	if(m_pin3>0) pinMode(m_pin3, OUTPUT);
	if(m_pin4>0) pinMode(m_pin4, OUTPUT);
	
	setSpeed(60);
	m_position = 0;
	
	return true;
}

void Stepper::setSpeed(unsigned long rpm_speed)
{
	m_stepdelay = 60UL * 1000UL / m_steps_per_revolution / rpm_speed;
}

void Stepper::step(int num_steps)
{
	int reverse = 0;
	unsigned long prev_ms = millis();
	
	if(num_steps<0) {
		num_steps = 0-num_steps;
		reverse = 1;
	}
	while(num_steps>0) {
		if( (unsigned long)(millis()-prev_ms) >= m_stepdelay ) {
			prev_ms = millis();
			m_position = reverse? (m_position-1):(m_position+1);
			m_position &= 0x03;
			if(m_pin3>0 && m_pin4>0) {
				// four-wire config
				switch(m_position) {
				case 0: // 1010
					setPinHigh(m_pin1);
					setPinLow(m_pin2);
					setPinHigh(m_pin3);
					setPinLow(m_pin4);
					break;
				case 1: // 0110
					setPinLow(m_pin1);
					setPinHigh(m_pin2);
					setPinHigh(m_pin3);
					setPinLow(m_pin4);
					break;
				case 2: // 0101
					setPinLow(m_pin1);
					setPinHigh(m_pin2);
					setPinLow(m_pin3);
					setPinHigh(m_pin4);
					break;
				case 3: // 1001
					setPinHigh(m_pin1);
					setPinLow(m_pin2);
					setPinLow(m_pin3);
					setPinHigh(m_pin4);
					break;
				}
			}
			else if(m_pin1>0 && m_pin2>0) {
				// two-wire config
				switch(m_position) {
				case 0: // 01
					setPinLow(m_pin1);
					setPinHigh(m_pin2);
					break;
				case 1: // 11
					setPinHigh(m_pin1);
					setPinHigh(m_pin2);
					break;
				case 2: // 10
					setPinHigh(m_pin1);
					setPinLow(m_pin2);
					break;
				case 3: // 00
					setPinLow(m_pin1);
					setPinLow(m_pin2);
					break;
				}
			}
			num_steps--;
		}
	}
}
