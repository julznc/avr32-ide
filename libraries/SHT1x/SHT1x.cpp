
/*

based on SHT1x/7x demo program (SHT1x_sample_code.c) V2.4

*/

#include <SHT1x.h>
#include <bsp_clock.h>
#include <math.h> // for log10
extern "C" {
	#include <gpio.h>
}


#define NOACK 0
#define ACK   1
                            //adr  command  r/w
#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET        0x1e   //000   1111    0

#define CLK_HIGH	gpio_set_pin_high(m_clkPin)
#define CLK_LOW		gpio_set_pin_low(m_clkPin)
#define CLK_OUTPUT	gpio_configure_pin(m_clkPin, GPIO_DIR_OUTPUT)

#define DAT_HIGH	gpio_set_pin_high(m_datPin)
#define DAT_LOW		gpio_set_pin_low(m_datPin)

#define DAT_INPUT	gpio_configure_pin(m_datPin, GPIO_DIR_INPUT | GPIO_PULL_UP )
#define DAT_OUTPUT	gpio_configure_pin(m_datPin, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP | GPIO_INIT_HIGH)
#define DAT_READ	gpio_get_pin_value(m_datPin)

#define DELAY_PULSE	delay_us(5)
#define DELAY_SETUP	delay_us(1)

SHT1x::SHT1x(): m_datPin(0), m_clkPin(0)
{
}

bool SHT1x::begin(uint32_t dataPin, uint32_t clockPin)
{
	if( !((dataPin>=AVR32_PIN_PA00 && dataPin<=AVR32_PIN_PA22) \
		|| (dataPin>=AVR32_PIN_PB00 && dataPin<=AVR32_PIN_PB12))) return false;
	if( !((clockPin>=AVR32_PIN_PA00 && clockPin<=AVR32_PIN_PA22) \
		|| (clockPin>=AVR32_PIN_PB00 && clockPin<=AVR32_PIN_PB12))) return false;
	
	m_datPin = dataPin;
	m_clkPin = clockPin;
	
	CLK_OUTPUT;
	return true;
}

bool SHT1x::readTemperature(float *pConvertedValue, uint16_t *pRawValue, uint8_t *pChecksum)
{
	startTransmission();                   //transmission start
	if(!writeByte(MEASURE_TEMP)) return false; //send command to sensor
	DAT_INPUT;
	DELAY_SETUP;
	volatile unsigned long prev_millis = millis();
	//wait until sensor has finished the measurement
	while( (DAT_READ!=0) && ((unsigned long)(millis()-prev_millis)<2000) ) delay(1);
	if(DAT_READ) return false; // timeout
	
	uint16_t raw = (readByte(ACK)) << 8; //read the first byte (MSB)
	raw |= readByte(ACK); //read the second byte (LSB)
	if(pRawValue) *pRawValue = raw;
	
	uint8_t chk = readByte(NOACK); //read checksum
	if(pChecksum) *pChecksum = chk;
	
	*pConvertedValue = ((float)raw * 0.01) - 39.7; // calculate temperature [°C] from 14 bit temp. VDD @ 3.5V
	
	return true;
}

bool SHT1x::readHumidity(float *pConvertedValue, float *pTemperature, uint8_t *pChecksum)
{
	startTransmission();                   //transmission start
	if(!writeByte(MEASURE_HUMI)) return false; //send command to sensor
	DAT_INPUT;
	DELAY_SETUP;
	volatile unsigned long prev_millis = millis();
	//wait until sensor has finished the measurement
	while( (DAT_READ!=0) && ((unsigned long)(millis()-prev_millis)<2000) ) delay(1);
	if(DAT_READ) return false; // timeout
	
	uint16_t raw = (readByte(ACK)) << 8; //read the first byte (MSB)
	raw |= readByte(ACK); //read the second byte (LSB)
	
	uint8_t chk = readByte(NOACK); //read checksum
	if(pChecksum) *pChecksum = chk;
	
  // constants for 12 Bit RH
  #define C1	-2.0468
  #define C2	0.0367
  #define C3	-0.0000015955
  #define T1	0.01
  #define T2	0.00008
  
	float rh_lin; // linear humidity
	float rh_true; //  temperature compensated humidity
	
	rh_lin = C1 + C2*((float)raw) + C3*((float)raw)*((float)raw);
	if(*pTemperature)
		rh_true = (*pTemperature-25.0)*(T1+T2*((float)raw)) + rh_lin;
	else // @ 25°C
		rh_true = (T1+T2*((float)raw)) + rh_lin;
	// limit range
	if(rh_true>100) rh_true = 100.0;
	if(rh_true<0.1) rh_true = 0.1;
	
	*pConvertedValue = rh_true;
	
	return true;
}

