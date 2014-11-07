
extern "C"
{
  #include <avr32/io.h>
  #include <sysclk.h>
  #include <gpio.h>
}

#include "bsp_adc.h"

#if UC3L
extern "C" {
  #include <adcifb.h>
}

static bool adc_inited = false; // is adc module initialized
static uint32_t prev_pin = 0; // previous pin used for adc read
static uint32_t config_channels = 0; // adc channels/pins that are already configured

static adcifb_opt_t adcifb_opt = 
{
	AVR32_ADCIFB_ACR_RES_12BIT, // Resolution mode
	15, // Channels Sample & Hold Time in [0,15]
	sysclk_get_pba_hz() / 1500000, // Ratio Fclk_adcifb/Fclk_adc = 2*i with i an integer in [1,64]
	3,// ADC Startup time in [0,127], where Tstartup = startup * 8 * Tclk_adc
	false // ADCIFB Sleep Mode enable/disable
};

static gpio_map_t adcifb_gpio_map = { {0, 0} };

bool adcMode(uint32_t pin)
{
	adcifb_gpio_map[0].pin = pin;
	switch(pin)
	{
	case AVR32_PIN_PA14: // AD[0]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_0_FUNCTION;
		break;
	case AVR32_PIN_PA15: // AD[1]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_1_FUNCTION;
		break;
	case AVR32_PIN_PA16: // AD[2]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_2_FUNCTION;
		break;
	case AVR32_PIN_PA18: // AD[4]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_4_FUNCTION;
		break;
	case AVR32_PIN_PA19: // AD[5]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_5_FUNCTION;
		break;
	case AVR32_PIN_PB06: // AD[6]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_6_FUNCTION;
		break;
	case AVR32_PIN_PB07: // AD[7]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_7_FUNCTION;
		break;
	case AVR32_PIN_PB08: // AD[8]
		adcifb_gpio_map[0].function = AVR32_ADCIFB_AD_8_FUNCTION;
		break;
	default:
		return false;
	}
	
	/* Disable pull up on ADCIFB channel input pin */
	gpio_disable_pin_pull_up(adcifb_gpio_map[0].pin);
	/* Assign and enable GPIO pins to the ADC function. */
	gpio_enable_module(adcifb_gpio_map, sizeof(adcifb_gpio_map) / sizeof(adcifb_gpio_map[0]));
	
	if(!adc_inited) {
		/* Enable and configure the ADCIFB module */
		if(adcifb_configure(&AVR32_ADCIFB, &adcifb_opt) != PASS) return false;
		/* Configure the trigger mode : "No trigger, only software trigger can start conversions". */
		if (adcifb_configure_trigger(&AVR32_ADCIFB, AVR32_ADCIFB_TRGMOD_NT, 0) != PASS) return false;
		adc_inited = true;		
	}
	
	return true;
}

uint32_t adcRead(uint32_t pin)
{
	uint32_t channel_mask = 0;
	if(prev_pin != pin)
	{
		switch(pin)
		{
		case AVR32_PIN_PA14: // AD[0]
			channel_mask = AVR32_ADCIFB_CH0_MASK;
			break;
		case AVR32_PIN_PA15: // AD[1]
			channel_mask = AVR32_ADCIFB_CH1_MASK;
			break;
		case AVR32_PIN_PA16: // AD[2]
			channel_mask = AVR32_ADCIFB_CH2_MASK;
			break;
		case AVR32_PIN_PA18: // AD[4]
			channel_mask = AVR32_ADCIFB_CH4_MASK;
			break;
		case AVR32_PIN_PA19: // AD[5]
			channel_mask = AVR32_ADCIFB_CH5_MASK;
			break;
		case AVR32_PIN_PB06: // AD[6]
			channel_mask = AVR32_ADCIFB_CH6_MASK;
			break;
		case AVR32_PIN_PB07: // AD[7]
			channel_mask = AVR32_ADCIFB_CH7_MASK;
			break;
		case AVR32_PIN_PB08: // AD[8]
			channel_mask = AVR32_ADCIFB_CH8_MASK;
			break;
		default:
			return 0; // invalid pin
		}
		
		if( !(config_channels & channel_mask) ) {
			adcMode(pin);
			config_channels |= channel_mask;
		}
		
		adcifb_channels_disable(&AVR32_ADCIFB, ~channel_mask);
		/* Enable the selected ADCIFB channel. */
		adcifb_channels_enable(&AVR32_ADCIFB, channel_mask);
		prev_pin = pin;
	}
	
	/* Wait until the ADC is ready to perform a conversion. */
	while (adcifb_is_ready(&AVR32_ADCIFB) != true) { }
	
	/* Start an ADCIFB conversion sequence. */
	adcifb_start_conversion_sequence(&AVR32_ADCIFB);
	
	/* Wait until the converted data is available. */
	while (adcifb_is_drdy(&AVR32_ADCIFB) != true) { }
	
	/* Get the last converted data. */
	return ( adcifb_get_last_data(&AVR32_ADCIFB) & 0xFFF );
}

#elif UC3C
extern "C" {
  #include <adcifa.h>
}

#warning not yet supported

bool adcMode(uint32_t pin)
{
	return false;
}

uint32_t adcRead(uint32_t pin)
{
	return 0xFF;
}

#endif