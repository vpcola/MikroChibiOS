#include "wifichannel.h"
#include "ch.hpp"
#include "hal.h"
#include "esp8266.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#define QUEUEBUF_SIZ 1500

// A fixed static array of input queues
// Here we define the static buffers for
// our input queues
static uint8_t queue_buff1[QUEUEBUF_SIZ];
static uint8_t queue_buff2[QUEUEBUF_SIZ];
static uint8_t queue_buff3[QUEUEBUF_SIZ];
static uint8_t queue_buff4[QUEUEBUF_SIZ];

static void notify(GenericQueue *qp) {
  (void)qp;
}

// Our array of input queues
static INPUTQUEUE_DECL(iq1, queue_buff1, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq2, queue_buff2, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq3, queue_buff3, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq4, queue_buff4, QUEUEBUF_SIZ, notify, NULL);

esp_channel _esp_channels[MAX_CONNECTIONS] = {
      { 1, TCP, CHANNEL_UNUSED, true, "", 0, &iq1 },
      { 2, TCP, CHANNEL_UNUSED, true, "", 0, &iq2 },
      { 3, TCP, CHANNEL_UNUSED, true, "", 0, &iq3 },
      { 4, TCP, CHANNEL_UNUSED, true, "", 0, &iq4 },
};

static SerialDriver * dbgstrm = NULL;

esp_channel * getChannel(int d)
{
    if ((d >= 0 ) && ( d < MAX_CONNECTIONS))
        return &_esp_channels[d - 1];

    return NULL;
}

static MUTEX_DECL(usartmtx);

char buffer[2048];

static void onLineStatus(IPStatus * ipstatus)
{
  // Currently if we receive an "Ulink" message, we do not have
  // a way to know which line id its coming from.
  // Hopefully a new firmware will allow us to discern which line
  // has been disconnected.
  //
  // Update: Unfortunately the same goes through AT+CIPSTATUS returns
  // just "STATUS:4", again it does not tell which line has been
  // disconnected ....bugger..

  for (int i = 0; i < ESP8266_MAX_CONNECTIONS; i++ )
  {
      esp_channel * ch = getChannel(i+1);
      if (ch)
      {
        ch->status = ipstatus->status[i];
        //chprintf((BaseSequentialStream *) dbgstrm, ">> Updating channel %d with status %d\r\n",
        //         i+1, ipstatus->status[i]);
      }
  }
}

static WORKING_AREA(channelListenerThreadWA, 512);
static msg_t channelListenerThread(void * arg)
{
    // continuously reads the tx circular buffers for
    // each connection and send data to wifi chip
    (void)arg;
    int bytestoread, numread, status, numwritten;
    int chanid; //, pollcount = 0;
    esp_channel * channel;

    chRegSetThreadName("wifi");

    while(1)
    {
        // Check if there's a message pending on the 
        // usart
        //chprintf((BaseSequentialStream *)dbgstrm, "<<Waiting for data...\r\n");
        if (!chMtxTryLock(&usartmtx))
        {
          chThdSleepMicroseconds(100);
          continue;
        }

        //chprintf((BaseSequentialStream *)dbgstrm, "<< Got lock...\r\n");
        if (esp8266HasData())
        {
            //chprintf((BaseSequentialStream *)dbgstrm, "<< Got data ...\r\n!");
            // Read the data and determine to which connection
            // it needs to be sent to
            bytestoread = esp8266ReadRespHeader(&chanid, &status, 2000);
            if ((bytestoread > 0) && (status == 2))
            {
                channel = getChannel(chanid);
                //chprintf((BaseSequentialStream *)dbgstrm, "<< %d Data on channel %d\r\n", bytestoread, chanid);
            
                // Read and push the data to the input queue of the
                // designated connection.
                if (channel)
                {
                  //chprintf((BaseSequentialStream *)dbgstrm, "<< Reading %d data ...\r\n", bytestoread);
                  // TODO: Right now just read whatever data
                  numread = esp8266Read(buffer, bytestoread);
                  // Wait for the "OK" after read ...
                  if (esp8266ReadUntil("OK\r\n", 1000))
                  {
                    // Push the numread bytes to the queue ..
                    // one character at a time ... slow
                    // but haven't really have time to think
                    // about refactoring code yet for speed.
                    //chprintf((BaseSequentialStream *)dbgstrm, "<< Sending data to queue\r\n");
                    numwritten = 0;
                    do {
                       if (chIQPutI(channel->iqueue, buffer[numwritten]) == RDY_OK)
                         numwritten++;
                       else break;
                    }while(numwritten < numread);
                    // TODO: check for error conditions here
                    //chprintf((BaseSequentialStream *)dbgstrm, "<< Wrote %d data to queue\r\n", numwritten);
                  }
                }
            }

            if (status == 0)
            {
              // If we receive an "Unlink" after reading,
              // we need to issu a CIPSTATUS in order to let us
              // know what channel it belongs...
              //esp8266CmdCallback("AT+CIPSTATUS", "OK\r\n", onLineStatus);
              esp8266GetIpStatus(onLineStatus);
            }

            // Signal the waiting read thread that data is now
            // available
        }
#if 0
        else
        {
          if (pollcount > 5)
          {
            chprintf((BaseSequentialStream *) dbgstrm, ">> Updating IP status ... %d\r\n");
            esp8266GetIpStatus(onLineStatus);
            pollcount = 0;
          }
        }
        pollcount++;
#endif

        chMtxUnlock();

        // Let other threads run ...
        chThdSleepMicroseconds(100);
    }

    return RDY_OK;
}

