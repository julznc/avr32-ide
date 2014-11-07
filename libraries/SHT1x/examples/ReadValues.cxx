#include <avr32.h>
#include <SHT1x.h>
#include <stdio.h> // sprintf

SHT1x sht;
char result[64];

int main(void)
{
	Serial.begin(115200);
	sht.begin(PB08, PB09);
	
	while(true)
	{
		float degC = 0, RH = 0;
		sht.readTemperature(&degC);
		sht.readHumidity(&RH, &degC);
		
		sprintf(result,
			"Temperature = %.2fC, "\
			"Humidity = %.1f%%, "\
			"Dew point = %.1fC\r\n",
			degC, RH, SHT1x::dewpoint(RH, degC) );
		Serial.puts(result);
		delay_ms(5000);
	}
}
