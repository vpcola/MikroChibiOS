#include "netcmd.h"
#include "esp8266.h"
#include "wifichannel.h"
#include "chprintf.h"
#include "shellutils.h"
#include "gfx.h" // GFILE
#include "ntp.h"
#include "parseurl.h"
#include "httputils.h"

#include <string.h>
#include <stdlib.h>

static const char * getreq = "GET /data/2.5/weather?q=Singapore,sg HTTP/1.0\r\n" \
    "Host: api.openweathermap.org\r\n" \
    "\r\n\r\n"; // third line is an empty line
static const char * url = "api.openweathermap.org";


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

void cmd_wget(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    int chanid = 1;
    int numread, bytestoread = 0;
    GFILE * fp = NULL;
    char * buffer, *outfile;
    urlinfo info;
    HttpReqHeader reqheader;
    HttpRespHeader resheader;

    if (argc != 2) {
        chprintf(chp, "Usage: wget <url> <file>\r\n");
        return;
    }

    outfile = argv[1];

    fp = gfileOpen(outfile, "w");
    if (fp)
    {
        chanid = channelOpen(TCP);
        if (chanid >= 0)
        {
            chprintf(chp, "Channel [%d] opened!\r\n", chanid);
            // Get the url info to get host and port
            if (parse_url(&info, argv[0]) < 0)
            {
                gfileClose(fp);
                return;
            }

            chprintf(chp, "Connecting to host [%s:%d]...",
                info.hostname,
                info.port);

            if (channelConnect(chanid, info.hostname, info.port) >= 0)
            {
                chprintf(chp, "connected!\r\n");
                // Now send header information
                reqheader.reqtype = GET;
                reqheader.httpversion = HTTP_1_0;
                strncpy(reqheader.path, info.path, MAX_PATH_LEN);
                strcpy(reqheader.host, "www.lightsurge.com");
                if (sendrequestheader(&reqheader, chanid) < 0)
                {
                    chprintf(chp, "Failed to send request header ...\r\n");
                    channelClose(chanid);
                    gfileClose(fp);
                    return;
                }
                // Get the response header ...
                if (recvresponseheader(&resheader, chanid) < 0)
                {
                    chprintf(chp, "Failed to receive response header ...\r\n");
                    channelClose(chanid);
                    gfileClose(fp);
                    return;
                }
                if (resheader.status == 200) // OK 
                {
                    // Read the rest of the data
                    bytestoread = resheader.contentlength;
                    chprintf(chp, "Received response of type [%d] subtype[%s] size[%d]\r\n",
                        resheader.contenttype,
                        resheader.contentsubtype,
                        bytestoread);

                    // Allocate memory for data receive
                    chprintf(chp, "Reading channel of (%d) bytes ..\r\n", bytestoread);
                    buffer = (char *) malloc(bytestoread + (4 - (bytestoread %4)));
                    if (buffer)
                    {
                        do {
                            numread = channelRead(chanid, buffer, bytestoread);
                            if (numread <= 0) break;
                            chprintf(chp, "Writing %d bytes data to %s\r\n", numread, outfile);
                            gfileWrite(fp, buffer, numread);

                            bytestoread -= numread;

                        }while(bytestoread > 0);

                        free(buffer);
                    }
                }else
                {
                    // Report error here and return
                    chprintf(chp, "HTTP header returned %d\r\n", resheader.status);
                }

            }else
                chprintf(chp, "failed!\r\n");

            channelClose(chanid);
        }
        gfileClose(fp);
    }
}

void cmd_weather(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    int chanid = 1;
    int numsend, numread, bytestoread = 0;
    GFILE * fp = NULL;
    char * buffer;

    if (argc != 1) {
      chprintf(chp, "Usage: weather <file>\r\n");
      return;
    }

    fp = gfileOpen(argv[0], "w");
    if (fp)
    {
      chanid = channelOpen(TCP);
      chprintf(chp, "Channel open returned %d\r\n", chanid);
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
                buffer = (char *) malloc(bytestoread + (4 - (bytestoread %4)));
                if (buffer)
                {
                    do {
                        numread = channelRead(chanid, buffer, bytestoread);
                        if (numread <= 0) break;
                        chprintf(chp, "Writing %d bytes data to %s\r\n", numread, argv[0]);
                        gfileWrite(fp, buffer, numread);

                        bytestoread -= numread;

                    }while(bytestoread > 0);

                    free(buffer);
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

void cmd_ntpdate(BaseSequentialStream * chp, int argc, char * argv[])
{
    (void)argv;
    timeval_x tv;
    
    // 0.sg.pool.ntp.org - 128.199.253.156
    // 1.sg.pool.ntp.org - 203.174.83.202

    if (sntp_get("192.168.0.107", 123, &tv) < 0)
        return;

    chprintf(chp, "NTP returned %ld seconds\r\n", tv.tv_sec);
    chprintf(chp, "             %ld microseconds\r\n", tv.tv_usec);

}


