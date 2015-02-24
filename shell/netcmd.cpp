#include "netcmd.h"
#include "esp8266.h"
#include "wifichannel.h"
#include "chprintf.h"
#include "shellutils.h"
#include "gfx.h" // GFILE

#include <string.h>
#include <stdlib.h>

#define WEATHERBUF_SIZ 8196
static char weatherbuff[WEATHERBUF_SIZ];// __attribute__((section("*.ccm"))) = { 0 };
static const char * getreq = "GET /data/2.5/weather?q=Singapore,sg HTTP/1.0\r\n" \
    "Host: api.openweathermap.org\r\n" \
    "\r\n\r\n"; // third line is an empty line
static const char * url = "api.openweathermap.org";

/**
 * This function calls raw esp8266 functions.
 */
#if 0
void cmd_wget(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    int id = 1;
    int status, bytestoread = 0;


    // First make sure we're connected to AP
    if (esp8266GetIPAddress() != NULL)
    {
      // Connect to the website url and port
      if (esp8266Connect(id, "api.openweathermap.org", 80, TCP))
      //if (esp8266Connect(id, "192.168.0.111", 80, TCP))
      //if (esp8266Connect(id, "192.168.0.107", 80, TCP))
      {
        chprintf(chp, ">>Connected!\r\n");

        if (esp8266SendHeader(id, strlen(getreq)))
          esp8266Send(getreq, strlen(getreq));

        //chThdSleepMilliseconds(2000);

        // Read until we get the response header
        while (((bytestoread = esp8266ReadRespHeader(&id,&status,5000)) > 0) && (id == 1))
        {
          // Read the rest of data
          if ((bytestoread < 8196) && (status == 2))
          {
            memset(weatherbuff, 0, bytestoread);
            esp8266Read(weatherbuff, bytestoread);
            // TODO: Call a callback function here to store
            //  the result of the read
          }
          // Read the remaining lines until
          // we find OK (read finished) or Unlink (where server closes connection)
          if(!esp8266ReadUntil("OK\r\n", 500)) break;
        }

        if (bytestoread > 0)
          esp8266Disconnect(id);
      }
    }

}
#endif


static int readheaderline(int channelid, char * buff, int bufsiz)
{
  int c, numread = 0;

  memset(buff, 0, bufsiz);

  while(
      ((c = channelGet(channelid)) >= 0) &&
      (numread < bufsiz)
      )
  {
      buff[numread] = c;
      numread++;

      if ((numread >= 2) &&
          (buff[numread-1] == '\n') &&
          (buff[numread-2] == '\r'))
      {
        // Read a line ... null terminate and return
        numread -= 2;
        buff[numread] = 0;
        break;
      }
  }

  return strlen(buff);
}

/**
 * The http header has Content-Length: <numbytes>
 * which we read in order to determine the number of bytes
 * we need to read for the reply
 */
static int readhttpsize(int channelid)
{
  char data[200];
  int datatoread = 0;
  // http header has two consecutive \r\n (empty line)
  // to terminate the header
  while(readheaderline(channelid, data, 200) > 0)
  {
    if(strstr(data, "Content-Length: ") != NULL)
      datatoread = atoi(data + 16);
  }

  return datatoread;
}

void cmd_weather(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    int chanid = 1;
    int numsend, numread, bytestoread = 0;
    GFILE * fp = NULL;

    if (argc != 1) {
      chprintf(chp, "Usage: weather <file>\r\n");
      return;
    }

    fp = gfileOpen(argv[0], "w");
    if (fp)
    {
      chanid = channelOpen(TCP);
      if (chanid >= 0)
      {
        chprintf(chp, "Channel [%d] opened!\r\n", chanid);
        if (channelConnect(chanid, url, 80) >= 0)
        {
            chprintf(chp, "Channel connected to %s\r\n", url);
            numsend = channelSend(chanid, getreq, strlen(getreq));
            if (numsend > 0)
            {
                chprintf(chp, "Sent GET request header (%d) to channel\r\n", numsend);
                // Read the reply here ...
                bytestoread = readhttpsize(chanid);
                chprintf(chp, "Reading channel reply of (%d) bytes ..\r\n", bytestoread);
                numread = channelRead(chanid, weatherbuff, bytestoread);
                if (numread > 0)
                {
                  chprintf(chp, "Writing %d bytes data to %s\r\n", numread, argv[0]);
                  gfileWrite(fp, weatherbuff, numread);
                }
            }
            channelClose(chanid);
         }else
            chprintf(chp, "Failed to connect to %s\r\n", url);
      }else
        chprintf(chp, "Can not get available channel to use ...\r\n");
      chprintf(chp, "Closing file...\r\n");
      gfileClose(fp);
    }
}

void cmd_ipstat(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argv;

  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (channelIsConnected(i))
    {
      esp_channel * channel = getChannel(i);
      if (channel)
      {
        chprintf(chp, "Channel %d connected to:\r\n", i);
        chprintf(chp, "\tIP: %s\r\n", channel->ipaddress);
        chprintf(chp, "\tPort: %d\r\n", channel->port);
      }
    }else
      chprintf(chp, "Channel %d is disconnected\r\n", i);
  }
}
