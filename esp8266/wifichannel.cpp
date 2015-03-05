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
static uint8_t queue_buff0[QUEUEBUF_SIZ]  __attribute__ ((section(".bufram")));
static uint8_t queue_buff1[QUEUEBUF_SIZ]  __attribute__ ((section(".bufram")));
static uint8_t queue_buff2[QUEUEBUF_SIZ]  __attribute__ ((section(".bufram")));
static uint8_t queue_buff3[QUEUEBUF_SIZ]  __attribute__ ((section(".bufram")));
static uint8_t queue_buff4[QUEUEBUF_SIZ]  __attribute__ ((section(".bufram")));

// The output queue used to send command
// and data to the esp8266
//static uint8_t queue_buffin[QUEUEBUF_SIZ];

static void notify(GenericQueue *qp) {
  (void)qp;
}

// Our array of input queues, actually no need to do this since Init
// is called later.
static INPUTQUEUE_DECL(iq0, queue_buff0, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq1, queue_buff1, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq2, queue_buff2, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq3, queue_buff3, QUEUEBUF_SIZ, notify, NULL);
static INPUTQUEUE_DECL(iq4, queue_buff4, QUEUEBUF_SIZ, notify, NULL);

//static OUTPUTQUEUE_DECL(iqin, queue_buffin, QUEUEBUF_SIZ, NULL, NULL);


esp_channel _esp_channels[MAX_CONNECTIONS] = {
      { 0, TCP, CHANNEL_UNUSED, false, "", 0, "127.0.0.1", 0, false, &iq0 , 0},
      { 1, TCP, CHANNEL_UNUSED, false, "", 0, "127.0.0.1", 0, false, &iq1 , 0},
      { 2, TCP, CHANNEL_UNUSED, false, "", 0, "127.0.0.1", 0, false, &iq2 , 0},
      { 3, TCP, CHANNEL_UNUSED, false, "", 0, "127.0.0.1", 0, false, &iq3 , 0},
      { 4, TCP, CHANNEL_UNUSED, false, "", 0, "127.0.0.1", 0, false, &iq4 , 0},
};

static SerialDriver * dbgstrm = NULL;

esp_channel * getChannel(int d)
{
    if ((d >= 0 ) && ( d < MAX_CONNECTIONS))
        return &_esp_channels[d];

    return NULL;
}


static MUTEX_DECL(usartmtx);

static void resetChannel(esp_channel * ch)
{
    if (ch)
    {
        ch->status = CHANNEL_UNUSED;
        ch->port = 0;
        ch->localport = 0;
        strncpy(ch->ipaddress, "", IPADDR_MAX_SIZ);
        strncpy(ch->localaddress, "127.0.0.1", IPADDR_MAX_SIZ);
        ch->isservergenerated = false;
        ch->ispassive = false;
        ch->type = TCP;
        ch->usecount = 0;
    }
}

static void onConnectionStatus(ConStatus * constatus)
{

  if(constatus)
  {
    esp_channel * ch = getChannel(constatus->id);

    chprintf((BaseSequentialStream *) dbgstrm,
             ">> Id[%d] Type[%d] IP [%s], port[%d], isserver[%d]\r\n",
             constatus->id, constatus->type, constatus->srcaddress,
             constatus->port, constatus->clisrv);
    if (ch)
    {
        // check if this is a newly connected channel
        if (ch->status != CHANNEL_CONNECTED)
        {
             // new connection, populate details here
             ch->status = CHANNEL_CONNECTED;
             ch->type = constatus->type;
             ch->isservergenerated = (constatus->clisrv == 1);
             strcpy(ch->ipaddress, constatus->srcaddress);
             ch->port = constatus->port;
             ch->usecount = 0; // clients first task is to increment this.
             // TODO: We need a way to signal waiting threads that
             // a new connection is available i.e. used by accept()
             // call. For ChibiOS, a semaphore might be appropriate
        }
    }
  }
}

