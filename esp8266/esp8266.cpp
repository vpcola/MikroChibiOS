#include "esp8266.h"
#include "ch.hpp"
#include "hal.h"
#include "chprintf.h"
#include "shellutils.h"

#include <stdlib.h>
#include <string.h>


static BaseSequentialStream * usart = NULL;
static BaseSequentialStream * dbgstrm = NULL;

#define RXBUFF_SIZ 2048
#define TXBUFF_SIZ 2048

#define READ_TIMEOUT 1000 // uart read timeout on 1000 ticks
#define WRITE_TIMEOUT 1000 // uart write timeout on 1000 ticks
// General purpose buffer for reading results
static char rxbuff[RXBUFF_SIZ] = {0};
static char txbuff[TXBUFF_SIZ] = {0};

#define FW_VERSION_STR_SIZ 100
static char firmwareVersionStr[FW_VERSION_STR_SIZ];
#define IP_STR_SIZ 100
static char assignedIP[IP_STR_SIZ] = {0};

typedef enum {
    WIFI_RESET = 0,
    WIFI_INITIALIZED,
    WIFI_AP_CONNECTED
} WIFI_STATUS;

static WIFI_STATUS espStatus = WIFI_RESET;


SerialDriver * getSerialDriver(void)
{
  return (SerialDriver *) usart;
}


bool esp8266HasData(void)
{
    if (usart)
    {
        // If get would block, then there's currently
        // no data in the input queue.
        return !sdGetWouldBlock((SerialDriver *) usart); 
    }
    return false;
}
// Reads the 8266 until the matched string
// is read
bool esp8266ReadUntil(const char * resp, int timeout)
{
    int c;
    int index = 0;
    int targetLength = strlen(resp);

    while((c = sdGetTimeout((SerialDriver *) usart, timeout)) >= 0)
    {
        DBG("%c", c);
        if (c != resp[index])
              index = 0;

        if (c == resp[index]){
           if(++index >= targetLength){
                return true;
           }
        }
    };

    return false;
}

int esp8266ReadSwitch(const char * resp1, const char * resp2, const char * resp3, int timeout)
{
  int c;
  int index1 = 0, index2 = 0, index3 = 0;
  int target1 = 0, target2 = 0, target3 = 0;

  target1 = strlen(resp1);
  target2 = strlen(resp2);
  target3 = strlen(resp3);

  while((c = sdGetTimeout((SerialDriver *) usart, timeout)) > 0)
  {
      DBG("%c", c);
      if (c != resp1[index1])  index1 = 0;
      if (c == resp1[index1])
        if(++index1 >= target1) return 0;

      if (c != resp2[index2])  index2 = 0;
      if (c == resp2[index2])
        if(++index2 >= target2) return 1;

      if (c != resp3[index3])  index3 = 0;
      if (c == resp3[index3])
        if(++index3 >= target3) return 2;
  }

  return -1; // none matched or timed out
}

int esp8266ReadBuffUntil(char * buffer, int len, const char * resp)
{
    int c;
    int index = 0;
    int numread = 0;
    int targetLength = strlen(resp);

    memset(buffer, 0, len);
    do
    {
        c = sdGet((SerialDriver *) usart);
        if (c >= 0) {
            DBG("%c", c);
            if (numread < len)
            {
              buffer[numread] = c;
              numread ++;
            }

            if (c != resp[index])
                index = 0;

            if (c == resp[index]){
                if(++index >= targetLength){
                    // buffer[numread] = 0;
                    return numread;
                }
            }
        }
    }while((c >= 0) && (numread < len));

    return numread;
}

int esp8266ReadLinesUntil(char * resp, responselinehandler handler)
{
  int numlines = 0, numread = 0;
  char linebuff[200];

  while((numread = esp8266ReadBuffUntil(linebuff, 200, "\r\n")) > 0)
  {
    // If we found our line
    if (strstr(linebuff, resp) != NULL)
      return numlines;

    strip(linebuff, '\r');
    if (handler) handler(linebuff, strlen(linebuff));
    numlines++;
  }

  return numlines;
}

