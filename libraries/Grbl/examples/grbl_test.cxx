#include <avr32.h>
#include <Grbl.h>

int main(void)
{
	Grbl controller(&Serial);
	controller.init();
	
	while(true)
	{
		controller.reset();
		controller.loop();
	}
}
