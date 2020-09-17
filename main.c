#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "main.h"
#include "bstr.h"
#include "blog.h"
#include "bcurl.h"
#include "hiredis_helper.h"
#include "cli.h"
#include "pmcat.h"
#include "pmsource.h"
#include "cgi.h"

#define PM_MODE_INIT	0
#define PM_MODE_CHECK	1
#define PM_MODE_CLI	2
#define PM_MODE_CGI	3
#define PM_MODE_CLEANUP	4


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
		mode = PM_MODE_CGI;
	} else {

		if(argc != 2) {
			usage(execn);
			exit(-1);
		}

		if(!xstrcmp(argv[1], "init"))
			mode = PM_MODE_INIT;
		else
		if(!xstrcmp(argv[1], "check"))
			mode = PM_MODE_CHECK;
		else
		if(!xstrcmp(argv[1], "cli"))
			mode = PM_MODE_CLI;
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

	/* Needs to be called before pmsource_init() */
	ret = pmcat_init();
	if(ret != 0) {
		fprintf(stderr, "Could not initialize categories\n");
		goto end_label;
	}

	ret = pmsource_init();
	if(ret != 0) {
		fprintf(stderr, "Could not initialize sources\n");
		goto end_label;
	}


	switch(mode) {
	case PM_MODE_CHECK:
		(void) pmsource_all_check();

		break;

	case PM_MODE_CLI:
		cli_loop();
		break;

	case PM_MODE_CGI:
		(void) cgi_handle(execn);
		break;
	}
		

end_label:

	bcurl_uninit();

	(void) pmsource_uninit();

	(void) pmcat_uninit();

	hiredis_uninit();

	(void) blog_uninit();

	return 0;
}


void
usage(char *execn)
{
	if(xstrempty(execn))
		return;

	printf("Usage: %s <init|check|cli>\n", execn);
}

