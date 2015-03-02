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
#define TXBUFF_SIZ 200

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

static const EspReturn returnValues[] = {
   { RET_ALREADY_CONNECTED, "LINK IS BUILDED\r\n" },
   { RET_NOCHANGE,          "no change\r\n" },
   { RET_SENT,              "SEND OK\r\n" },
   { RET_ERROR,             "ERROR\r\n" },
   { RET_UNLINK,            "Unlink\r\n" },
   { RET_LINKED,            "Linked\r\n" },
   { RET_CLOSED,            ",CLOSED\r\n" },
   { RET_CONNECT,           ",CONNECT\r\n" },
   { RET_READY,             "ready\r\n" },
   { RET_IPD,               "+IPD,"  },
   { RET_OK,                "OK\r\n" },
   { RET_NONE,              "\r\n" },
};

#define NUM_RESPONSES sizeof(returnValues)/sizeof(EspReturn)

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

#if 0
static int esp8266ReadSwitch2(const char * resp1, const char * resp2, int timeout)
{
  int c;
  int index1 = 0, index2 = 0;
  int target1 = 0, target2 = 0;

  target1 = strlen(resp1);
  target2 = strlen(resp2);

  while((c = sdGetTimeout((SerialDriver *) usart, timeout)) > 0)
  {
      DBG("%c", c);
      if (c != resp1[index1])  index1 = 0;
      if (c == resp1[index1])
        if(++index1 >= target1) return 0;

      if (c != resp2[index2])  index2 = 0;
      if (c == resp2[index2])
        if(++index2 >= target2) return 1;
  }

  return -1; // none matched or timed out
}
static int esp8266ReadSwitch3(const char * resp1, const char * resp2, const char * resp3, int timeout)
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
#endif

static int esp8266ReadSwitch(int retvals, char * buffer, int * bufsiz, int timeout)
{
  int index[NUM_RESPONSES];
  int lens[NUM_RESPONSES];
  int numstored, i, x, c, numlen;

  if (!buffer || !bufsiz) return -1;

  numlen = NUM_RESPONSES;
  for (i = 0; i < numlen; i++)
  {
    index[i] = 0;
    lens[i] = strlen(returnValues[i].retstr);
  }

  numstored = 0;
  while((c = sdGetTimeout((SerialDriver *) usart, timeout)) > 0)
  {
     if (numstored < *bufsiz)
     {
       buffer[numstored] = c;
       numstored++;
       DBG("%c", c);

       // Evaluate if this belongs to the
       // list of return values
       for(x = 0; x < numlen; x++)
       {
         if (c != returnValues[x].retstr[index[x]])  index[x] = 0;
         if (c == returnValues[x].retstr[index[x]])
           if(++(index[x]) >= lens[x])
           {
             if (retvals & returnValues[x].retval)
             {
               buffer[numstored] = 0;
               *bufsiz = numstored;
               DBG("\r\n**Got [%s], returning %d\r\n",
                   buffer, returnValues[x].retval);
               return returnValues[x].retval;
             }
           }
       }

       // Reset the buffer on a newline or linefeed
       if ((numstored >= 2) &&
           (buffer[numstored-2] == '\r') &&
           (buffer[numstored-1] == '\n')
         )
           numstored = 0;

     }else
       break;
  }

  buffer[numstored] = 0;
  *bufsiz = numstored;
  DBG("\r\n**Got [%s], returning with -1\r\n", buffer);
  return -1;
}

