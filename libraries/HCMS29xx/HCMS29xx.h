
#ifndef HCMS29XX_H
#define HCMS29XX_H

#include <avr32/io.h>
#include <compiler.h>

class HCMS29xx
{
public:
	HCMS29xx();
	bool begin(uint32_t dataPin, uint32_t clockPin, uint32_t registerSelectPin,
				uint32_t chipEnablePin, uint32_t resetPin, uint8_t displayLength = 8 /* 4 or 8 */,
				uint8_t peakCurrent = 0b01 /* 0 to 3 */, uint8_t brightness = 14 /* 0 to 15 */);

	void displayMessage(const char *msg);
	void scrollMessage(const char *msg, size_t length, uint32_t scroll_delay = 400);
	
private:
	uint32_t datPin;
	uint32_t clkPin;
	uint32_t regPin;
	uint32_t cePin;
	uint32_t rstPin;
	uint8_t dispLen;

	void sendByte(uint8_t data);
};

#endif  // HCMS29XX_H
