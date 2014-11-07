
#include <SIM900.h>

SIM900::SIM900():
	m_ser(NULL),
	m_net(NET_SMART),
	m_pwron(-1)
{
}
bool SIM900::begin(HardwareUart *serial, uint8_t network, int pwron)
{
	gsmdebug("sim900 init: start...\r\n");
	
	m_ser = serial;
	m_net = network;
	m_pwron = pwron;
	
	if(!m_ser) return false;
	m_ser->begin(DEFAULT_SIM900_BAUD);
	
	if(m_pwron>=0)
	{
		pinMode(m_pwron, OUTPUT);
		delay(50);
	}
	
	if(!powerOn()) return false;
	gsmdebug("sim900 init: gsm power on -> ok\r\n");
	
	portFlushRX();
	gsmdebug("sim900 init: success!\r\n");
	
	return true;
}

bool SIM900::powerOn()
{
	if(m_pwron>=0){
		setPinLow(m_pwron);
		delay(50);
	}
		
	if(sendATcmd("AT", "OK", 200, 1)) goto done; // previously powered on
	
	if(m_pwron>=0){ // toggle power-on pin
		setPinHigh(m_pwron);
		delay(2000);
	}
	
	if(!sendATcmd("AT", "OK", 1000, 10)) // test auto baud
		return false; // e.g. timeout occurred
	
	delay(2000);
	if(m_pwron>=0)
		setPinLow(m_pwron);
	
done:
	if(!sendATcmd("ATE0", "OK")) return false; // turn-off echo
	gsmdebug("sim900 power on: ATE0 -> ok\r\n");
	
	gsmdebug("sim900 power on: done\r\n");
	return true;
}

bool SIM900::powerOff()
{
	return sendATcmd("AT+CPOWD=1", "POWER DOWN", 5000, 1); // normal power down
}

bool SIM900::sendATcmd(const char *cmd, const char *resp, uint32_t timeout, uint8_t retry, bool clearRX)
{
	uint8_t count = 0;
	if(!m_ser) return false; // todo: check gsm module status pin
	
	gsmdebug("sim900 send cmd: %s\r\n", cmd);
	
	while(true)
	{
		if(clearRX) portFlushRX();
		
		m_ser->print("%s\r\n", cmd); m_ser->flushTx(); // send AT command, append CRLF and wait until finished writing (flush output)
		if(!resp) goto done; // not expecting any response
		
		uint32_t prev_millis = millis();
		while( !m_ser->isrx() && (millis()-prev_millis<timeout)); // wait for a character
		if( !m_ser->isrx() ) { // timeout occurred
			count++;
			gsmdebug("  retry ( %d / %d )\r\n", count, retry);
			if( count < retry ) continue; // retry sending command
			else break; // max retry count reached
		}
		
		if( waitResponse(resp, timeout) == 0 ) break; // got the expected response
		if( ++count >= retry ) break; // max retry count reached
	}

done:
	return ( count < retry );
	
	return true;
}
	
int32_t SIM900::portReceiveRaw( const char *str_to_wait, uint16_t timeout, bool bInt )
{
	char *p = 0;
	uint8_t ch;
	uint16_t i;
	uint32_t ms;
	
	if(!m_ser) return (-1);
	
	memset(m_buff, '\0', SIM900_BUFF_SIZE);
	i = 0; ms = millis();
	do{
		if( m_ser->isrx() ){
			ch = m_ser->getc();
			if( bInt && (ch<'0' || ch>'9') ) break;
			m_buff[i++] = ch;
			if(str_to_wait){
				p = strstr( (const char *)m_buff, str_to_wait);
				if( p>0 ) return 0; // string to be checked was found
			}
			ms = millis();
		}
	}while( (i<SIM900_BUFF_SIZE-1) && ((uint32_t)(millis()-ms)<timeout) );
	//m_buff[i] = '\0';
	
	//gsmdebug("m_buff = %s [i = %d, str_to_wait = %s, p = %x]\r\n", m_buff, i, str_to_wait, p);
	if(bInt) return atoi( (const char *)m_buff); // decoded integer
	if(i<SIM900_BUFF_SIZE-1) return (-1); // timeout
	return (-2); // rx buffer full
}

size_t SIM900::readBytes(char *buff, size_t max_len)
{
	size_t len = 0;
	uint32_t ms = millis();
	
	if(!m_ser) return 0;
	
	while(len < max_len)
	{
		if( m_ser->isrx() ) {
			*buff++ = (char)m_ser->getc();
			len++;
			ms = millis();
		}
		else if((uint32_t)(millis()-ms)>500)
			break; // time out occured
	}
	return len;
}

size_t SIM900::readBytesUntil( char ch, char *buff, size_t max_len)
{
	size_t len = 0;
	char tmp;
	uint32_t ms = millis();
	
	if(!m_ser) return 0;
	
	while(len < max_len)
	{
		if( m_ser->isrx() ) {
			tmp = (char)m_ser->getc();
			if(tmp ==  ch) break;
			*buff++ = tmp;
			len++;
			ms = millis();
		}
		else if((uint32_t)(millis()-ms)>500)
			break; // time out occured
	}
	return len;
}