int esp8266ReadBuffUntil(char * buffer, int len, const char * resp, int timeout)
{
    int c;
    int index = 0;
    int numread = 0;
    int targetLength = strlen(resp);

    memset(buffer, 0, len);
    do
    {
        c = sdGetTimeout((SerialDriver *) usart, timeout);
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

  while((numread = esp8266ReadBuffUntil(linebuff, 200, "\r\n", READ_TIMEOUT)) > 0)
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

static int esp8266CmdX(const char * cmd, int returns, int cmddelay, int timeout)
{
  int bufsiz = RXBUFF_SIZ;
  if (usart)
  {
      // Send the command to the wifi chip
      chprintf(usart, "%s\r\n", cmd);
      DBG(">>%s\r\n", cmd);
      if (cmddelay > 0)
          chThdSleepMilliseconds(cmddelay);

      return esp8266ReadSwitch(returns, rxbuff, &bufsiz, timeout);
  }
  return -1;
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
        numread = esp8266ReadBuffUntil(buffer, buflen, "\r\n", READ_TIMEOUT);
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

    if (!esp8266Cmd("AT+RST\r\n", "ready\r\n", 1000))
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
  while((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, "\r\n", TIME_INFINITE)) > 0)
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

  if (esp8266CmdX(txbuff, RET_OK, 5000, TIME_INFINITE) > 0)
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


//  if (esp8266CmdX("AT+CIPMODE=1\r\n", RET_OK | RET_ERROR, 100, TIME_INFINITE) != RET_OK)
//  {
//     if(dbgstrm) chprintf(dbgstrm, "ESP8266 Setting CIPMODE Failed!!\r\n");
//     return WIFI_ERR_MUX;
//  }


  if (esp8266CmdX("AT+CIPMUX=1\r\n", RET_OK | RET_ERROR, 100, TIME_INFINITE) != RET_OK)
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
  return (esp8266CmdX(txbuff, RET_OK | RET_NOCHANGE, 100, TIME_INFINITE) == RET_OK );
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
  int numread, numline = 0;
  //char temp[100], * loc;

  chprintf(usart, "AT+CIFSR\r\n");
  DBG(">>AT+CIFSR\r\n");

  assignedIP[0] = 0;
  // Parse all response lines
  while((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, "\r\n", READ_TIMEOUT)) > 0)
  {
    // If we found our line
    if (strstr(rxbuff, "OK\r\n") != NULL)
      break;

    strip(rxbuff, '\r');
    DBG("CIFSR [%s]\r\n", rxbuff);

    //if ((loc = strstr(rxbuff, "+CIFSR:STAIP,\"")) != NULL)
    if (numline == 2)
    {
      // store the station ip
      //strcpy(temp, loc + 14);
      //strip(temp, '\r');
      strcpy(assignedIP, rxbuff);
    }

    numline++;
  }

  if (strlen(assignedIP) == 0) return NULL;

  return (const char *) assignedIP;
}


int esp8266GetIpStatus(onIPStatus iphandler, onConStatus stathandler)
{
  int numlines = 0, numread = 0;
  int status = WIFI_CONN_UNKNWN, chanid = -1;
  IPStatus ipstatus;
  char * p, tmp[100];

  for (int i = 0; i < ESP8266_MAX_CONNECTIONS; i++)
    ipstatus.status[i] = WIFI_CONN_DISCONNECTED;

  chprintf(usart, "AT+CIPSTATUS\r\n");
  DBG(">>AT+CIPSTATUS\r\n");
  while((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, "\r\n", READ_TIMEOUT)) > 0)
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
      p = strtok(rxbuff + 11, ",");
      if (p) chanid = atoi(p);

      if ((chanid >= 0) && (chanid < ESP8266_MAX_CONNECTIONS))
      {
        ConStatus constatus;

        constatus.id = chanid;
        p = strtok(NULL, ",");
        if (p)
        {
          strcpy(tmp, p);
          constatus.type = (strstr(tmp, "TCP") != NULL) ? TCP: UDP;
        }
        p = strtok(NULL, ",");
        if (p)
        {
          strcpy(tmp, p+1);
          strip(tmp, '\"');
          strcpy(constatus.srcaddress, tmp);
        }
        p = strtok(NULL, ",");
        if (p)
        {
          constatus.port = atoi(p);
        }
        p = strtok(NULL, ",");
        if (p)
        {
          constatus.clisrv = atoi(p);
        }

        if(stathandler) stathandler(&constatus);

        DBG("Status = %d, Detected channel = %d\r\n", status, chanid);
        ipstatus.status[chanid] = status;
      }else
      {
        DBG("Channel out of range\r\n");
      }
    }
    numlines++;
  }

  if(iphandler) iphandler(&ipstatus);

  return status;
}

