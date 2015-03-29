#include "esp8266a.h"
#include "chprintf.h"
#include "shellutils.h"
#include <string.h>
#include <stdlib.h>

typedef struct _espretval {
    int retval;
    const char * retstr;
} EspReturn;

static const EspReturn returnValues[] = {
    { RET_ALREADY_CONNECTED, "LINK IS BUILDED\r\n" },
    { RET_NOCHANGE,          "no change\r\n" },
    { RET_SENT,              "SEND OK\r\n" },
    { RET_SENTFAIL,          "SEND FAIL\r\n" },
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

// General purpose buffer for reading results
static char rxbuff[RXBUFF_SIZ]  __attribute__ ((section(".bufram")));
static char txbuff[TXBUFF_SIZ]  __attribute__ ((section(".bufram")));


int Esp8266::read(int retvals, char * buffer, int * bufsiz)
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
  while((c = sdGetTimeout((SerialDriver *) _sdp, _timeout)) > 0)
  {
     if (numstored < *bufsiz)
     {
       buffer[numstored] = c;
       numstored++;

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
  return -1;
}

int Esp8266::readlines(int retvals, linehandler handler)
{
    int index[NUM_RESPONSES];
    int lens[NUM_RESPONSES];
    int numstored, i, x, c, numlen;


    numlen = NUM_RESPONSES;
    for (i = 0; i < numlen; i++)
    {
        index[i] = 0;
        lens[i] = strlen(returnValues[i].retstr);
    }

    numstored = 0;
    while((c = sdGetTimeout((SerialDriver *) _sdp, _timeout)) > 0)
    {
        if (numstored < RXBUFF_SIZ)
        {
            rxbuff[numstored] = c;
            numstored++;

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
                            rxbuff[numstored] = 0;
                            return returnValues[x].retval;
                        }
                    }
            }

            // Reset the buffer on a newline or linefeed
            // and call the callback if a line is found
            if ((numstored >= 2) &&
                    (rxbuff[numstored-2] == '\r') &&
                    (rxbuff[numstored-1] == '\n')
               )
            {
                rxbuff[numstored] = 0; // Null terminate
                // Call the handler.
                if (handler) (this->*handler)(rxbuff, numstored);

                numstored = 0;
            }

        }else
            break;
    }

    rxbuff[numstored] = 0;
    return -1;

}

void Esp8266::ongmrline(char * buffer, int bufsiz)
{
    // Compare only the first 2 numbers, usually "00"
    if(strncmp((const char *) buffer, "00", 2) == 0)
        strncpy(_fwversion, rtrim(buffer, '\r'), bufsiz);
}

void Esp8266::onaplist(char * buffer, int bufsiz)
{
    char line[200], *p;
    rtrim(buffer, '\r');
    rtrim(buffer, ')' );
    if (strstr(buffer, "+CWLAP:(") != NULL)
    {
        ssidinfo info;
        strcpy(line, (char *) buffer + 8);
        p = strtok(line, ",");
        if (p) info.ecn = atoi(p); // ecn
        p = strtok(NULL, ",");
        if (p) strncpy(info.ssid, p, SSIDMAX_LEN);
        p = strtok(NULL, ",");
        if (p) info.strength = atoi(p);
        p = strtok(NULL, ",");
        if (p) strncpy(info.macaddr, p, MACADDRMAX_LEN);

        // add info to the list
        if (_aphandler) _aphandler(&info);
    }

}

void Esp8266::oncifsr(char * buffer, int bufsiz)
{
    char *loc, temp[200];

    rtrim(buffer, '\r');
    if ((loc = strstr(buffer, "+CIFSR:STAIP,\"")) != NULL)
    {
        // store the station ip
        strcpy(temp, loc + 14);
        rtrim(temp, '\"');
        strcpy(_ipaddr, temp);
    }
}


Esp8266::Esp8266(SerialDriver * sdp, int mode, int timeout, ssidinfohandler onAPhandler)
    :  _sdp(sdp),
    _mode(mode),
    _timeout(timeout),
    _aphandler(onAPhandler)
{
}

int Esp8266::init()
{
    int retval, numread = RXBUFF_SIZ;

    chprintf((BaseSequentialStream *) _sdp, "AT+RST\r\n");
    if ( read(RET_READY, (char *) rxbuff, &numread) == RET_READY )
    {
        chprintf((BaseSequentialStream *) _sdp, "AT+CWMODE=%d\r\n", _mode);
        retval = read(RET_OK | RET_NOCHANGE, (char *) rxbuff, &numread);
        if ( retval == RET_OK || retval == RET_NOCHANGE)
        {
            // Now retrieve the firmware version ...
            chprintf((BaseSequentialStream *) _sdp, "AT+GMR\r\n");
            if (readlines(RET_OK, &Esp8266::ongmrline) == RET_OK)
            {
                // List the AP's and save it to the map/list
                chprintf((BaseSequentialStream *) _sdp, "AT+CWLAP\r\n");
                if (readlines(RET_OK, &Esp8266::onaplist) == RET_OK)
                    return 0;
            }
        }
        
    }

    return -1;
}

int Esp8266::connectAP(const char * ssid, const char * passwd)
{
    int retval, numread = RXBUFF_SIZ;

    chsnprintf(txbuff, TXBUFF_SIZ,"AT+CWJAP=\"%s\",\"%s\"\r\n",
            ssid,
            passwd);
    chprintf((BaseSequentialStream *) _sdp, "%s", txbuff);
    retval = read(RET_OK, (char *) rxbuff, &numread);
    if (retval == RET_OK)
    {
        // Once connected, find the IP address and MAC
        chprintf((BaseSequentialStream *) _sdp, "AT+CIFSR\r\n");
        if (readlines(RET_OK, &Esp8266::oncifsr) != RET_OK)
        {
            // Post warnings here
        }
        // Set the MUX to 1
        chsnprintf(txbuff, TXBUFF_SIZ,"AT+CIPMUX=%d\r\n",
                1);
        if (read(RET_OK, (char *) rxbuff, &numread) != RET_OK)
        {
            // post warnings here
        }

        return 0;
    }
    return -1;
}

void Esp8266::disconnectAP()
{
  int numread = RXBUFF_SIZ;
  chprintf((BaseSequentialStream *) _sdp, "AT+CWQAP\r\n");

  //read(RET_OK, (char *) rxbuff, &numread);
  if (read(RET_OK, (char *) rxbuff, &numread) != RET_OK)
  {
      // post warnings here
  }

}

int Esp8266::writechannel(int chanid, const char * data, int datalen)
{
    return 0;
}