int wifiInit(int mode, SerialDriver * usart, SerialDriver * dbg)
{
    // Create a mutex lock so that we
    // can synchronize access to the esp8266
    dbgstrm = dbg;

    chMtxInit(&usartmtx);

    // Initialize all the queues here
    chIQInit(&iq1, queue_buff1, QUEUEBUF_SIZ, notify, NULL);
    chIQInit(&iq2, queue_buff2, QUEUEBUF_SIZ, notify, NULL);
    chIQInit(&iq3, queue_buff3, QUEUEBUF_SIZ, notify, NULL);
    chIQInit(&iq4, queue_buff4, QUEUEBUF_SIZ, notify, NULL);

    return esp8266Init(usart, mode, dbg);
}

int wifiConnectAP(const char * ssid, const char * password)
{
    int conresult = esp8266ConnectAP(ssid, password);
    if (conresult != WIFI_ERR_NONE)
    {
        // handle an error here
        return -1;
    }

    // Start the channel read thread
    chThdCreateStatic(channelListenerThreadWA, sizeof(channelListenerThreadWA),
            NORMALPRIO, channelListenerThread, NULL);

    return 0;
}

// Channel open returns an unused, empty channel
int channelOpen(int conntype)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
        if ( _esp_channels[i].status == CHANNEL_UNUSED)
        {
            _esp_channels[i].status = CHANNEL_DISCONNECTED;
            _esp_channels[i].type = conntype;

            return i+1;
        }

    return 0;
}

int channelConnect(int channel, const char * ipaddress, uint16_t port)
{
    int status = -1;
    esp_channel * ch = getChannel(channel);

    if(!ch) return -1; // channel does not exist

    if (ch->status == CHANNEL_CONNECTED) 
    {
        return -2; // Channel already connected
    }

    // A connection request needs to lock the usart
    //chprintf((BaseSequentialStream *)dbgstrm, "<< Acquiring lock ...\r\n");
    chMtxLock(&usartmtx);

    if ( esp8266Connect(channel, ipaddress, port, ch->type)) 
    {
        // Once connected, set the status of the channel
        // array.
        ch->status = CHANNEL_CONNECTED;
        ch->port = port;

        strncpy(ch->ipaddress, ipaddress, IPADDR_MAX_SIZ); 
        // reset the queue
        chIQResetI(ch->iqueue);

        status = 0; // no error, connected
    }

    // Unlock the usart here
    chMtxUnlock();
    //chprintf((BaseSequentialStream *)dbgstrm,"<< Unlocked\r\n");

    // return with status
    return status;
}

bool channelIsConnected(int channel)
{
  esp_channel * ch = getChannel(channel);
  if (ch) return (ch->status == CHANNEL_CONNECTED);

  return false;
}

// Closes the channel
int channelClose(int channel)
{
    bool retval = false;
    esp_channel * ch = getChannel(channel);

    // lock the usart
    chMtxLock(&usartmtx);

    retval = esp8266Disconnect(channel);
    if (ch)
    {
      ch->status = CHANNEL_UNUSED;
      ch->port = 0;
      strncpy(ch->ipaddress, "", IPADDR_MAX_SIZ);
    }
    
    chMtxUnlock();

    return (retval) ? 0 : -1;
}
#if 0
static char sendbuff[1024];
int channelPrint(int channel, const char * format, ...)
{
  int numwrite = 0, numwritten;
  va_list ap;
  va_start(ap, format);

  chMtxLock(&usartmtx);
  numwrite = vsnprintf(sendbuff, 1024, format, ap);
  numwritten = channelSend(channel, sendbuff, numwrite);

  chMtxUnlock();
  va_end(ap);

  return numwritten;
}
#endif

bool channelSendLine(int channel, const char * msg)
{
  bool result = false;

  if (channelIsConnected(channel))
  {
    chMtxLock(&usartmtx);
    result = esp8266SendLine(channel, msg);
    chMtxUnlock();
    return result;
  }

  return result;
}

int channelSend(int channel, const char * msg, int msglen)
{
    int numsend = 0;

    if (channelIsConnected(channel))
    {
      // Lock the usart ...
      chMtxLock(&usartmtx);

      //chprintf((BaseSequentialStream *) dbgstrm, ">>Sending Data ...\r\n");
      // Send the message with a blocking call...
      if ( esp8266SendHeader(channel, msglen))
        numsend = esp8266Send(msg, msglen);

      // Unlock the usart ...
      chMtxUnlock();
    }else
      return -1;

    return numsend;
}

int channelRead(int chanid, char * buff, int msglen)
{
    // Channel read reads from an input queue if data
    // is available, otherwise it blocks until all msglen
    // data is read.
    int numread = 0;
    int data;
    esp_channel * channel = getChannel(chanid);

    // chprintf((BaseSequentialStream *) dbgstrm, ">>Reading data from queue\r\n");

    if (!channel)
    {
      // chprintf((BaseSequentialStream *) dbgstrm, ">>Invalid channel\r\n");
      return 0;
    }

    // If disconnected and no data currently on queue
    if (!channelIsConnected(chanid) && chIQIsEmptyI(channel->iqueue))
      return -1;

    // Quirk ... we need to wait indefinitely
    // on the first byte read ..
    buff[numread] = chIQGet(channel->iqueue);
    numread++;

    // Read from the input queue when there's data
    do{
      data = chIQGetTimeout(channel->iqueue,1000);
      if (data >= 0)
      {
        buff[numread] = data;
        numread++;
      }
    }while((data >= 0) && (numread < msglen));

    // chprintf((BaseSequentialStream *) dbgstrm, ">>Read %d data from queue\r\n", numread);
    return numread;
}

int channelGet(int chanid)
{
  esp_channel * channel = getChannel(chanid);

  if (!channelIsConnected(chanid) && chIQIsEmptyI(channel->iqueue))
    return -1;

  if (channel)
    return chIQGet(channel->iqueue);

  return -1;
}