int esp8266Server(int channel, int type, uint16_t port)
{
    int mode = 0;
    // TODO: Esp8266 firmware can not yet start a UDP
    // server, so type here is always TCP
    if (type == UDP) return -1;

    chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPSERVER=%d,%d\r\n",
        mode, port);

    return esp8266CmdX(txbuff, RET_OK|RET_ERROR, 100, TIME_INFINITE);
        
}

int esp8266Connect(int channel, const char * ip, uint16_t port, int type)
{
    // Simply send the data over the channel.
    chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n",
        channel,
        type == TCP ? "TCP":"UDP",
        ip,
        port);

    // For my particular firmware AT+CIPSTART returns "LINKED" and
    // "UNLINK"
    return esp8266CmdX(txbuff, RET_LINKED | RET_UNLINK | RET_ERROR , 100, TIME_INFINITE);
}

bool esp8266Disconnect(int channel)
{
  chsnprintf(txbuff, TXBUFF_SIZ, "AT+CIPCLOSE=%d\r\n", channel);
  return (esp8266CmdX(txbuff, RET_OK|RET_ERROR, 100, TIME_INFINITE) == RET_OK);
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
    int numsent = 0, bufsiz;

    do {
      //retval = sdPutTimeout((SerialDriver *) usart, data[numsent], WRITE_TIMEOUT);
      //if (retval < 0) break;
      if (sdPut((SerialDriver *)usart, data[numsent]) < 0)
        break;
      numsent++;
    }while(numsent < len);



#ifdef DEBUG
    if (numsent > 0)
    {
      DBG("\r\n>>Writtten %d bytes ...\r\n", numsent);
      hexdump(dbgstrm, (void *) data, numsent);
    }
#endif

   // while(esp8266ReadUntil("SEND OK\r\n", READ_TIMEOUT) < 1);
   bufsiz = RXBUFF_SIZ;
   esp8266ReadSwitch(RET_SENT, rxbuff, &bufsiz, TIME_INFINITE);

   return numsent;
}

int esp8266ReadRespHeader(int * channel, int * param, int timeout)
{
  char *p = NULL;
  int numread = 0, bufsiz = RXBUFF_SIZ, retval;

  // Discard data until we receive part of the header
  DBG(">>Waiting for message header ...\r\n");
  // Read loop until we have a status
  retval = esp8266ReadSwitch(RET_ERROR |
                             RET_UNLINK |
                             RET_LINKED |
                             RET_CONNECT |
                             RET_CLOSED |
                             RET_IPD, rxbuff, &bufsiz, timeout);
  DBG(">>Retval = %d\r\n", retval);
  if(retval == RET_IPD)
  {
      DBG(">>Read the +IPD, reading message length and channel ..\r\n");
      // Read header information (up until the ":")
      memset(rxbuff, 0, RXBUFF_SIZ);
      if ((numread = esp8266ReadBuffUntil(rxbuff, RXBUFF_SIZ, ":", READ_TIMEOUT)) > 0)
      {
          // Parse header information for
          // Channel and number of bytes
          p = strtok(rxbuff, ",");
          if (p) *channel = atoi(p);
          p = strtok(NULL, ",");
          if (p) *param = atoi(p);
          DBG(">>Read channel = %d, bytestoread = %d\r\n", *channel, *param);
      }
  }

  if ((retval == RET_CONNECT) ||
      (retval == RET_CLOSED))
  {
      p = strtok(rxbuff, ",");
      if (p) *channel = atoi(p);
      DBG(">>Read closed/connect status on channel %d.. \r\n", *channel);
  }

  return retval;
}

int esp8266Read(char * buffer, int bytestoread)
{
  int numread = 0;
  int c;

  do {
    //c = sdGetTimeout((SerialDriver *) usart, READ_TIMEOUT);
    c = sdGet((SerialDriver *) usart);
    if (c >= 0)
    {
        DBG("%c", c );
        buffer[numread] = c; 
        numread ++;
    }else
      break;
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



