#include <avr32.h>

#define EXAMPLE_TC                 (&AVR32_TC1)
#define EXAMPLE_TC_CHANNEL         0
#define EXAMPLE_TC_IRQ             AVR32_TC1_IRQ0
#define EXAMPLE_TC_IRQ_GROUP       AVR32_TC1_IRQ_GROUP
#define EXAMPLE_TC_IRQ_PRIORITY    AVR32_INTC_INT0

// Flag to update the timer value
volatile static bool update_timer = true;
// Variable to contain the time ticks occurred
volatile static uint32_t tc_tick = 0;

__attribute__((__interrupt__)) static void tc_irq(void)
{
	// Increment the ms seconds counter
	tc_tick++;

	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(EXAMPLE_TC, EXAMPLE_TC_CHANNEL);

	// specify that an interrupt has been raised
	update_timer = true;
}

void tc_init(volatile avr32_tc_t *tc)
{
	// Options for waveform generation.
	tc_waveform_opt_t waveform_opt;
	// Channel selection.
	waveform_opt.channel  = EXAMPLE_TC_CHANNEL;
	// Software trigger effect on TIOB.
	waveform_opt.bswtrg   = TC_EVT_EFFECT_NOOP;
	// External event effect on TIOB.
	waveform_opt.beevt    = TC_EVT_EFFECT_NOOP;
	// RC compare effect on TIOB.
	waveform_opt.bcpc     = TC_EVT_EFFECT_NOOP;
	// RB compare effect on TIOB.
	waveform_opt.bcpb     = TC_EVT_EFFECT_NOOP;
	// Software trigger effect on TIOA.
	waveform_opt.aswtrg   = TC_EVT_EFFECT_NOOP;
	// External event effect on TIOA.
	waveform_opt.aeevt    = TC_EVT_EFFECT_NOOP;
	// RC compare effect on TIOA.
	waveform_opt.acpc     = TC_EVT_EFFECT_NOOP;
	/*
	 * RA compare effect on TIOA.
	 * (other possibilities are none, set and clear).
	 */
	waveform_opt.acpa     = TC_EVT_EFFECT_NOOP;
	/*
	 * Waveform selection: Up mode with automatic trigger(reset)
	 * on RC compare.
	 */
	waveform_opt.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER;
	// External event trigger enable.
	waveform_opt.enetrg   = false;
	// External event selection.
	waveform_opt.eevt     = 0;
	// External event edge selection.
	waveform_opt.eevtedg  = TC_SEL_NO_EDGE;
	// Counter disable when RC compare.
	waveform_opt.cpcdis   = false;
	// Counter clock stopped with RC compare.
	waveform_opt.cpcstop  = false;
	// Burst signal selection.
	waveform_opt.burst    = false;
	// Clock inversion.
	waveform_opt.clki     = false;
	// Internal source clock 3, connected to fPBA / 8.
	waveform_opt.tcclks   = TC_CLOCK_SOURCE_TC3;
	
	// Options for enabling TC interrupts
	tc_interrupt_t tc_interrupt;
	tc_interrupt.etrgs = 0;
	tc_interrupt.ldrbs = 0;
	tc_interrupt.ldras = 0;
	tc_interrupt.cpcs  = 1; // Enable interrupt on RC compare alone
	tc_interrupt.cpbs  = 0;
	tc_interrupt.cpas  = 0;
	tc_interrupt.lovrs = 0;
	tc_interrupt.covfs = 0;
	
	// Initialize the timer/counter.
	tc_init_waveform(tc, &waveform_opt);
	
		/*
	 * Set the compare triggers.
	 * We configure it to count every 1 milliseconds.
	 * We want: (1 / (fPBA / 8)) * RC = 1 ms, hence RC = (fPBA / 8) / 1000
	 * to get an interrupt every 10 ms.
	 */
	tc_write_rc(tc, EXAMPLE_TC_CHANNEL, (sysclk_get_pba_hz() / 8 / 1000));
	// configure the timer interrupt
	tc_configure_interrupts(tc, EXAMPLE_TC_CHANNEL, &tc_interrupt);
	// Start the timer/counter.
	tc_start(tc, EXAMPLE_TC_CHANNEL);
}

int main(void)
{
	volatile avr32_tc_t *tc = EXAMPLE_TC;
	uint32_t timer = 0;
	
	Serial.begin(115200);
	Serial.print("ATMEL AVR UC3 - Timer/Counter Example 3\r\n");
	
	// Enable the clock to the selected example Timer/counter peripheral module.
	sysclk_enable_peripheral_clock(EXAMPLE_TC);

	// Disable the interrupts
	cpu_irq_disable();
	
	// Register the RTC interrupt handler to the interrupt controller.
	INTC_register_interrupt(&tc_irq, EXAMPLE_TC_IRQ, EXAMPLE_TC_IRQ_PRIORITY);
	
	// Enable the interrupts
	cpu_irq_enable();
	
	// Initialize the timer module
	tc_init(tc);
	
	while(true)
	{
		// Update the display on USART every second.
		if ((update_timer) && (!(tc_tick%1000))) {
			timer++;
			// Print the timer value
			Serial.print("Timer: %ld s\r\n", timer);
			// Reset the timer update flag to wait till next timer interrupt
			update_timer = false;
		}
	}
}
