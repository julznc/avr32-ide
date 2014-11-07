
extern "C"
{
#include <avr32/io.h>
#include <power_clocks_lib.h>
#include <gpio.h>
#include <usart.h>
}

#include <stdint.h>
#include <stdarg.h>
#include "bsp_clock.h"
#include "bsp_uart.h"

#if UC3L

HardwareUart Serial0( 0 );
HardwareUart Serial1( 1 );
HardwareUart Serial2( 2 );
HardwareUart Serial3( 3 );

__attribute__((__interrupt__)) static void usart0_int_handler(void)
{
	Serial0._isr();
}

__attribute__((__interrupt__)) static void usart1_int_handler(void)
{
	Serial1._isr();
}

__attribute__((__interrupt__)) static void usart2_int_handler(void)
{
	Serial2._isr();
}

__attribute__((__interrupt__)) static void usart3_int_handler(void)
{
	Serial3._isr();
}

static const gpio_map_t usart0_gpio =
{
	{AVR32_USART0_RXD_0_PIN, AVR32_USART0_RXD_0_FUNCTION}, // PA01
	{AVR32_USART0_TXD_0_PIN, AVR32_USART0_TXD_0_FUNCTION}  // PA00
};

static const gpio_map_t usart1_gpio =
{
	{AVR32_USART1_RXD_0_1_PIN, AVR32_USART1_RXD_0_1_FUNCTION}, // PB11
	{AVR32_USART1_TXD_0_1_PIN, AVR32_USART1_TXD_0_1_FUNCTION}  // PB10
};

static const gpio_map_t usart2_gpio =
{
	{AVR32_USART2_RXD_0_0_PIN, AVR32_USART2_RXD_0_0_FUNCTION}, // PA21
	{AVR32_USART2_TXD_0_0_PIN, AVR32_USART2_TXD_0_0_FUNCTION}  // PA20
};

static const gpio_map_t usart3_gpio =
{
	{AVR32_USART3_RXD_0_0_PIN, AVR32_USART3_RXD_0_0_FUNCTION}, // PB01
	{AVR32_USART3_TXD_0_0_PIN, AVR32_USART3_TXD_0_0_FUNCTION}  // PB00
};

#elif UC3C
HardwareUart Serial0( 0 );
HardwareUart Serial1( 1 );
HardwareUart Serial2( 2 );
HardwareUart Serial3( 3 );

  #warning not yet supported

#endif


HardwareUart::HardwareUart( uint8_t portNum ):
	m_portNum(portNum),
	m_usart(NULL)
{
}

