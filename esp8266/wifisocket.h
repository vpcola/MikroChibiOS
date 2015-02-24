#ifndef __WIFISOCKET_H__
#define __WIFISOCKET_H__

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Thu Feb 12 2015
 *
 **/
#ifdef __cplusplus
extern "C" {
#endif

#define HOSTNAME_MAX_LENGTH 200

// Address families
#define AF_INET         2
//#define AF_INET6

// Socket types
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

// Socket protocol
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1           // Unfortunately ESP8266 does not support ICMP
#define IPPROTO_IPV4    IPPROTO_IP
#define IPPROTO_TCP     6 
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define IPPROTO_NONE    59
#define IPPROTO_RAW     255
#define IPPROTO_MAX     256


// Socket options
#define SOL_SOCKET                  0xFFFF      // Socket level
#define SOCKOPT_RECV_NONBLOCK       0           // non-blocking receive mode
#define SOCKOPT_RECV_TIMEOUT        1           // Set the default timeout
#define SOCKOPT_ACCEPT_NONBLOCK     2           // Accept on non-blocking mode

#define MAX_PACKET_SIZE     1400


// Socket return codes
#define SOC_ERROR            -1
#define SOC_IN_PROGRESS      -2

// common defs
#define IPADDR_NONE 0


//Use in case of Big Endian only

#define htonl(A)    ((((unsigned long)(A) & 0xff000000) >> 24) | \
                    (((unsigned long)(A) & 0x00ff0000) >> 8) | \
                    (((unsigned long)(A) & 0x0000ff00) << 8) | \
                    (((unsigned long)(A) & 0x000000ff) << 24))

#define ntohl       htonl

//Use in case of Big Endian only
#define htons(A)     ((((unsigned long)(A) & 0xff00) >> 8) | \
                     (((unsigned long)(A) & 0x00ff) << 8))
#define ntohs       htons



typedef struct _in_addr_t {
    unsigned long s_addr;          // load with inet_pton()
} in_addr;

typedef struct _sockaddr_t {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
} sockaddr;


// IPv4 AF_INET sockets:
typedef struct _sockaddr_in_t {
     short            sin_family;   // e.g. AF_INET, AF_INET6
     unsigned short   sin_port;     // e.g. htons(3490)
     in_addr          sin_addr;     // see struct in_addr, above
     char             sin_zero[8];  // zero this if you want to
} sockaddr_in;

typedef unsigned long socklen_t;

// conversion functions, stolen from LWIP 1.4
char * inet_ntoa(const in_addr *addr);
char * inet_ntoa_r(const in_addr *addr, char *buf, int buflen);

int inet_aton_r(const char *cp);
int inet_aton(int af, const char *cp, in_addr *addr);


// Socket emulation layer, provides socket-like API
// to applications
int socket(int domain, int type, int protocol);
int close(int sockfd);

// TODO: Implement the ff functions below
int accept(int sockfd, sockaddr * addr, socklen_t * addrlen);
int bind(int sockfd, const sockaddr * addr, long addrlen);
int listen(int sockfd, long backlog);

// TODO: Implement the ff functions below
int setsockopt(int sockfd, int level, int optname, const void * optval, socklen_t optlen);
int getsockopt(int sockfd, int level, int optname, void * optval, socklen_t * optlen);

int connect(int sockfd, const sockaddr *addr, socklen_t addrlen);

int send(int sockfd, const void *msg, int len, int flags);
// TODO: Implement the ff function below
int sendto(int sockfd, const void * buf, int len, int flags, const sockaddr * to, socklen_t tolen);

int recv(int sockfd, void *buf, int len, int flags);
// TODO: Implement the ff function below
int recvfrom(int sockfd, void * buf, int len, int flags, sockaddr * from, socklen_t * fromlen);

#ifdef __cplusplus
}
#endif

#endif

