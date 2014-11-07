
#ifndef TWOWIREMASTER_H
#define TWOWIREMASTER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <avr32/io.h>
#include <gpio.h>

#ifndef TWOWIRESLAVE_H
#include <twim.h>
#endif

class TwoWireMaster
{
private:
	bool m_initialied;
	uint8_t m_port;
	uint32_t m_clk_pin;
	uint32_t m_data_pin;
public:
	TwoWireMaster();
	bool begin(uint32_t clk_pin, uint32_t data_pin, uint32_t speed, uint32_t chip = 0);
};


#ifdef __cplusplus
}
#endif

#endif // TWOWIREMASTER_H