void HardwareUart::begin( uint32_t baud )
{
	usart_options_t m_options;

#if 1 // static constructor workaround
	if(this == &Serial0)
		m_portNum = 0;
	else if(this == &Serial1)
		m_portNum = 1;
	else if(this == &Serial2)
		m_portNum = 2;
	else if(this == &Serial3)
		m_portNum = 3;
#endif

#if UC3L
	// Assign GPIO to USART.
	switch(m_portNum)
	{
	case 0:
		m_usart = &AVR32_USART0;
		gpio_enable_module(usart0_gpio, sizeof(usart0_gpio) / sizeof(usart0_gpio[0]));
		break;
	case 1:
		m_usart = &AVR32_USART1;
		gpio_enable_module(usart1_gpio, sizeof(usart1_gpio) / sizeof(usart1_gpio[0]));
		break;
	case 2:
		m_usart = &AVR32_USART2;
		gpio_enable_module(usart2_gpio, sizeof(usart2_gpio) / sizeof(usart2_gpio[0]));
		break;
	case 3:
		m_usart = &AVR32_USART3;
		gpio_enable_module(usart3_gpio, sizeof(usart3_gpio) / sizeof(usart3_gpio[0]));
		break;
	default:
		m_usart = NULL;
		break;
	}
#elif UC3C

#endif

	if(!m_usart) return;

	m_options.baudrate = baud;
	m_options.charlength = 8;
	m_options.paritytype = USART_NO_PARITY;
	m_options.stopbits = USART_1_STOPBIT;
	m_options.channelmode = USART_NORMAL_CHMODE;

	// Initialize USART in RS232 mode.
	usart_init_rs232(m_usart, &m_options, sysclk_get_pba_hz());


	// Disable all interrupts.
	Disable_global_interrupt();
#if UC3L
	switch(m_portNum)
	{
	case 0:
		INTC_register_interrupt(&usart0_int_handler, AVR32_USART0_IRQ, AVR32_INTC_INT0);
		break;
	case 1:
		INTC_register_interrupt(&usart1_int_handler, AVR32_USART1_IRQ, AVR32_INTC_INT0);
		break;
	case 2:
		INTC_register_interrupt(&usart2_int_handler, AVR32_USART2_IRQ, AVR32_INTC_INT0);
		break;
	case 3:
		INTC_register_interrupt(&usart3_int_handler, AVR32_USART3_IRQ, AVR32_INTC_INT0);
		break;
	}
#elif UC3C

#endif
	// clear FIFO buffers
	TxFifo.inptr = TxFifo.outptr = RxFifo.inptr = RxFifo.outptr = 0;

	// Enable USART Rx interrupt.
	m_usart->ier = AVR32_USART_IER_RXRDY_MASK;

	// Enable all interrupts.
	Enable_global_interrupt();

	delay_ms(50);
	//print("m_portNum = %d\r\n", m_portNum);
}

void HardwareUart::end()
{
	if(!m_usart) return;
	pcl_disable_module(AVR32_USART3_CLK_PBA);
}

void HardwareUart::putc(uint8_t c)
{
	if(!m_usart) return;

	// wait until buffer has an empty slot.
	while (( (TxFifo.inptr+1) & UART_BUFF_MASK) == TxFifo.outptr) continue;

	//place character in buffer
	TxFifo.buff[TxFifo.inptr] = c;
	// increment in index
	TxFifo.inptr = (TxFifo.inptr + 1) & UART_BUFF_MASK;
	// start the TX sequence if not yet running
	if(!(TxFifo.outptr == TxFifo.inptr)) { // if TX buffer still not empty
		m_usart->ier = AVR32_USART_IER_TXEMPTY_MASK; // (re)enable TX interrupt;
	}
}

void HardwareUart::print(char ch)
{
	putc(ch);
}

void HardwareUart::print(int num)
{
	print("%d", num);
}

void HardwareUart::puts(const char *s)
{
	while(*s) putc(*s++);
}

uint8_t HardwareUart::isrx()
{
	if(!m_usart) return 0;

	//return (m_usart->csr & AVR32_USART_CSR_RXRDY_MASK) != 0;
	// checks if a character is present in the RX buffer
	return (RxFifo.inptr != RxFifo.outptr);
}

uint8_t HardwareUart::getc()
{
	if(!m_usart) return 0;

	// wait until a character is present
	while (isrx()==0) continue;

	// get a character from RX buffer
	uint8_t c = RxFifo.buff[RxFifo.outptr];
	// increment out index
	RxFifo.outptr = (RxFifo.outptr+1)&UART_BUFF_MASK;

	return c;
}

void HardwareUart::flushRx()
{
	while(isrx()) getc();
}

void HardwareUart::flushTx()
{
	while(TxFifo.outptr!=TxFifo.inptr) continue;
}

