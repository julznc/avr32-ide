#include <avr32.h>

#define	ledPin		PA13
#define interval	1000

unsigned long previousMillis = 0;

int main(void)
{
	pinMode(ledPin, OUTPUT);  
	
	while(true)
	{
		unsigned long currentMillis = millis();
		if(currentMillis - previousMillis > interval) {
			previousMillis = currentMillis;
			togglePin(ledPin);
		}
	}
}
