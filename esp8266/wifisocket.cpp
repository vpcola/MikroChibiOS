#include "wifisocket.h"
#include "wifichannel.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * TODO: This whole file is a big TODO list.
 * I need to provide a socket like API for
 * emulating calls to the esp8266 chip.
 */

char * inet_ntoa(const in_addr *addr)
{
      static char str[16];
        return inet_ntoa_r(addr, str, 16);
}

char * inet_ntoa_r(const in_addr *addr, char *buf, int buflen)
{
    uint32_t s_addr;
    char inv[3];
    char *rp;
    uint8_t *ap;
    uint8_t rem;
    uint8_t n;
    uint8_t i;
    int len = 0;

    s_addr = addr->s_addr;

    rp = buf;
    ap = (uint8_t *)&s_addr;
    for(n = 0; n < 4; n++) {
        i = 0;
        do {
            rem = *ap % (uint8_t)10;
            *ap /= (uint8_t)10;
            inv[i++] = '0' + rem;
        } while(*ap);
        while(i--) {
            if (len++ >= buflen) {
                return NULL;
            }
            *rp++ = inv[i];
        }
        if (len++ >= buflen) {
            return NULL;
        }
        *rp++ = '.';
        ap++;
    }
    *--rp = 0;
    return buf;
}

int inet_aton(int af, const char *cp, in_addr *addr)
{
    uint32_t val;
    uint8_t base;
    char c;
    uint32_t parts[4];
    uint32_t *pp = parts;

    if (af != AF_INET) return -1;

    c = *cp;
    for (;;) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, 1-9=decimal.
         */
        if (!isdigit(c))
            return (0);
        val = 0;
        base = 10;
        if (c == '0') {
            c = *++cp;
            if (c == 'x' || c == 'X') {
                base = 16;
                c = *++cp;
            } else
                base = 8;
        }
        for (;;) {
            if (isdigit(c)) {
                val = (val * base) + (int)(c - '0');
                c = *++cp;
            } else if (base == 16 && isxdigit(c)) {
                val = (val << 4) | (int)(c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            } else
                break;
        }
        if (c == '.') {
            /*
             * Internet format:
             *  a.b.c.d
             *  a.b.c   (with c treated as 16 bits)
             *  a.b (with b treated as 24 bits)
             */
            if (pp >= parts + 3) {
                return (0);
            }
            *pp++ = val;
            c = *++cp;
        } else
            break;
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && !isspace(c)) {
        return (0);
    }
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    switch (pp - parts + 1) {

        case 0:
            return (0);       /* initial nondigit */

        case 1:             /* a -- 32 bits */
            break;

        case 2:             /* a.b -- 8.24 bits */
            if (val > 0xffffffUL) {
                return (0);
            }
            val |= parts[0] << 24;
            break;

        case 3:             /* a.b.c -- 8.8.16 bits */
            if (val > 0xffff) {
                return (0);
            }
            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

        case 4:             /* a.b.c.d -- 8.8.8.8 bits */
            if (val > 0xff) {
                return (0);
            }
            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
        default:
            break;
    }

    if (addr) {
        addr->s_addr = htonl(val);
    }
    return (1);
}

int inet_aton_r(const char *cp)
{
    in_addr val;

    if (inet_aton(AF_INET, cp, &val)) {
        return (int) val.s_addr;
    }
    return (IPADDR_NONE);
}



// For socket(), we need to find an empty channel and 
// return the channel id. 
int socket(int domain, int type, int protocol)
{
    int channeltype;

    if (domain != AF_INET) return -1;
    // currently we only support SOCK_DGRAM and 
    // and SOCK_STREAM
    if ((type != SOCK_STREAM) && (type != SOCK_DGRAM))
        return -1;

    // Only support PROT_IP/TCP/UDP/IPV4
    // sparse documentation indicates ESP8266
    // does not support any other protocol types
    // *wish ICMP was supported* 
    switch(protocol)
    {
        // case IPPROTO_IP:
        case IPPROTO_IPV4:
        case IPPROTO_TCP:
            channeltype = 0; // TCP
            break;
        case IPPROTO_UDP:
            channeltype = 1; // UDP
            break;
        default:
            return -1;
    }

    return channelOpen(channeltype);

}

int close(int sockfd)
{
    // Closes the channel and marks the channel as ready to be
    // used again.
    return channelClose(sockfd);
}

int connect(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    sockaddr_in * dest;
    char ipaddress[100];
    unsigned short port;

    // addr is an outbound address ... cast to sockaddr_in
    if (addrlen == sizeof(sockaddr_in))
    {
        dest = (sockaddr_in *) addr;
        // ntohs
        port = ntohs(dest->sin_port);
        // convert the ip address (canonical/interger format)
        strcpy(ipaddress, inet_ntoa((const in_addr*) &dest->sin_addr));

        return channelConnect(sockfd, ipaddress, port);
    }

    return -1;
}

int send(int sockfd, const void *msg, int len, int flags)
{
    return channelSend(sockfd, (const char *) msg, len);
}

int sendto(int sockfd, const void * buf, int len, int flags, const sockaddr * to, socklen_t tolen)
{
   return -1; 
}

int recv(int sockfd, void *buf, int len, int flags)
{
    return channelRead(sockfd, (char *) buf, len);
}

int recvfrom(int sockfd, void * buf, int len, int flags, sockaddr * from, socklen_t * fromlen)
{
    return -1;
}


