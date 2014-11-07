
#ifndef SHT1X_X
#define SHT1X_X

#include <avr32/io.h>
#include <compiler.h>

class SHT1x
{
public:
	SHT1x();
	bool begin(uint32_t dataPin, uint32_t clockPin);
	
	bool readTemperature(float *pConvertedValue, uint16_t *pRawValue=NULL, uint8_t *pChecksum=NULL);
	bool readHumidity(float *pConvertedValue, float *pTemperature=NULL, uint8_t *pChecksum=NULL);
	
	static float dewpoint(float h, float t);
	
	bool reset();
	bool readStatusReg(uint8_t *pvalue, uint8_t *pchecksum);
	bool writeStatusReg(uint8_t *pvalue);
	
private:
	volatile uint32_t m_datPin;
	volatile uint32_t m_clkPin;
	bool writeByte(uint8_t value);
	uint8_t readByte(bool ack);
	void startTransmission();
	void resetConnection();
};

#endif //SHT1X_X
