#include "parseurl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct _scheme_info
{
    schemeinfo schemetype;
    const char * schemestr;
    unsigned short defaultport;
} infoschemes;

static const infoschemes scheme_list[] = 
{
    { SCHEME_HTTPS, "https", 443 },
    { SCHEME_HTTP,  "http" , 80  },
    { SCHEME_FILE,  "file" , 0   },
    { SCHEME_MAILTO, "mailto", 25},
    { SCHEME_FTP,   "ftp",   21  }
};

#define MAX_SCHEMES_LEN sizeof(scheme_list)/sizeof(infoschemes)

static schemeinfo getScheme(const char * scheme, unsigned short * defport)
{
    int i, numlist = MAX_SCHEMES_LEN;
    *defport = 0;


    for(i = 0; i < numlist; i++)
        if (stricmp(scheme_list[i].schemestr, scheme) == 0)
        {
            *defport = scheme_list[i].defaultport;
            return scheme_list[i].schemetype;
        }

    return SCHEME_UNKNOWN;
}

int parse_url(urlinfo * info, const char* url) 
{
	char uri[MAX_URL_LEN];
	char * p, *z, * u;

	if (!info) return -1;

	strcpy(uri, url);
    // Set default params

	// Locate for the first occurence of ":" in
	// <scheme>://<url>
	if ((p=strchr(uri, ':')) != NULL)
	{
		*p = 0;
        info->scheme = getScheme(uri, &info->port);
		//printf("scheme [%s]\n", uri);
		p  += 3; // skip the "//"
	}

	//printf("Rem uri [%s]\n", p );
	// separate the path part from the host and path
	// <hostname>/<path>
	if ((z = strchr(p, '/')) != NULL)
	{
		// Create 2 strings ...
		*z = 0;
		// p+1 contains the path part
		//printf("path [%s]\n", z+1);
		strncpy(info->path, z+1, MAX_PATH_LEN); 
	}else
	  strcpy(info->path, "");

	// further split the tokens into
	// <user>:<password>
	if ((z = strchr(p, '@')) != NULL)
	{
		// spit again to <user>:<password> @ <hostname>:<port>
		*z = 0; 
		// p contains username and password
		z++; // contains hostname and port
		u = strtok(z, ":");
		if (u)
		{
			// user
			strncpy(info->hostname, u, MAX_HOSTNAME_LEN);
			//printf("hostname [%s]\n", u);
		}
		u = strtok(NULL, ":");
		if (u)
		{
			// password
			info->port = atoi(u);
			//printf("port [%d]\n", info->port);
		}
		// Now handle username and passpword
		u = strtok(p, ":");
		if (u)
		{	
			strncpy(info->username, u, MAX_USERNAME_LEN);
			//printf("username [%s]\n", u);
		}
		u = strtok(NULL, ":");
		if (u)
		{
			strncpy(info->passwd, u, MAX_PASSWD_LEN);
			//printf("passwd [%s]\n", u);
		}
	}else
	{
		u = strtok(p, ":");
		if (u)
		{
			// user
			strncpy(info->hostname, u, MAX_HOSTNAME_LEN);
			//printf("hostname [%s]\n", u);
		}
		u = strtok(NULL, ":");
		if (u)
		{
			// password
			info->port = atoi(u);
			//printf("port [%d]\n", info->port);
		}
	}

	return 0;
}