static void onLineStatus(IPStatus * ipstatus)
{
  // Currently if we receive an "Ulink" or "Linked" message, we do not have
  // a way to know which line id its coming from.
  // Hopefully a new firmware will allow us to discern which line
  // has been disconnected. This is the reason why
  // there is a need to issue CIPSTATUS and parse the results.
  // I wish the Unlink message should be "Unlink:<id>" which
  // would indicate which channel is disconnected.

  for (int i = 0; i < MAX_CONNECTIONS; i++ )
  {
      esp_channel * ch = getChannel(i);
      if (ch)
      {
          // If we have a connection that has changed
          // status
          if ((ipstatus->status[i] == WIFI_CONN_DISCONNECTED)
              && (ch->status == CHANNEL_CONNECTED))
              resetChannel(ch);

          switch(ipstatus->status[i])
          {
              case WIFI_CONN_GOTIP:
              case WIFI_CONN_CONNECTED:
                ch->status = CHANNEL_CONNECTED;
                break;
              case WIFI_CONN_DISCONNECTED:
              default:
                ch->status = CHANNEL_DISCONNECTED;
          }
      }
  }
}

static WORKING_AREA(channelListenerThreadWA, 512);
static msg_t channelListenerThread(void * arg)
{
    // continuously reads the tx circular buffers for
    // each connection and send data to wifi chip
    (void)arg;
    int retval, numread, param, numwritten;
    int chanid, c; //, pollcount = 0;
    esp_channel * channel;
    SerialDriver * sdp = getSerialDriver();

    chRegSetThreadName("wifi");

    while(1)
    {
        // Check if there's a message pending on the 
        // usart
        //chprintf((BaseSequentialStream *)dbgstrm, "<<Waiting for data...\r\n");
        if (!chMtxTryLock(&usartmtx))
        {
          chThdSleepMicroseconds(10);
          continue;
        }

        //chprintf((BaseSequentialStream *)dbgstrm, "<< Got lock...\r\n");
        if (esp8266HasData())
        {
            //chprintf((BaseSequentialStream *)dbgstrm, "<< Got data ...\r\n!");
            // Read the data and determine to which connection
            // it needs to be sent to
            retval = esp8266ReadRespHeader(&chanid, &param, 2000);
            if ((param > 0) && (retval == RET_IPD))
            {
                channel = getChannel(chanid);
                //chprintf((BaseSequentialStream *)dbgstrm, "<< %d Data on channel %d\r\n", bytestoread, chanid);
            
                // Read and push the data to the input queue of the
                // designated connection.
                if (channel)
                {
                  //chprintf((BaseSequentialStream *)dbgstrm, "<< Reading %d data ...\r\n", bytestoread);
                  numread = 0; numwritten = 0;
                  while(numread < param)
                  {
                    c = sdGet(sdp); //  c = sdGetTimeout(sdp, 1000);
                    if (c >= 0) 
                    {
                        // buffer[numread] = c;
                        // Push the character into the queue
                        if (chIQPutI(channel->iqueue, c) == RDY_OK)
                            numwritten++;

                        numread ++;
                    }
                  }

                  if (esp8266ReadUntil("OK\r\n", 1000))
                  {
                    //chprintf((BaseSequentialStream *)dbgstrm, "<< Wrote %d data to queue\r\n", numwritten);
                  }
                }
            }

            if ((retval == RET_UNLINK) || (retval == RET_LINKED))
            {
              // If we receive an "Unlink" after reading,
              // we need to issue a CIPSTATUS in order to let us
              // know what channel it belongs...
              //esp8266CmdCallback("AT+CIPSTATUS", "OK\r\n", onLineStatus);
              esp8266GetIpStatus(onLineStatus,onConnectionStatus);
            }
        } // has data

        chMtxUnlock();

        // Let other threads run ...
        chThdSleepMicroseconds(10);
    }

    return RDY_OK;
}

