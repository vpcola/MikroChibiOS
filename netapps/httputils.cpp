#include "httputils.h"
#include "wifichannel.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>

#define TMPBUFF_LEN 300
static char tmpbuff[TMPBUFF_LEN];

typedef struct _httpcontenttype {
    content_type type;
    const char * strtype;
} httpcontenttype;

static const httpcontenttype content_types[] = 
{
    { APPLICATION,  "application" },
    { AUDIO,        "audio"       },
    { IMAGE,        "image"       },
    { MESSAGE,      "message"     },
    { MULTIPART,    "multipart"   },
    { TEXT,         "text"        },
    { VIDEO,        "video"       },
};

#define CONTENT_TYPES_LEN sizeof(content_types)/sizeof(httpcontenttype)

int sendrequestheader(HttpReqHeader * hdr, int channel)
{
    int numtosend;
    // Send information from hdr to channel
    if (!hdr) return -1;
    numtosend = chsnprintf(tmpbuff, TMPBUFF_LEN, "%s /%s %s\r\nHost: %s\r\nConnection: %s\r\n\r\n\r\n",
        (hdr->reqtype == GET) ? "GET" : "POST",
        hdr->path,
        (hdr->httpversion == HTTP_1_0) ? "HTTP/1.0" : "HTTP/1.1",
        hdr->host,
        "close");

    return channelSend(channel, tmpbuff, numtosend);
}

static content_type getcontenttype(const char * str)
{
    int i, numcontenttypes = CONTENT_TYPES_LEN;
    for(i = 0; i < numcontenttypes; i++)
    {
        if (strstr(str, content_types[i].strtype) != NULL) 
            return content_types[i].type;
    }
    return UNKNOWN;
}

int recvresponseheader(HttpRespHeader * hdr, int channel)
{
    int linelen, numlines = 0;
    char * p, * z ;


    if (!hdr) return -1;
    

    do {
        linelen = channelReadLine(channel, tmpbuff, TMPBUFF_LEN);
        if (linelen > 0)
        {
            if (numlines == 0) // read the status and result
            {
                p = strtok(tmpbuff, " ");
                if (p)
                {
                    // ignore the version
                }
                p = strtok(NULL, " ");
                if (p)
                    hdr->status = atoi(p);
                p = strtok(NULL, " ");
                if (p)
                {
                    // ignore the status str
                }
            }else
            {
                // parse the request header
                p = strchr(tmpbuff, ':');
                if (p)
                {
                    *p = 0;
                    // tmpbuff contains the type
                    if (strstr(tmpbuff, "Content-Type") != NULL)
                    {
                        // Get the application type and value
                        z = strtok(p+1, "/");
                        if (z)
                            hdr->contenttype = getcontenttype(z);

                        z = strtok(NULL, ";");
                        if (z)
                            strncpy(hdr->contentsubtype, z, CONTENTSUBTYPE_LEN);
                    }

                    if (strstr(tmpbuff, "Content-Length") != NULL)
                        hdr->contentlength = atoi(p+1);
                }
            }
            numlines++;
        }
    }while(linelen > 0);

    return numlines;
}

