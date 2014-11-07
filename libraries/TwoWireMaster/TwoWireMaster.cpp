
#include "TwoWireMaster.h"
#include <sysclk.h>

TwoWireMaster::TwoWireMaster() :
	m_initialied(false),
	m_port(0),
	m_clk_pin(0),
	m_data_pin(0)
{

}

bool TwoWireMaster::begin(uint32_t clk_pin, uint32_t data_pin, uint32_t speed, uint32_t chip)
{
	gpio_map_t gpio_map = { {0,0}, {0,0} };	
	gpio_map[0].pin = m_clk_pin = clk_pin;
	gpio_map[1].pin = m_data_pin = data_pin;
	
	switch(clk_pin)
	{
	case AVR32_TWIMS0_TWCK_0_0_PIN: //PA04
		gpio_map[0].function = AVR32_TWIMS0_TWCK_0_0_FUNCTION;
		m_port = 0;
		break;
	case AVR32_TWIMS0_TWCK_0_1_PIN: //PA20(?)
		gpio_map[0].function = AVR32_TWIMS0_TWCK_0_1_FUNCTION;
		m_port = 0;
		break;
	case AVR32_TWIMS0_TWCK_0_2_PIN: //PA07
		gpio_map[0].function = AVR32_TWIMS0_TWCK_0_2_FUNCTION;
		m_port = 0;
		break;
	case AVR32_TWIMS0_TWCK_0_3_PIN: //PB05:
		gpio_map[0].function = AVR32_TWIMS0_TWCK_0_3_FUNCTION;
		m_port = 0;
		break;
	case AVR32_TWIMS1_TWCK_0_0_PIN: //PA05
		gpio_map[0].function = AVR32_TWIMS1_TWCK_0_0_FUNCTION;
		m_port = 1;
		break;
	case AVR32_TWIMS1_TWCK_0_1_PIN: //PB04
		gpio_map[0].function = AVR32_TWIMS1_TWCK_0_1_FUNCTION;
		m_port = 1;
		break;
	default:
		return false;
	}
	
	switch(data_pin)
	{
	case AVR32_TWIMS0_TWD_0_0_PIN: //PA10
		if(m_port!=0) return false;
		gpio_map[0].function = AVR32_TWIMS0_TWD_0_0_FUNCTION;
		break;
	case AVR32_TWIMS0_TWD_0_1_PIN: //PA21
		if(m_port!=0) return false;
		gpio_map[0].function = AVR32_TWIMS0_TWD_0_1_FUNCTION;
		break;
	case AVR32_TWIMS0_TWD_0_2_PIN: //PA05
		if(m_port!=0) return false;
		gpio_map[0].function = AVR32_TWIMS0_TWD_0_2_FUNCTION;
		break;
	case AVR32_TWIMS1_TWD_0_PIN: //PA17
		if(m_port!=1) return false;
		gpio_map[0].function = AVR32_TWIMS1_TWD_0_FUNCTION;
		break;
	default:
		return false;
	}
	
	// TWIM gpio pins configuration
	gpio_enable_module (gpio_map, sizeof (gpio_map) / sizeof (gpio_map[0]));

	twim_options_t twim_opt = {
		sysclk_get_pba_hz(), // The PBA clock frequency.
		speed, // The baudrate of the TWI bus.
		chip, // The desired address.
		false // SMBUS mode
	};
	
	// Initialize as master
	if(m_port==0) {
		if(gpio_map[0].pin!=AVR32_TWIMS0_TWCK_0_3_PIN)
			gpio_enable_gpio_pin(AVR32_TWIMS0_TWCK_0_3_PIN); // PB05
		if(gpio_map[1].pin!=AVR32_TWIMS0_TWD_0_1_PIN)
			gpio_enable_gpio_pin(AVR32_TWIMS0_TWD_0_1_PIN); // PA21
		m_initialied = ( twim_master_init(&AVR32_TWIM0, &twim_opt) == STATUS_OK );
	}
	else if(m_port==1) {
		if(gpio_map[0].pin!=AVR32_TWIMS1_TWCK_0_1_PIN)
			gpio_enable_gpio_pin(AVR32_TWIMS1_TWCK_0_1_PIN); // PB04
		m_initialied = ( twim_master_init(&AVR32_TWIM1, &twim_opt) == STATUS_OK );
	}
	
	return m_initialied;
}