bool esp8266Cmd(const char * cmd, const char * rsp, int cmddelay)
{
    if (usart)
    {
        // Send the command to the wifi chip 
        chprintf(usart, "%s\r\n", cmd);
        DBG(">>%s\r\n", cmd);
        if (cmddelay > 0)
            chThdSleepMilliseconds(cmddelay);

        return esp8266ReadUntil(rsp, READ_TIMEOUT);
    }else
        return false;

}

bool esp8266CmdX(const char * cmd, const char * rsp)
{
    if (usart)
    {
        // Send the command to the wifi chip
        chprintf(usart, "%s\r\n", cmd);
        DBG(">>%s\r\n", cmd);

        return esp8266ReadUntil(rsp, TIME_INFINITE);
    }else
        return false;

}
/*
bool esp8266Dta(const char * cmd, const char * rsp, int cmddelay)
{
    if (usart)
    {
        // Send the command to the wifi chip
        chprintf(usart, "%s", cmd);
        //DBG(">>%s\r\n", cmd);
        if (cmddelay > 0)
            chThdSleepMilliseconds(cmddelay);

        return esp8266ReadUntil(rsp, READ_TIMEOUT);
    }else
        return false;

}
*/
int esp8266CmdCallback(const char *cmd, const char * rsp, responselinehandler handler)
{
  chprintf(usart, "%s\r\n", cmd);
  DBG(">>%s\r\n", cmd);
  return esp8266ReadLinesUntil((char *) rsp, handler);
}

int esp8266CmdRsp(const char * cmd, const char * term, char * buffer, int buflen, int respline)
{
    int numread = 0;
    if (usart)
    {
        // Send the command to the wifi chip 
        chprintf(usart, "%s\r\n", cmd);
        DBG(">>%s\r\n", cmd);
        // Read multi-line response, until the response line
        // we wish to get.
        for (int i = 0; i < respline; i++)
            esp8266ReadUntil("\r\n", READ_TIMEOUT);
        // Read the response
        numread = esp8266ReadBuffUntil(buffer, buflen, "\r\n" );
        // continue reading until terminating string
        esp8266ReadUntil(term, READ_TIMEOUT);

        return numread;
    }else
        return 0;
}

static void onAddAP(APInfo * info)
{
  if (dbgstrm)
  chprintf(dbgstrm, "SSID[%s] signal[%d] ecn[%d] MAC[%s]\r\n",
      info->ssid,
      info->strength,
      info->ecn,
      info->macaddr);
}

int esp8266Init(SerialDriver * driver, int mode, SerialDriver * dbg)
{
    // Set the sequential stream (usart) 
    usart = (BaseSequentialStream *) driver;
    // Set the debug channel
    dbgstrm = (BaseSequentialStream *) dbg;

    //chMtxInit(&usartmtx);

    if (!esp8266CmdX("AT+RST\r\n", "ready\r\n"))
    {
        chprintf(dbgstrm, "Failed to reset ESP8266!\r\n");
        return WIFI_ERR_RESET;
    }else
        chprintf(dbgstrm, "ESP8266 Initialized\r\n");

    espStatus = WIFI_INITIALIZED;

    chprintf(dbgstrm, "ESP8266 Firmware Version [%s]\r\n", esp8266GetFirmwareVersion());

    if(esp8266SetMode(mode))
      chprintf(dbgstrm, "ESP8266 Mode set to %d\r\n", mode);

    esp8266ListAP(onAddAP);

    return WIFI_ERR_NONE;
}


int esp8266ListAP(onNewAP apCallback)
{
  int numlines = 0, numread = 0;
  char line[200], *p;
  APInfo apinfo;

  chprintf(dbgstrm, "ESP8266 Listing APs...\r\n");

  chprintf(usart, "AT+CWLAP\r\n");
  DBG(">>AT+CWLAP\r\n");
  while((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, "\r\n")) > 0)
  {
    // If we found our line
    if (strstr(rxbuff, "OK\r\n") != NULL)
      return numlines;

    strip(rxbuff, '\r');
    strip(rxbuff, ')' );
    if (strstr(rxbuff, "+CWLAP:(") != NULL)
    {
        strcpy(line, (char *) rxbuff + 8);
        //DBG("AP [%s]\r\n", line);
        p = strtok(line, ",");
        if (p) apinfo.ecn = atoi(p); // ecn
        p = strtok(NULL, ",");
        if (p) strcpy(apinfo.ssid, p);
        p = strtok(NULL, ",");
        if (p) apinfo.strength = atoi(p);
        p = strtok(NULL, ",");
        if (p) strcpy(apinfo.macaddr, p);
        // Don't know what the last param
        // is, it doesn't seem to be documented
        // anywhere

        // Call the callback
        if(apCallback) apCallback(&apinfo);
    }
    numlines++;
  }

  return numlines;
}

