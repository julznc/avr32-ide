
#include <SIM900.h>

SIM900GSM::SIM900GSM()
{
}

bool SIM900GSM::begin(HardwareUart *serial, uint8_t network, int pwron)
{
	if( !SIM900::begin(serial, network, pwron) ) return false;
	
	if(!sendATcmd("AT+CMGF=1", "OK")) return false; // text mode
	gsmdebug("sms init: AT+CMGF=1 -> ok\r\n");
	
	return true;
}
	
bool SIM900GSM::sendSMS( const char *number, const char *message )
{
	gsmdebug("sim900 send sms...\r\n");

	m_ser->print("AT+CMGS=\"");
	m_ser->print(number);
	if(!sendATcmd("\"", ">", 5000, 1)) return false;
	gsmdebug("send sms: begin message\r\n");
	
	m_ser->print(message);
	m_ser->print((char)0x1A ); // append CTRL+Z
	gsmdebug("send sms: end message\r\n");
	
	uint32_t prev_millis = millis();
	while( !m_ser->isrx() && (millis()-prev_millis<10000)); // wait for a character
	if( !m_ser->isrx() ) return false; // timeout occurred
	
	if( waitResponse("+CMGS", DEFAULT_SIM900_TIMEOUT) < 0 ) return false;
	gsmdebug("send sms OK\r\n");
	return true;
}

bool SIM900GSM::deleteSMS( int16_t index, delete_flag flag )
{
	gsmdebug("sim900 delete sms...\r\n");
	
	if( index<1 || flag<DEL_INDEX || flag>DEL_ALL ) return false;
	
	m_ser->print("AT+CMGD=%d,%d", index, (int)flag);
	
	if(!sendATcmd("", "OK", 5000, 1)) return false;
	
	gsmdebug("delete sms index[%d]\r\n", index);	
	return true;
}

bool SIM900GSM::getSMS( int16_t index, char *msg_txt, size_t msg_len, char *sender, char *timestamp  )
{
	size_t len;
	gsmdebug("sim900 get sms...\r\n");
	
	if( index<1 || !msg_txt ) return false;
	
	portFlushRX();
	m_ser->print("AT+CMGR=%d\r\n", index);
	
	uint32_t prev_millis = millis();
	while( !m_ser->isrx() && (millis()-prev_millis<2000)); // wait for a character
	if( !m_ser->isrx() ) return false; // timeout occurred
	
	// format:
	//<CR><LF>+CMGR: "<message_status>","<sender>",,"<timestamp>"<CR><LF><message_body><CR><LF><CR><LF>OK<CR><LF>
	
	gsmdebug("get sms: decode sender\r\n");
	if( waitResponse("\",\"", 500) < 0 ) return false;
	if( sender && !readBytesUntil('\"', sender, 15) )  return false;
	
	gsmdebug("get sms: decode timestamp\r\n");
	if( waitResponse(",\"", 500) < 0 ) return false;
	if( timestamp && !readBytesUntil('\"', timestamp, 22) )  return false;

	gsmdebug("get sms: decode message body\r\n");
	if( waitResponse("\r\n", 500) < 0 ) return false;

	len = readBytes(msg_txt, msg_len);
	if( !len )  return false;
	
//#define ENDSTR   "\r\nOK\r\n"
#define ENDSTR    "\r\n\r\nOK\r\n"
#define ENDSIZE   sizeof(ENDSTR)-1

	if( len>ENDSIZE && len<msg_len ) {
		if( strstr(msg_txt + len - ENDSIZE, ENDSTR) - msg_txt + ENDSIZE == len )
			msg_txt[len-ENDSIZE+1+1] = 0; // disregard end string
	}
	
	gsmdebug("get sms OK\r\n");
	return true;
}

int16_t SIM900GSM::hasSMS( message_status status )
{
	int16_t idx = -1;
	
	gsmdebug("gsm has sms?...\r\n");
	
	switch(status)
	{
	case REC_UNREAD:
		m_ser->print("AT+CMGL=\"REC UNREAD\"");
		break;
	case REC_READ:
		m_ser->print("AT+CMGL=\"REC READ\"");
		break;
	case STO_UNSENT:
		m_ser->print("AT+CMGL=\"STO UNSENT\"");
		break;
	case STO_SENT:
		m_ser->print("AT+CMGL=\"STO SENT\"");
		break;
	case MSG_ALL:
		m_ser->print("AT+CMGL=\"ALL\"");
		break;
	default:
		return (-1);
	}
	
	if(!sendATcmd("", "+CMGL: ", 5000, 1)) return (-1);
	
	idx = (int16_t)getIntegerResponse();
	gsmdebug("has sms: index = %d\r\n", idx);
	return idx;
}
