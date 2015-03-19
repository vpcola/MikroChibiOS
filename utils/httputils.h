#ifndef __HTTPUTILS_H__
#define __HTTPUTILS_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com 
 * Date : Wed Mar 04 2015
 **/

#ifdef __cplusplus
extern "C" {
#endif

#include "parseurl.h"

typedef enum {
    GET,
    POST
} req_type;

typedef enum {
    HTTP_1_0,
    HTTP_1_1
} req_version;

typedef enum {
    UNKNOWN = -1,
    APPLICATION,
    AUDIO,
    IMAGE,
    MESSAGE,
    MULTIPART,
    TEXT,
    VIDEO
} content_type;


typedef struct _httpreqheader {
	req_type    reqtype; // GET or POST
    req_version httpversion; // HTTP/1.0 or HTTP/1.1
	char path[MAX_PATH_LEN]; 
    char host[MAX_HOSTNAME_LEN];
} HttpReqHeader;

#define CONTENTSUBTYPE_LEN 50
typedef struct _httpresheader {
    int status;
    content_type contenttype;
    char contentsubtype[CONTENTSUBTYPE_LEN];
    int contentlength;
} HttpRespHeader;


// for use with servers who 
// read the channel for the header information
int sendrequestheader(HttpReqHeader * hdr, int channel);

// for use with clients who 
// sends header infor/request to the server
int recvresponseheader(HttpRespHeader * hdr, int channel);

#ifdef __cplusplus
}
#endif

#endif

