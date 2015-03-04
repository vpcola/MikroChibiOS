//
// sntp.cpp
//

#include <string.h>
#include "hal.h"
#include "chprintf.h"
#include "ntp.h"
#include "wifichannel.h"
#include "wifisocket.h"
#include "timeutils.h"

#define NTPEPOCH            (86400U * (365U * 70U + 17U))
#define NTPPORT             123

extern SerialDriver SD1;

typedef struct _ntp_packet {
    unsigned char mode_vn_li;
    unsigned char stratum;
    char poll;
    char precision;
    unsigned long root_delay;
    unsigned long root_dispersion;
    unsigned long reference_identifier;
    unsigned long reference_timestamp_secs;
    unsigned long reference_timestamp_fraq;
    unsigned long originate_timestamp_secs;
    unsigned long originate_timestamp_fraq;
    unsigned long receive_timestamp_seqs;
    unsigned long receive_timestamp_fraq;
    unsigned long transmit_timestamp_secs;
    unsigned long transmit_timestamp_fraq;
} ntp_packet;

#if 0
int sntp_get(const char * hostname, int port, timeval_x *tv)
{
    sockaddr_in sa;
    ntp_packet pkt;
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return -1;

    // TODO:
    // convert destination hostname and port
    // to ip address (via gethostbyname)
    sa.sin_family=AF_INET;
    sa.sin_port=htons(port);
    if (inet_aton(AF_INET, hostname, &sa.sin_addr) <= 0)
        return -1;


    if (connect(sockfd, (sockaddr *) &sa, sizeof(sa)) < 0)
        return -1;

    memset(&pkt, 0, sizeof pkt);
    pkt.mode_vn_li = (4 << 3) | 3;
    pkt.originate_timestamp_secs = htonl(secondsinceepoch() + NTPEPOCH);

    if(send(sockfd, &pkt, sizeof(pkt), 0) != sizeof(pkt))
        return -1;

    if(recv(sockfd, &pkt, sizeof(pkt), 0) != sizeof(pkt))
        return -1;

    tv->tv_sec = ntohl(pkt.transmit_timestamp_secs) - NTPEPOCH;
    tv->tv_usec = ntohl(pkt.transmit_timestamp_fraq) / 4295;

    close(sockfd);

    return 0;
}
#endif

int sntp_get(const char * hostname, int port, timeval_x *tv)
{
  int chanid;
  ntp_packet pkt;

  chanid = channelOpen(UDP);
  if(chanid > 0)
  {
      if (channelConnect(chanid, hostname, port) < 0)
        return -1;
      chprintf((BaseSequentialStream *)&SD1, "SNTP connected ...\r\n");

      memset(&pkt, 0, sizeof pkt);
      pkt.mode_vn_li = (4 << 3) | 3;
      pkt.originate_timestamp_secs = htonl(secondsinceepoch() + NTPEPOCH);
      if (channelSend(chanid, (const char *) &pkt, sizeof(pkt)) == sizeof(pkt))
      {
         chprintf((BaseSequentialStream *)&SD1, "SNTP data sent ...\r\n");
         // Read back the data
         if (channelRead(chanid, (char *) &pkt, sizeof(pkt)) == sizeof(pkt))
         {
            chprintf((BaseSequentialStream *)&SD1, "SNTP data received ...\r\n");
            // Set the time
            tv->tv_sec = ntohl(pkt.transmit_timestamp_secs) - NTPEPOCH;
            tv->tv_usec = ntohl(pkt.transmit_timestamp_fraq) / 4295;

            channelClose(chanid);
            return 0;
         }
      }
      channelClose(chanid);
  }

  return -1;
}
