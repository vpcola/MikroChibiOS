#include "wifisocket.h"

/**
 * TODO: This whole file is a big TODO list.
 * I need to provide a socket like API for
 * emulating calls to the esp8266 chip.
 */

// For socket(), we need to find an empty channel and 
// return the channel id. 
int socket(int domain, int type, int protocol)
{
    // TODO: find an unused channel and return with
    // the channel id

    return -1;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    // TODO:
    // the sockfd parameter contains the channel id.
    // Send a request to connect over the request queue of the
    // channel. The thread then sees this request on iteration
    // and send the command to the esp8266.
    //
    // The calling thread has to wait until the semaphore is
    // signalled which means the "Linked" command reply from 
    // from the 8266 has been received. Afterwards we are then
    // connected.

    return -1;
}

int send(int sockfd, const void *msg, int len, int flags)
{
    // TODO:
    // the send() command sends data to the request queue of the
    // channel. the thread then process this request and sends
    // the data to the esp8266.
    // The calling thread for this function is blocked (waiting for
    // semaphore to be signalled) untill all data is sent from 
    // the request queue.
    // This function returns the number of bytes transferred.

    return 0;
}

int recv(int sockfd, void *buf, int len, int flags)
{
    // This routine simply reads the data in the queue if data
    // is available. It waits for further data (calling thread
    // is blocked) untill the full len of bytes is received.

    return 0;
}

int close(int sockfd)
{
    // Closes the channel and marks the channel as ready to be
    // used again.

    return 0;
}

