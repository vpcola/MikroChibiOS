#ifndef __PARSEURL_H_
#define __PARSEURL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_URL_LEN 200
#define MAX_USERNAME_LEN 100
#define MAX_PASSWD_LEN 100
#define MAX_HOSTNAME_LEN 200
#define MAX_PATH_LEN 200


	typedef enum {
		// some schemes we define...
		SCHEME_UNKNOWN,
		SCHEME_HTTPS,
		SCHEME_HTTP,
		SCHEME_FILE,
        SCHEME_FTP,
		SCHEME_MAILTO
	} schemeinfo;

	typedef struct _urlinfo{
		int scheme;
		char username[MAX_USERNAME_LEN];
		char passwd[MAX_PASSWD_LEN];
		char hostname[MAX_HOSTNAME_LEN];
		unsigned short port;
		// TODO: Path can further be
		// broken down to path and query.
		char path[MAX_PATH_LEN];
	} urlinfo;

int parse_url(urlinfo * info, const char* url);


#ifdef __cplusplus
}
#endif


#endif


