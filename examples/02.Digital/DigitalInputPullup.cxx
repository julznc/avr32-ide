#include <avr32.h>

#define	ledPin		PA13
#define	buttonPin	PA02

int main(void)
{
	Serial.begin(9600);
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT_PULLUP);
	
	while(true)
	{
		int sensorVal = digitalRead(buttonPin);
		Serial.print("%d\r\n",sensorVal);
		
		if (sensorVal == HIGH)
			digitalWrite(ledPin, LOW);
		else
			digitalWrite(ledPin, HIGH);
		
		delay_ms(200);
	}
}