int esp8266ConnectAP(const char *ssid, const char *password)
{
  if(dbgstrm) chprintf(dbgstrm, "ESP8266 Joining AP (%s) ... ", ssid);

  chsnprintf(txbuff, TXBUFF_SIZ,"AT+CWJAP=\"%s\",\"%s\"\r\n",
    ssid,
    password);

  if (esp8266Cmd(txbuff, "OK\r\n", 5000))
  {
    espStatus = WIFI_AP_CONNECTED;
    if (dbgstrm) chprintf(dbgstrm, "Success!\r\n");
  }
  else
  {
    if (dbgstrm) chprintf(dbgstrm, "Failed!\r\n");
    return WIFI_ERR_JOIN;
  }

  if (dbgstrm) chprintf(dbgstrm, "Assigned IP [%s]\r\n", esp8266GetIPAddress());


  if (!esp8266Cmd("AT+CIPMUX=1\r\n", "OK\r\n", 1000))
  {
      if (dbgstrm) chprintf(dbgstrm, "ESP8266 Setting MUX Failed!!\r\n");
      return WIFI_ERR_MUX;
  }

  return WIFI_ERR_NONE;
}

bool esp8266DisconnectAP(void)
{
  return esp8266Cmd("AT+CWQAP\r\n", "OK\r\n", 1000);
}

bool esp8266SetMode(int mode)
{
  chsnprintf(txbuff, 200, "AT+CWMODE=%d\r\n", mode);
  return esp8266Cmd(txbuff, "OK\r\n", 1000);
}

static void ongetfirmwareversion(const char * buffer, int len)
{
  char tmpbuf[50];
  strncpy(tmpbuf, buffer, len);
  // Compare only the first 2 numbers, usually "00"
  if(strncmp((const char *) tmpbuf, "00", 2) == 0)
    strcpy(firmwareVersionStr, strip(tmpbuf, '\r'));
}

const char * esp8266GetFirmwareVersion(void)
{
  memset(firmwareVersionStr, 0, FW_VERSION_STR_SIZ);
  esp8266CmdCallback("AT+GMR\r\n", "OK\r\n", ongetfirmwareversion);
  return firmwareVersionStr;
}

const char * esp8266GetIPAddress(void)
{
  if (strlen(assignedIP) > 0 ) return assignedIP;

  if (esp8266CmdRsp("AT+CIFSR\r\n", "OK\r\n", rxbuff, RXBUFF_SIZ, 2) > 0)
  {
    strcpy(assignedIP, strip(rxbuff, '\r'));
    return (const char *) assignedIP;
  }

  return NULL;
}


int esp8266GetIpStatus(onIPStatus handler)
{
  int numlines = 0, numread = 0;
  int status = WIFI_CONN_UNKNWN, chanid;
  IPStatus ipstatus;

  for (int i = 0; i < ESP8266_MAX_CONNECTIONS; i++)
    ipstatus.status[i] = WIFI_CONN_DISCONNECTED;

  chprintf(usart, "AT+CIPSTATUS\r\n");
  DBG(">>AT+CIPSTATUS\r\n");
  while((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, "\r\n")) > 0)
  {
    // If we found our line
    if (strstr(rxbuff, "OK\r\n") != NULL)
      break;

    strip(rxbuff, '\r');

    // Since MUX=1, we generally ignore the line
    // STATUS: <x> since we are after for the status
    // of the connection.
    // If the connection id is listed, then
    // we have an open connection.
    DBG("IPSTAT [%s]\r\n", rxbuff);

    if (strncmp(rxbuff, "STATUS:", 7) == 0)
      status = atoi(rxbuff + 7);

    if (strstr(rxbuff, "+CIPSTATUS:") != NULL)
    {
      chanid = atoi(rxbuff + 11);
      if ((chanid >= 0) && (chanid < ESP8266_MAX_CONNECTIONS))
      {
        DBG("Status = %d, Detected channel = %d\r\n", status, chanid);
        ipstatus.status[chanid] = status;
      }else
      {
        DBG("Channel out of range\r\n");
      }
    }
    numlines++;
  }

  if(handler) handler(&ipstatus);

  return status;
}