float SHT1x::dewpoint(float h, float t)
{
// calculates dew point [°C]
	float k = (log10(h)-2)/0.4343 + (17.62*t)/(243.12+t);
	return ( 243.12*k/(17.62-k) );
}

bool SHT1x::reset(void)
{
// resets the sensor by a softreset 
	resetConnection(); //reset communication
	return writeByte(RESET); //send RESET-command to sensor
}

bool SHT1x::readStatusReg(uint8_t *pvalue, uint8_t *pchecksum)
{
// reads the status register with checksum (8-bit)
	startTransmission();             //transmission start
	if(!writeByte(STATUS_REG_R)) return false; //send command to sensor
	*pvalue = readByte(ACK);       //read status register (8-bit)
	*pchecksum = readByte(NOACK);  //read checksum (8-bit)  
	return true;
}

bool SHT1x::writeStatusReg(uint8_t *pvalue)
{
// writes the status register with checksum (8-bit)
	startTransmission();                   //transmission start
	if(!writeByte(STATUS_REG_W)) return false;//send command to sensor
	if(!writeByte(*pvalue)) return false;    //send value of status register
	return true;
}

//===============================================================================

bool SHT1x::writeByte(uint8_t value)
{
	DAT_OUTPUT;
	for (uint8_t i=0x80; i>0; i>>=1)
	{
		if (i & value) DAT_HIGH;
		else DAT_LOW;
		DELAY_SETUP;
		CLK_HIGH;
		DELAY_PULSE;
		CLK_LOW;
		DELAY_SETUP;
	}
	DAT_INPUT;
	DELAY_SETUP;
	CLK_HIGH;
	DELAY_PULSE;
	bool ack = (DAT_READ==0);
	CLK_LOW;
	return ack;
}

uint8_t SHT1x::readByte(bool ack)
{
	uint8_t val = 0;
	DAT_INPUT;
	for (uint8_t i=0x80; i>0; i>>=1)
	{
		CLK_HIGH;
		DELAY_PULSE;
		if(DAT_READ)
			val |= i;
		CLK_LOW;
		DELAY_SETUP;
	}
	DAT_OUTPUT;
	DELAY_SETUP;
	if(ack) DAT_LOW;
	else DAT_HIGH;
	DELAY_SETUP;
	CLK_HIGH;
	DELAY_PULSE;
	CLK_LOW;
	DELAY_SETUP;
	DAT_INPUT;
	return val;
}

void SHT1x::startTransmission()
{
// generates a transmission start 
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
	DAT_OUTPUT;
	DAT_HIGH; CLK_LOW;
	DELAY_SETUP;
	CLK_HIGH;
	DELAY_SETUP;
	DAT_LOW;
	DELAY_SETUP;
	CLK_LOW;
	DELAY_PULSE;
	CLK_HIGH;
	DELAY_SETUP;
	DAT_HIGH;
	DELAY_SETUP;
	CLK_LOW;
}

void SHT1x::resetConnection()
{
// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
	DAT_OUTPUT;
	DAT_HIGH; CLK_LOW;
	DELAY_SETUP;
	for(uint8_t i=0; i<9; i++)
	{
		CLK_HIGH; DELAY_PULSE;
		CLK_LOW; DELAY_SETUP;
	}
	startTransmission();
}