int wifiInit(int mode, SerialDriver * usart, SerialDriver * dbg)
{
    // Create a mutex lock so that we
    // can synchronize access to the esp8266
    dbgstrm = dbg;

    chMtxInit(&usartmtx);

    // Initialize the read in queue
    //chOQInit(&iqin, queue_buffin, QUEUEBUF_SIZ, NULL, NULL);

    // Initialize all the queues here
    chIQInit(&iq0, queue_buff0, QUEUEBUF_SIZ, notify, NULL);
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
    {
        chprintf((BaseSequentialStream *)dbgstrm, "<< Channel %d is %d\r\n",
                 _esp_channels[i].id,
                  _esp_channels[i].status );

        if ((_esp_channels[i].status == CHANNEL_UNUSED) ||
            (_esp_channels[i].status == CHANNEL_DISCONNECTED))
        {
            _esp_channels[i].status = CHANNEL_DISCONNECTED;
            _esp_channels[i].type = conntype;
            chprintf((BaseSequentialStream *)dbgstrm, "<< Opening channel[%d] with type %d\r\n",
                     i, conntype);

            return i;
        }
    }
    return -1;
}

int channelConnect(int channel, const char * ipaddress, uint16_t port)
{
    int status = -1, retval;
    esp_channel * ch = getChannel(channel);

    if(!ch) return -1; // channel does not exist

    if (ch->status == CHANNEL_CONNECTED) 
    {
        return -2; // Channel already connected
    }

    // A connection request needs to lock the usart
    //chprintf((BaseSequentialStream *)dbgstrm, "<< Acquiring lock ...\r\n");
    chMtxLock(&usartmtx);

    chprintf((BaseSequentialStream *)dbgstrm, "<< Opening channel[%d] [%s:%d] with %d\r\n",
             channel, ipaddress, port, ch->type);

    retval = esp8266Connect(channel, ipaddress, port, ch->type);
    // chprintf((BaseSequentialStream *)dbgstrm, "<< Connect returned %d!\r\n", retval);
    if ((retval == RET_OK) || (retval == RET_LINKED))
    {

        // Once connected, set the status of the channel
        // array.
        ch->status = CHANNEL_CONNECTED;
        ch->port = port;
        strncpy(ch->ipaddress, ipaddress, IPADDR_MAX_SIZ); 
        ch->usecount++;
        ch->ispassive = false;
        ch->isservergenerated = false;
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

int channelServer(int channel, int type, uint16_t port)
{
    // to create a tcp server, we don't need a
    // channel id and type (since for now it only 
    // supports TCP server)
    esp_channel * ch = getChannel(channel);

    if(!ch) return -1;

    // this must be a passive channel
    if (!ch->ispassive) return -1;

    // lock the usart
    chMtxLock(&usartmtx);

    if (ch && (esp8266Server(channel, type, port) == RET_OK))
    {
        // passive socket doesn't have a meaning for
        // the esp8266, we only use this to pass parameters
        // and to simulate a bind-listen-accept socket 
        // like emulation so after we create a server 
        // connection on the esp8266, we free up this channel.
        resetChannel(ch);
    }

    chMtxUnlock();

    return 0;
}


// Closes the channel
int channelClose(int channel)
{
    int retval = -1;

    esp_channel * ch = getChannel(channel);

    if (!ch) return -1;

    // lock the usart
    chMtxLock(&usartmtx);

    if (esp8266Disconnect(channel))
    {
        resetChannel(ch);
        retval = 0;
    }
    
    chMtxUnlock();
    
    return retval;
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
    int numsend = -1;

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
    }

    return numsend;
}

int channelSendTo(int channel, const char * msg, int msglen, const char * ipaddress, uint16_t port)
{
  esp_channel * ch = getChannel(channel);
  if(ch)
  {
    if (channelConnect(channel, ipaddress, port) < 0)
      return -1;
    return channelSend(channel, msg, msglen);
  }

  return -1;
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

int channelReadLine(int chanid, char * buff, int buflen)
{
    int data, numread = 0;
    esp_channel * channel = getChannel(chanid);
    if (channel)
    {
        // Read from the input queue when there's data
        do{
            data = chIQGet(channel->iqueue);
            if (data <= 0) break;
            buff[numread] = (char) data;
            numread++;

            // If newline is reached, 
            // return with numread.
            if ((numread >= 2) &&
                    (buff[numread-2] == '\r') &&
                    (buff[numread-1] == '\n'))
            {
                numread -= 2;
                buff[numread] = 0;
                break;
            }
        }while(numread < buflen);
    } else 
        return -1;

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
