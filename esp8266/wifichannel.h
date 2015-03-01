#ifndef __WIFICHANNEL_H__
#define __WIFICHANNEL_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Thu Feb 12 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#include "hal.h"
#include "esp8266.h"

/**
 *  Since ESP8266 can accomodate only 4
 * different connections. We manage only
 * a static list of 4 ring buffers each
 * for receive and send
 **/
#define MAX_CONNECTIONS 5

enum wifiModes {
      WIFI_MODE_STA = 1,
      WIFI_MODE_AP,
      WIFI_MODE_APSTA
};

#define IPADDR_MAX_SIZ 100
typedef struct {
   int id; 
   int type;
   int status;
   bool ispassive; // affects how we read from the channel
   char ipaddress[IPADDR_MAX_SIZ]; // can be a string
   uint16_t port;
   char localaddress[IPADDR_MAX_SIZ];
   uint16_t localport;  // Used for binding localport to socket
   InputQueue * iqueue;
} esp_channel;

typedef enum {
    CHANNEL_UNUSED,
    CHANNEL_CONNECTED,
    CHANNEL_DISCONNECTED
} esp_channel_status;

enum {
  CMD_SEND,
  CMD_SENDTO,
  CMD_RECVFROM
};

typedef struct {
  int cmd;
  int channel;
  int numbytes;
} esp_channel_cmd;

esp_channel * getChannel(int d);

int wifiInit(int mode, SerialDriver * usart, SerialDriver * dbg);
int wifiConnectAP(const char * ssid, const char * password);

// Channel open returns an unused, empty channel
int channelOpen(int conntype);

// Send and receive
//int channelPrint(int channel, const char * format, ...);
bool channelSendLine(int channel, const char * msg);
int channelSend(int channel, const char * msg, int msglen);
int channelSendTo(int channel, const char * msg, int msglen, const char * ipaddress, uint16_t port);
int channelRead(int channel, char * buff, int msglen);
int channelGet(int channel);

// Connect the channel to tcp or udp connection
int channelConnect(int channel, const char * ipddress, uint16_t port);
bool channelIsConnected(int channel);
// Closes the channel
int channelClose(int channel);

#ifdef __cplusplus
}
#endif

#endif

