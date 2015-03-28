#ifndef __ESP8266A_H__
#define __ESP8266A_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Wed Mar 25 2015
 **/
#include "hal.h"
#include "wifidefs.h"

#include <string>
#include <list>

#define  RET_INVAL      -1
#define  RET_NONE       0x0001
#define  RET_OK         0x0002
#define  RET_READY      0x0004
#define  RET_LINKED     0x0008
#define  RET_SENT       0x0010
#define  RET_UNLINK     0x0020
#define  RET_ERROR      0x0040
#define  RET_ALREADY_CONNECTED 0x0080
#define  RET_CONNECT    0x0100
#define  RET_CLOSED     0x0200
#define  RET_IPD        0x0400
#define  RET_NOCHANGE   0x0800
#define  RET_SENTFAIL   0x1000

typedef struct _ssidinfo
{
    char ssid[SSIDMAX_LEN];
    char macaddr[MACADDRMAX_LEN];
    int  strength;
    int  ecn;
} ssidinfo;

class Esp8266;
typedef void (Esp8266::*linehandler)(char * data, int datalen);

class Esp8266
{
    public :
    Esp8266(SerialDriver * sdp, int mode, int timeout = TIMEOUT_DEFAULT );
    
    int init();
    int connectAP(const char * SSID, const char * passwd);
    
    int writechannel(int chanid, const char * data, int datalen);

    int read(int retvals, char * buffer, int * bufsiz);
    int readlines(int retvals, linehandler handler);

    const char * getipaddr() { return _ipaddr; }
    const char * getfwversion() { return _fwversion; }
    const char * getmacaddr() { return _macaddr; }

    const std::list<ssidinfo> * getssidlist() { return &_ssids; }

    private :

    void ongmrline(char * buffer, int bufsiz);
    void onaplist(char * buffer, int bufsiz);
    void oncifsr(char * buffer, int bufsiz);


    SerialDriver * _sdp;
    int _mode;
    int _timeout;
    char _ipaddr[IPADDRMAX_LEN];
    char _macaddr[MACADDRMAX_LEN];
    char _fwversion[FWVERMAX_LEN];

    std::list<ssidinfo> _ssids;
};

#endif

