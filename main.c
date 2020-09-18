#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "bstr.h"
#include "blog.h"
#include "hiredis_helper.h"
#include "cli.h"
#include "cgi.h"

#define SLS_MODE_CLI	0
#define SLS_MODE_CGI	1


void usage(char *);


int
main(int argc, char **argv)
{
	int		ret;
	char		*execn;
	int		mode;

	if(xstrempty(argv[0])) {
		fprintf(stderr, "Invalid argv[0]\n");
		goto end_label;
	}

	execn = basename(argv[0]);
	if(xstrempty(execn)) {
		fprintf(stderr, "Can't get executable name\n");
		goto end_label;
	}

	if(xstrendswith(execn, ".cgi")) {
		mode = SLS_MODE_CGI;
	} else {

		if(argc != 2) {
			usage(execn);
			exit(-1);
		}

		if(!xstrcmp(argv[1], "cli"))
			mode = SLS_MODE_CLI;
		else {
			usage(execn);
			exit(-1);
		}
	}

	ret = blog_init(execn);
	if(ret != 0) {
		fprintf(stderr, "Could not initialize logging: %s\n",
		    strerror(ret));
		goto end_label;
	}

	ret = hiredis_init();
	if(ret != 0) {
		fprintf(stderr, "Could not connect to redis\n");
		goto end_label;
	}

	switch(mode) {

	case SLS_MODE_CLI:
		cli_loop();
		break;

	case SLS_MODE_CGI:
		(void) cgi_handle(execn);
		break;
	}
		

end_label:

	hiredis_uninit();

	(void) blog_uninit();

	return 0;
}


void
usage(char *execn)
{
	if(xstrempty(execn))
		return;

	printf("Usage: %s <cli>, or invoke as CGI\n", execn);
}

