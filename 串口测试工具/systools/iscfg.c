#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shared.h"
#include "product.h"
#include "ih_errno.h"

void iscfg_usage(char *cmd)
{
	printf("usage:\n");
	printf("	%s read path\n",cmd);
	printf("	%s write path\n",cmd);
	printf("	%s restore\n",cmd);
}

#ifdef STANDALONE
int main(int argc, char* argv[])
#else
int iscfg_main(int argc, char* argv[])
#endif
{
	if(argc < 2) { 
		iscfg_usage(argv[0]);
		return -1;
	} else if(strcmp(argv[1],"read") == 0) {
		return relocate_is_config(argv[2]);
	} else if(strcmp(argv[1],"write") == 0) {
		return save_is_config(argv[2]);
	} else if(strcmp(argv[1],"restore") == 0) {
		restore_is_config();
		return 0;
	}

	iscfg_usage(argv[0]);
	return -1;	
}