void HardwareUart::_isr()
{
	uint16_t temp;

	if(!m_usart) return;

	if(m_usart->csr & AVR32_USART_CSR_RXRDY_MASK)
	{
		// byte read and save to buffer
		RxFifo.buff[RxFifo.inptr] = ((m_usart->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET) & 0xFF;
		temp = (RxFifo.inptr+1) & UART_BUFF_MASK;
		if(temp != RxFifo.outptr){	// avoid buffer overrun
			RxFifo.inptr = temp;
		}
	}
	if(m_usart->csr & AVR32_USART_CSR_TXEMPTY_MASK)
	{
		if(TxFifo.outptr==TxFifo.inptr) {// if buffer empty
			m_usart->idr = AVR32_USART_IER_TXEMPTY_MASK; // disable TX interrupt
		}
		else {
			// place the character to the data register
			m_usart->thr = TxFifo.buff[TxFifo.outptr++];
			TxFifo.outptr &= UART_BUFF_MASK; // circular FIFO
		}
	}
}

/*------------------------------------------------------------------------/
/  Universal string handler for user console interface
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

void HardwareUart::vprint (
	const char*	fmt,	/* Pointer to the format string */
	va_list arp			/* Pointer to arguments */
	)
{
	unsigned int r, i, j, w, f;
	unsigned long v;
	char s[16], c, d, *p;

	for (;;) {
		c = *fmt++;					/* Get a char */
		if (!c) break;				/* End of format? */
		if (c != '%') {				/* Pass through it if not a % sequense */
			putc(c); continue;
		}
		f = 0;
		c = *fmt++;					/* Get first char of the sequense */
		if (c == '0') {				/* Flag: '0' padded */
			f = 1; c = *fmt++;
		} else {
			if (c == '-') {			/* Flag: left justified */
				f = 2; c = *fmt++;
			}
		}
		for (w = 0; c >= '0' && c <= '9'; c = *fmt++)	/* Minimum width */
			w = w * 10 + c - '0';
		if (c == 'l' || c == 'L') {	/* Prefix: Size is long int */
			f |= 4; c = *fmt++;
		}
		if (!c) break;				/* End of format? */
		d = c;
		if (d >= 'a') d -= 0x20;
		switch (d) {				/* Type is... */
		case 'S' :					/* String */
			p = va_arg(arp, char*);
			for (j = 0; p[j]; j++) ;
			while (!(f & 2) && j++ < w) putc(' ');
			puts(p);
			while (j++ < w) putc(' ');
			continue;
		case 'C' :					/* Character */
			putc((char)va_arg(arp, int)); continue;
		case 'B' :					/* Binary */
			r = 2; break;
		case 'O' :					/* Octal */
			r = 8; break;
		case 'D' :					/* Signed decimal */
		case 'U' :					/* Unsigned decimal */
			r = 10; break;
		case 'X' :					/* Hexdecimal */
			r = 16; break;
		default:					/* Unknown type (passthrough) */
			putc(c); continue;
		}

		/* Get an argument and put it in numeral */
		v = (f & 4) ? va_arg(arp, long) : ((d == 'D') ? (long)va_arg(arp, int) : (long)va_arg(arp, unsigned int));
		if (d == 'D' && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (char)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			s[i++] = d + '0';
		} while (v && i < sizeof(s));
		if (f & 8) s[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) putc(d);
		do putc(s[--i]); while(i);
		while (j++ < w) putc(' ');
	}
}


void HardwareUart::print (			/* Put a formatted string to the default device */
	const char*	fmt,	/* Pointer to the format string */
	...					/* Optional arguments */
	)
{
	va_list arp;

	va_start(arp, fmt);
	vprint(fmt, arp);
	va_end(arp);
}

int HardwareUart::gets (		/* 0:End of stream, 1:A line arrived */
	char* buff,	/* Pointer to the buffer */
	int len		/* Buffer length */
	)
{
	int c, i;
#define	_LINE_ECHO		1
	i = 0;
	for (;;) {
		c = getc();				/* Get a char from the incoming stream */
		if (!c) return 0;			/* End of stream? */
		if (c == '\r') break;		/* End of line? */
		if (c == '\b' && i) {		/* Back space? */
			i--;
			if (_LINE_ECHO) putc(c);
			continue;
		}
		if (c >= ' ' && i < len - 1) {	/* Visible chars */
			buff[i++] = c;
			if (_LINE_ECHO) putc(c);
		}
	}
	buff[i] = 0;	/* Terminate with a \0 */
	if (_LINE_ECHO) putc('\n');
	return 1;
}
