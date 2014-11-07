/*
MultiSerial
*/

#include <avr32.h>

int main(void)
{
	Serial.begin(115200);
	Serial1.begin(115200);
	
	while(true)
	{
		if (Serial.isrx()){
			Serial1.putc( Serial.getc() ); 
		}
		if (Serial1.isrx()){
			Serial.putc( Serial1.getc() ); 
		}
	}
}
