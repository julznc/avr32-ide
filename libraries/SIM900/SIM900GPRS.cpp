
#include <SIM900.h>

SIM900GPRS::SIM900GPRS()
{
}

bool SIM900GPRS::begin(HardwareUart *serial, uint8_t network, int pwron)
{
	if( !SIM900::begin(serial, network, pwron) ) return false;
	
	if(!configGPRS(m_net)) return false;
	
	portFlushRX();
	
	return true;
}
	
bool SIM900GPRS::configGPRS(uint8_t network)
{
	bool ret;
	
	if(sendATcmd("AT+SAPBR=4,1", "CONTYPE: GPRS", 500, 4)) goto check_apn; // connection type already set to GPRS
	
	if(!sendATcmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK")) return false; // configure bearer profile
	gsmdebug("config GPRS: set contype -> ok\r\n");

check_apn: // check first existing apn
	switch(network)
	{
	case NET_SMART_BRO:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_SMART_BRO, 100, 2)) goto done;
		break;
	case NET_SUN_POSTPAID:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_SUN_POSTPAID, 100, 2)) goto done;
		break;
	case NET_SUN_PREPAID:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_SUN_PREPAID, 100, 2)) goto done;
		break;
	case NET_GLOBE_POSTPAID:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_GLOBE_POSTPAID, 100, 2)) goto done;
		break;
	case NET_GLOBE_PREPAID:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_GLOBE_PREPAID, 100, 2)) goto done;
		break;
	case NET_PLDT_WEROAM:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_PLDT_WEROAM, 100, 2)) goto done;
		break;
	case NET_SMART:
	default:
		if(sendATcmd("AT+SAPBR=4,1", "APN: " APN_SMART, 100, 2)) goto done;
		break;
	}

	// set APN
	switch(network)
	{
	case NET_SMART_BRO:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_SMART_BRO "\"", "OK");
		break;
	case NET_SUN_POSTPAID:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_SUN_POSTPAID "\"", "OK");
		break;
	case NET_SUN_PREPAID:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_SUN_PREPAID "\"", "OK");
		break;
	case NET_GLOBE_POSTPAID:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_GLOBE_POSTPAID "\"", "OK");
		break;
	case NET_GLOBE_PREPAID:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_GLOBE_PREPAID "\"", "OK");
		break;
	case NET_PLDT_WEROAM:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_PLDT_WEROAM "\"", "OK");
		break;
	case NET_SMART:
	default:
		ret = sendATcmd("AT+SAPBR=3,1,\"APN\",\"" APN_SMART "\"", "OK");
		break;
	}
	if(!ret) return false; // failed to set APN
	gsmdebug("config GPRS: set APN -> ok\r\n");
	
	if(sendATcmd("AT+SAPBR=5,1", "OK")) // try to save to memory (if supported)
		gsmdebug("config GPRS: save to NVRAM -> ok\r\n"); 
done:
	gsmdebug("config GPRS: success!\r\n");
	return true;
}

bool SIM900GPRS::openGPRS()
{
	bool ret = false;
	uint8_t retry = DEFAULT_SIM900_RETRY;
	while( !(ret=isOpenedGPRS()) && (retry--) ) // while not opened
	{
		if(ret) return true;
		if(!sendATcmd("AT+SAPBR=1,1", "OK", 10000, 1)) return false; // try to open
		delay(5000); // wait for possible disconnect response i.e. "+SAPBR 1: DEACT"
		
		gsmdebug("open GPRS retry [%d / %d]\r\n", DEFAULT_SIM900_RETRY - retry, DEFAULT_SIM900_RETRY);
	}
	return ret;
}

bool SIM900GPRS::closeGPRS()
{
	if(!isOpenedGPRS()) return false; // not opened
	return sendATcmd("AT+SAPBR=0,1", "OK", 500, 1);
}

bool SIM900GPRS::isOpenedGPRS()
{
	if( waitResponse("+SAPBR 1: DEACT", 500) < 0 ) {
		gsmdebug("gprs was closed!\r\n");
		return false;
	}
	bool ret = sendATcmd("AT+SAPBR=2,1", "+SAPBR: 1,1,\"", 2000, 1);
	if(ret){
		delay(100);
		gsmdebug("gprs open: ip add = ");
		while(m_ser->isrx() > 0){
			char ch = (char)m_ser->getc();
			if(ch>'-' && ch<':') gsmdebug(ch);
		}
		gsmdebug("\r\n");
	}
	else{
		gsmdebug("gprs not open.\r\n");
	}
	return ret;
}
	
int SIM900GPRS::httpget(const char *url, const char *path, char *response, int len, bool endGPRS)
{
	int count = 0;
	uint32_t prev_millis;
	
	gsmdebug("httpget: start...\r\n");
	
	if(!url || !response || len<1) return 0;
	
	if( !openGPRS() ) return 0;
	gsmdebug("httpget: gprs opened\r\n");
	
	if(!sendATcmd("AT+HTTPINIT", "OK", 1000, 5)) goto end_httpget;
	gsmdebug("httpget: http init ok\r\n");
	delay(100);
	
	if(!sendATcmd("AT+HTTPPARA=\"CID\",1", "OK", 500, 5)) goto end_httpget;
	gsmdebug("httpget: set CID ok\r\n");
	delay(50);
	
	/* set URL */
	m_ser->print("AT+HTTPPARA=\"URL\",\"");
	m_ser->print(url);
	if(path) m_ser->print(path);
	if(!sendATcmd("\"", "OK", 1000, 1)) goto end_httpget;
	gsmdebug("httpget: set URL = \"");
	gsmdebug(url);
	if(path) gsmdebug(path);
	gsmdebug("\" ok\r\n");
	delay(50);
	
	/* HTTP GET */
	if(!sendATcmd("AT+HTTPACTION=0", "OK", 5000, 1)) goto end_httpget;
	delay(5); m_ser->flushRx();
	prev_millis = millis();
	while( !m_ser->isrx() && (millis()-prev_millis<30000)); // wait for a character
	if( waitResponse("+HTTPACTION:0,200,", 500) < 0 ) goto end_httpget;
	gsmdebug("httpget: send GET command ok\r\n");
	delay(50);
	
	m_ser->print("AT+HTTPREAD=0,%d", len);
	if(!sendATcmd("", "+HTTPREAD:", 5000, 1)) goto end_httpget;
	gsmdebug("httpget: start read command ok\r\n");
	
	if( waitResponse("\r\n", 500) < 0 ) goto end_httpget;
	gsmdebug("httpget: reading response...\r\n");
	delay(50);
	
	while( count<len && m_ser->isrx() )
	{
		*response++ = (char)m_ser->getc();
		count++;
		if(!m_ser->isrx()) delay(20);
	}
	
	gsmdebug("httpget: got %d bytes\r\n", count);

end_httpget:
	sendATcmd("AT+HTTPTERM", "OK", 500, 2);
	if(endGPRS) closeGPRS();
	
	return count;
}
