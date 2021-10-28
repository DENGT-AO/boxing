#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ih_logtrace.h"
/*
 *argv[1]: output_file
  argv[2]: domain_name
  content of output_file,e.g.: baidu.com:220.181.6.81
 */
int dnsc_main(int argc,char *argv[])
{
	FILE *pf;
	struct hostent *he;
	struct in_addr ia;
	const char *s;
	
	if (argc < 3) {
		LOG_ER("DNSC: invalid argument");
		return -EINVAL;
	}

	if ((he = gethostbyname(argv[2])) != NULL) {
		memcpy(&ia, he->h_addr_list[0], sizeof(ia));
		s = inet_ntoa(ia);
		
		//LOG_DB("DNSC: got %s <==>%s", argv[2], s);
		if ( (pf = fopen(argv[1],"w")) != NULL ) {
			fprintf(pf,"%s:%s\n",argv[2],s);
			fclose(pf);
			return 0;
		}

		LOG_WA("DNSC: fail to save result to %s", argv[1]);
	} else {
		LOG_WA("DNSC: unable to resolve: %s", argv[2]);
	}

	return -1;
}
