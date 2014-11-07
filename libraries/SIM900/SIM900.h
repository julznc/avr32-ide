
#ifndef SIM900_H
#define SIM900_H

#include <avr32.h>

enum NETWORK
{
	NET_SMART=1,
	NET_SMART_BRO,
	NET_SUN_POSTPAID,
	NET_SUN_PREPAID,
	NET_GLOBE_POSTPAID,
	NET_GLOBE_PREPAID,
	NET_PLDT_WEROAM
};

// access point names
#define APN_SMART               "internet"
#define APN_SMART_BRO           "smartbro"
#define APN_SUN_POSTPAID        "fbband"
#define APN_SUN_PREPAID         "minternet"
#define APN_GLOBE_POSTPAID      "internet.globe.com.ph"
#define APN_GLOBE_PREPAID       "http.globe.com.ph"
#define APN_PLDT_WEROAM         "WEROAMPLAN"


#define DEFAULT_SIM900_BAUD         19200
#define DEFAULT_SIM900_TIMEOUT      500
#define DEFAULT_SIM900_RETRY        20
#define SIM900_DEBUG

#ifdef SIM900_DEBUG
  #define gsmdebug(str, args...)        Serial.print(str, ##args)
#else
  #define gsmdebug(str, args...)        do{ }while(0)
#endif

class SIM900
{
public:
	SIM900();
	bool begin(HardwareUart *serial=&Serial0, uint8_t network=NET_SMART, int pwron=-1);
	bool powerOn();
	bool powerOff();
	
	bool sendATcmd(const char *cmd="AT", const char *resp=NULL /*expected response*/,
					uint32_t timeout=DEFAULT_SIM900_TIMEOUT,
					uint8_t retry=DEFAULT_SIM900_RETRY,
					bool clearRX=true);
	int32_t portReceiveRaw( const char *str_to_wait = NULL, uint16_t timeout = DEFAULT_SIM900_TIMEOUT, bool bInt = false );
	#define portFlushRX()			portReceiveRaw( NULL, 200, false )
	#define getIntegerResponse()	portReceiveRaw( NULL, 1000, true )
	#define waitResponse(rsp, to)	portReceiveRaw( rsp, to, false )
	
	size_t readBytes( char *buff, size_t max_len);
	size_t readBytesUntil( char ch, char *buff, size_t max_len);
	
protected:
	HardwareUart *m_ser; // serial port
	uint8_t m_net; // enum NETWORK
	int m_pwron; // power on pin
	
	#define SIM900_BUFF_SIZE    1024 //256
	uint8_t m_buff[SIM900_BUFF_SIZE]; // temporary buffer
};

class SIM900GSM : public SIM900
{
public:
	enum message_status{
		REC_UNREAD = 1,
		REC_READ = 2,
		STO_UNSENT = 3,
		STO_SENT = 4,
		MSG_ALL = 5
	};
	enum delete_flag{
		DEL_INDEX = 0,
		DEL_ALL_READ = 1,
		DEL_ALL = 4
	};
	
	SIM900GSM();
	virtual bool begin(HardwareUart *serial=&Serial0, uint8_t network=NET_SMART, int pwron=-1);
	
	// SMS functions
	bool sendSMS( const char *number, const char *message );
	bool deleteSMS( int16_t index = 1, delete_flag flag = DEL_INDEX );
	bool getSMS( int16_t index, char *msg_txt, size_t msg_len, char *sender = NULL, char *timestamp = NULL );
	int16_t hasSMS( message_status status = MSG_ALL );
		
};

class SIM900GPRS : public SIM900
{
public:
	SIM900GPRS();
	virtual bool begin(HardwareUart *serial=&Serial0, uint8_t network=NET_SMART, int pwron=-1);
	
	bool configGPRS(uint8_t network=NET_SMART);
	bool openGPRS();
	bool closeGPRS();
	bool isOpenedGPRS();
	
	int httpget(const char *url, const char *path=NULL, char *response=NULL, int len=0, bool endGPRS=false);
};

#endif // SIM900_H
