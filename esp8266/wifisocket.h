#ifndef __WIFISOCKET_H__
#define __WIFISOCKET_H__

/**
 * Author : Cola Vergil
 * Email  : tkcov@svsqdcs01.telekurs.com
 * Date : Thu Feb 12 2015
 **/
#ifdef __cplusplus
extern "C" {
#endif

    struct sockaddr {
        unsigned short    sa_family;    // address family, AF_xxx
        char              sa_data[14];  // 14 bytes of protocol address
    };

    struct in_addr {
        unsigned long s_addr;          // load with inet_pton()
    };

    // IPv4 AF_INET sockets:
    struct sockaddr_in {
        short            sin_family;   // e.g. AF_INET, AF_INET6
        unsigned short   sin_port;     // e.g. htons(3490)
        struct in_addr   sin_addr;     // see struct in_addr, above
        char             sin_zero[8];  // zero this if you want to
    };

#define socklen_t   int

// Socket emulation layer, provides socket-like API
// to applications
int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int send(int sockfd, const void *msg, int len, int flags);
int recv(int sockfd, void *buf, int len, int flags);
int close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif

