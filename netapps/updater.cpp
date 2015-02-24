#include "updater.h"
#include "ch.hpp"
#include "htu21d.h"
#include "wifichannel.h"
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
    int chanid, numwrite;

    chRegSetThreadName("netupdater");
    chprintf(dbg, "[*] Starting netupdater thread...\r\n");
    chanid = channelOpen(TCP);
    if (chanid >= 0)
    {
      // Connect to sever
      //chprintf(dbg, "[*] Got % chan id, connecting to server\r\n", chanid);
      if (channelConnect(chanid, serverurl, serverport) >= 0)
      {
        // Connected ... start periodic update
        chprintf(dbg, "[*] Connected to server...\r\n");
        while(channelIsConnected(chanid))
        {
          // Get the temperature and humidity
          float temp = getTemp();
          float humidity = getHumidity();
          //chprintf(dbg, "[*] Updating server with %f temp, %f humidity ..\r\n", temp, humidity);
          numwrite = chsnprintf(buff, 100, "Temp: %f\r\n", temp);
          channelSend(chanid, buff, numwrite );
          numwrite = chsnprintf(buff, 100, "Humid: %f\r\n", humidity);
          channelSend(chanid, buff, numwrite);

          chThdSleepSeconds(10); // Sleep 10 seconds
        }

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
