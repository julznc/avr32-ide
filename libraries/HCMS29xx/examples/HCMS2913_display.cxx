#include <avr32.h>
#include <HCMS29xx.h>

HCMS29xx myLed;
const char message[] = "    HCMS-2913 tested w/ AT32UC3L    ";

int main(void)
{
	myLed.begin(PB02 /*data*/, PB03 /*clock*/,
		PB04 /*rs*/, PB05 /*ce*/, PB06 /*reset*/);
	
	myLed.displayMessage("HCMS2913");
	delay(5000);
	
	while(true)
	{
		myLed.scrollMessage(message, sizeof(message));
	}
}