bool esp8266Connect(int channel, const char * ip, uint16_t port, int type)
{
    // Simply send the data over the channel.
    chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n",
        channel,
        type == TCP ? "TCP":"UDP",
        ip,
        port);

    return esp8266Cmd(txbuff, "Linked\r\n", 1000);
    //return esp8266Cmd(txbuff, "OK\r\n", 100);
}

bool esp8266Disconnect(int channel)
{
  chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPCLOSE=%d\r\n", channel);
  return esp8266Cmd(txbuff, "OK\r\n", 100);
}

bool esp8266SendLine(int channel, const char * str)
{
  int datatosend = 0;

  if (str)
    datatosend = strlen(str) + 2;
  else
    datatosend = 2; // \r\n (empty lines)

  chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPSEND=%d,%d\r\n",
             channel,
             datatosend);
  // Wait untill the prompt
  if (esp8266Cmd(txbuff, ">", 0))
  {
    DBG("\r\n>>Got the prompt! Sending rest of data!\r\n");
    if (str) chprintf(usart, "%s\r\n", str);
    else chprintf(usart, "\r\n");

    return esp8266ReadUntil("SEND OK\r\n", READ_TIMEOUT);
  }

  return false;
}

bool esp8266SendHeader(int channel, int datatosend)
{
  chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPSEND=%d,%d\r\n",
             channel,
             datatosend);

  // Wait untill the prompt
  if (esp8266Cmd(txbuff, ">", 1000))
  {
    DBG(">>Got the command prompt! ... send the rest of the data!\r\n");
    return true;
  }

  return false;
}

int esp8266Send(const char * data, int len)
{
    int numsent = 0;
    //numsent = sdWriteTimeout((SerialDriver *)usart, (uint8_t *) data, len, WRITE_TIMEOUT);
    do {
      sdPut((SerialDriver *) usart, data[numsent]);
      numsent++;
    }while(numsent < len);

#ifdef DEBUG
    if (numsent > 0)
    {
      DBG("\r\n>>Writtten %d bytes ...\r\n", numsent);
      hexdump(dbgstrm, (void *) data, numsent);
    }
#endif

    if (!esp8266ReadUntil("SEND OK\r\n", READ_TIMEOUT))
        numsent = -1;

    return numsent;
}

int esp8266ReadRespHeader(int * channel, int * status, int timeout)
{
  char *p = NULL;
  int bytestoread = 0, numread = 0;

  // Discard data until we receive part of the header
  DBG(">>Waiting for message header ...\r\n");
  // Read loop until we have a status
  *status = esp8266ReadSwitch("Unlink\r\n", "OK\r\n", "+IPD,", timeout);
  DBG(">>Status = %d\r\n", *status);
  if(*status == 2)
  {
      DBG(">>Read the +IPD, reading message length and channel ..\r\n");
      // Read header information (up until the ":")
      memset(rxbuff, 0, RXBUFF_SIZ);
      if ((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, ":")) > 0)
      {
          // Parse header information for
          // Channel and number of bytes
          p = strtok(rxbuff, ",");
          if (p) *channel = atoi(p);
          p = strtok(NULL, ",");
          if (p) bytestoread = atoi(p);
          DBG(">>Read channel = %d, bytestoread = %d\r\n", *channel, bytestoread);
      }
  }

  return bytestoread;
}

int esp8266Read(char * buffer, int bytestoread)
{
  int numread = 0;
  int c;

  do {
    c = sdGet((SerialDriver *) usart);
    if (c >= 0)
    {
        DBG("%c", c );
        buffer[numread] = c; 
        numread ++;
    }
  }while(numread < bytestoread);

#ifdef DEBUG
  if (numread > 0)
  {
    DBG("\r\n>>Read %d bytes ... dumping data\r\n", numread);
    hexdump(dbgstrm, buffer, numread);
  }
#endif

  return numread;
}



