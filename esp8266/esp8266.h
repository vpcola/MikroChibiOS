#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "hal.h"

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Wed Feb 04 2015
 **/
#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG 1

#ifdef DEBUG
#define DBG(X, ...)    if(dbgstrm) chprintf(dbgstrm, X, ##__VA_ARGS__ )
#else
#define DBG(X, ...)
#endif


enum wifiErrors {
    WIFI_ERR_NONE = 0,
    WIFI_ERR_AT,
    WIFI_ERR_RESET,
    WIFI_ERR_JOIN,
    WIFI_ERR_MUX,
    WIFI_ERR_MODE,
    WIFI_ERR_CONNECT,
    WIFI_ERR_LINK  
};

enum wifiConRequest {
    WIFI_DISCONNECT = 0,
    WIFI_CONNECT
};

enum wifiConStatus {
    WIFI_CONN_UNKNWN = 1,
    WIFI_CONN_GOTIP,        // 2
    WIFI_CONN_CONNECTED,    // 3
    WIFI_CONN_DISCONNECTED, // 4
};

enum esp8266ConnectionType
{
    TCP = 0,
    UDP
};

// My current esp8266 chip returns
// something like:
// +CWLAP:(3,"SSID",-55,"00:02:6f:d9:9d:18",4
// for each AP found.
#define SSIDMAX_SIZ 50
#define MACADDR_SIZ 20
typedef struct {
  int ecn;
  char ssid[SSIDMAX_SIZ];
  int strength;
  char macaddr[MACADDR_SIZ];
  int unknown;
} APInfo;

#define ESP8266_MAX_CONNECTIONS 5
typedef struct {
  int status[ESP8266_MAX_CONNECTIONS];
} IPStatus;



typedef enum {
  RET_INVAL         = 0x00000000,
  RET_NONE          = 0x00000001,
  RET_OK            = 0x00000010,
  RET_READY         = 0x00000100,
  RET_LINKED        = 0x00001000,
  RET_SENT          = 0x00010000,
  RET_UNLINK        = 0x00100000,
  RET_ERROR         = 0x01000000,
  RET_ALREADY_CONNECTED = 0x10000000
} EspRetVal;

typedef struct {
  EspRetVal retval;
  char retstr[100];
} EspReturn;

SerialDriver * getSerialDriver(void);

// Right now, there can be one user of
// the 8266 at any point of time.
// I still have to find a way to multiplex
// the usage (specially the read part) of the
// 8266 on multiple threads.
//void esp8266Lock(void);
//void esp8266Unlock(void);

// Checks the usart buffer if data
// is present
bool esp8266HasData(void);
// Read esp8266 untill a certain string is received.
// Read (discard characters read), until resp is read
bool esp8266ReadUntil(const char * resp, int timeout);
// 2 types of terminating string
int esp8266ReadSwitch2(const char * resp1, const char * resp2, int timeout);
// 3 types of terminating string
int esp8266ReadSwitch3(const char * resp1, const char * resp2, const char * resp3, int timeout);
// Read to buffer/len until resp is read (resp will be
// included in the buffer). Returns number of bytes read
// or negative if len can not hold the number of bytes
// needed to be read.
int esp8266ReadBuffUntil(char * buffer, int len, const char * resp);
// A variant of the same function, but takes a function
// pointer which will be called for each line encountered.
typedef void (*responselinehandler)(const char * data, int len);
int esp8266ReadLinesUntil(char * resp, responselinehandler handler);

// Request - Response communication
// to the esp8266 chip
bool esp8266Cmd(const char * cmd, const char * rsp, int cmddelay);
int esp8266CmdCallback(const char *cmd, const char * rsp, responselinehandler handler);

// variation of the above, without the \r\n
// bool esp8266Dta(const char * cmd, const char * rsp, int cmddelay);
int esp8266CmdRsp(const char * cmd, const char * term, char * buffer, int buflen, int respline);


// ESP8266 Routines
int esp8266Init(SerialDriver * driver, int mode, SerialDriver * dbg);
int esp8266ConnectAP(const char *ssid, const char *password);
bool esp8266DisconnectAP(void);
typedef void (*onNewAP)(APInfo * info);
int esp8266ListAP(onNewAP apCallback);
// Get the status of the connection
typedef void (*onIPStatus)(IPStatus * info);
int esp8266GetIpStatus(onIPStatus handler);
//int esp8266GetIpStatus(int channel);
const char * esp8266GetFirmwareVersion(void);
const char * esp8266GetIPAddress(void);
bool esp8266SetMode(int mode);

// Client API
bool esp8266Connect(int channel, const char * ip, uint16_t port, int type);
bool esp8266SendLine(int channel, const char * str);

bool esp8266SendHeader(int channel, int datatosend);
int esp8266Send(const char * data, int len);

int esp8266ReadRespHeader(int * channel, int * status, int timeout);
int esp8266Read(char * buffer, int buflen);

bool esp8266Disconnect(int channel);


#ifdef __cplusplus
}
#endif


#endif

