#include "updater.h"
#include "ch.hpp"
#include "htu21d.h"
//#include "wifichannel.h"
#include "wifisocket.h"
#include "chprintf.h"

#define serverurl "192.168.0.107"
#define serverport 4545

static BaseSequentialStream * dbg = NULL;
static char buff[100];

static WORKING_AREA(humidTempUpdaterWA, 512);
static msg_t humidTempUpdater(void * arg)
{
    // continuously reads the tx circular buffers for
    // each connection and send data to wifi chip
    (void)arg;
    int sockfd, numwrite, numwritten;
    sockaddr_in serveraddr;

    chRegSetThreadName("netupd");
    chprintf(dbg, "[*] Starting netupdater thread...\r\n");
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //channelOpen(TCP);
    if (sockfd >= 0)
    {
      // Connect to sever
      //chprintf(dbg, "[*] Got % chan id, connecting to server\r\n", chanid);
      serveraddr.sin_family=AF_INET;
      serveraddr.sin_port=htons(serverport);
      if (inet_aton(AF_INET, serverurl, &serveraddr.sin_addr) <= 0)
      {
        chprintf(dbg, "[*] Invalid URL %s\r\n", serverurl);
      }

      // if (channelConnect(chanid, serverurl, serverport) >= 0)
      if (connect(sockfd, (sockaddr *) &serveraddr, sizeof(serveraddr)) == 0 )
      {
        // Connected ... start periodic update
        chprintf(dbg, "[*] Connected to server...\r\n");
        //while(channelIsConnected(sockfd)) // currently we do not have a way to translate this
        do{
          // Get the temperature and humidity
          float temp = getTemp();
          float humidity = getHumidity();
          //chprintf(dbg, "[*] Updating server with %f temp, %f humidity ..\r\n", temp, humidity);
          numwrite = chsnprintf(buff, 100, "Temp: %f\r\n", temp);
          // channelSend(chanid, buff, numwrite );
          numwritten = send(sockfd, buff, numwrite, 0);
          if (numwritten < 0) break;

          numwrite = chsnprintf(buff, 100, "Humid: %f\r\n", humidity);
          // channelSend(chanid, buff, numwrite);
          numwritten = send(sockfd, buff, numwrite, 0);
          if (numwritten < 0) break;

          chThdSleepSeconds(10); // Sleep 10 seconds
        }while(1);

        close(sockfd);
        chprintf(dbg, "[*] netupdater thread stoped, server closes connection\r\n");
      }
    }
}

void startHumidTempUpdater(BaseSequentialStream * dbgstrm)
{
  dbg = dbgstrm;

  // Start the humidity and temperature updater
  chThdCreateStatic(humidTempUpdaterWA, sizeof(humidTempUpdaterWA),
          NORMALPRIO, humidTempUpdater, NULL);
}
