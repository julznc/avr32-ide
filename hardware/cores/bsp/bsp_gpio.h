
#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#define PA00	AVR32_PIN_PA00
#define PA01	AVR32_PIN_PA01
#define PA02	AVR32_PIN_PA02
#define PA03	AVR32_PIN_PA03
#define PA04	AVR32_PIN_PA04
#define PA05	AVR32_PIN_PA05
#define PA06	AVR32_PIN_PA06
#define PA07	AVR32_PIN_PA07
//#define PA08	AVR32_PIN_PA08 // XIN0
//#define PA09	AVR32_PIN_PA09 // XOUT0
//#define PA10	AVR32_PIN_PA10 // XIN32
#define PA11	AVR32_PIN_PA11
//#define PA12	AVR32_PIN_PA12 // XOUT32
#define PA13	AVR32_PIN_PA13
#define PA14	AVR32_PIN_PA14
#define PA15	AVR32_PIN_PA15
#define PA16	AVR32_PIN_PA16
#define PA17	AVR32_PIN_PA17
#define PA18	AVR32_PIN_PA18
#define PA19	AVR32_PIN_PA19
#define PA20	AVR32_PIN_PA20
#define PA21	AVR32_PIN_PA21
#define PA22	AVR32_PIN_PA22
#define PB00	AVR32_PIN_PB00
#define PB01	AVR32_PIN_PB01
#define PB02	AVR32_PIN_PB02
#define PB03	AVR32_PIN_PB03
#define PB04	AVR32_PIN_PB04
#define PB05	AVR32_PIN_PB05
#define PB06	AVR32_PIN_PB06
#define PB07	AVR32_PIN_PB07
#define PB08	AVR32_PIN_PB08
#define PB09	AVR32_PIN_PB09
#define PB10	AVR32_PIN_PB10
#define PB11	AVR32_PIN_PB11
#define PB12	AVR32_PIN_PB12


#ifndef HIGH
#define HIGH	1
#endif

#ifndef LOW
#define LOW		0
#endif

#define	INPUT				( GPIO_DIR_INPUT )
#define	OUTPUT				( GPIO_DIR_OUTPUT | GPIO_INIT_LOW )
#define	INPUT_PULLUP		( GPIO_DIR_INPUT | GPIO_PULL_UP )
#define	INPUT_PULLDOWN		( GPIO_DIR_INPUT | GPIO_PULL_DOWN )
#define	OUTPUT_OPENDRAIN	( GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN )

#define	pinMode(pin,mode)			gpio_configure_pin(pin, mode)
__always_inline static void digitalWrite(uint32_t pin, uint32_t value)
{
	if(value) gpio_set_pin_high(pin);
	else gpio_set_pin_low(pin);
}
#define	digitalRead(pin)			(gpio_get_pin_value(pin) != 0)

#define setPinHigh(pin)		gpio_set_pin_high(pin)
#define setPinLow(pin)		gpio_set_pin_low(pin)
#define togglePin(pin)		gpio_tgl_gpio_pin(pin)


#endif	// __BSP_GPIO_H__
