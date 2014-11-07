#include <avr32.h>

// available ADC pins on UC3L0128
// PA14, PA15, PA16, PA18, PA19, PB06, PB07 & PB08
#define ADC_PIN PB07

int main(void)
{
	Serial.begin(115200);
	
	while(true)
	{
		// read the input on adc pin
		uint32_t adcval = analogRead(ADC_PIN);
		// print out the value and equivalent voltage (@3.3V supply)
		Serial.print("ADC value = %d (%d mV)\r\n", adcval, adcval*3300/4095);
		delay(1000);
	}
}
