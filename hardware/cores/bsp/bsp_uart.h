
#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#ifdef __cplusplus
extern "C" {

#define UART_BUFF_SIZE		1024
#define	UART_BUFF_MASK		(UART_BUFF_SIZE-1)

class HardwareUart
{
public:
	HardwareUart( uint8_t portNum = 3 );
	void begin( uint32_t baud = 115200 );
	void end();
	void putc( uint8_t c );
	void print( char ch );
	void print( int num );
	void puts( const char *s );
	uint8_t isrx();
	uint8_t getc();
	
	void flushRx();
	void flushTx();
	
	/* from ChaN's embedded string functions */
	void print( const char *fmt, ... );
	void vprint( const char* fmt, va_list arp );
	int gets( char* buff, int len );
	
	/* interrupt service routine */
	void _isr();

private:
	volatile uint8_t m_portNum;
	volatile avr32_usart_t *m_usart;
	
	volatile struct {
		uint16_t	inptr, outptr;			/* in/out index */
		uint8_t		buff[UART_BUFF_SIZE];	/* receive/transmit buffer */
	} TxFifo, RxFifo;
};

extern HardwareUart Serial0;
extern HardwareUart Serial1;
extern HardwareUart Serial2;
extern HardwareUart Serial3;

// debug Serial
#define Serial	Serial3

}
#endif // __cplusplus

#endif	// __BSP_UART_H__
