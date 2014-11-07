#include <avr32.h>

#define	ledPin		PA13
#define	buttonPin	PA02

int buttonState = 0;

int main(void)
{
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT);
	
	while(true)
	{
		buttonState = digitalRead(buttonPin);
		if (buttonState == HIGH)
			digitalWrite(ledPin, HIGH);
		else
			digitalWrite(ledPin, LOW);
	}
}
