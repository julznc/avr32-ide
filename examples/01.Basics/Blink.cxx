#include <avr32.h>

int led = PA13;

int main(void)
{
	pinMode(led, OUTPUT);
	
	while(true)
	{
		digitalWrite(led, HIGH);
		delay(1000);
		digitalWrite(led, LOW);
		delay(1000);
	}
}

