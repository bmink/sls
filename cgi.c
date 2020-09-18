#include "cgi.h"
#include "bstr.h"
#include "blog.h"
#include "bcurl.h"
#include "errno.h"
#include "slsobj.h"

int cgi_index(bstr_t *, const char *);

void cgi_header(const char *, bstr_t *);
void cgi_footer(bstr_t *);



int
cgi_handle(const char *execn)
{
	int	err;
	bstr_t	*resp;
	int	ret;
	char	*envstr;

	err = 0;
	resp = NULL;

	resp = binit();
	if(resp == NULL) {
		blogf("Couldn't allocate resp");
		err = ENOMEM;
		goto end_label;
	}

	envstr = getenv("QUERY_STRING");

	if(!xstrbeginswith(envstr, "cat=")) {

		ret = cgi_index(resp, execn);
		if(ret != 0) {
			blogf("Couldn't generate index page");
			err = ENOEXEC;
			goto end_label;
		}
	} else {
#if 0
		ret = cgi_category(resp, execn, envstr + 4);
		if(ret != 0) {
			blogf("Couldn't generate category page");
			err = ENOEXEC;
			goto end_label;
		}
#endif
	}

end_label:

	if(err == 0) {
		printf("Content-Type: text/html;charset=UTF-8\n\n");
		printf("%s\n", bget(resp));
	} else {
		printf("Content-Type: text/plain\n\n");
		printf("Error: %s\n", strerror(err));
	}

	buninit(&resp);

	return err;
}


int
cgi_index(bstr_t *resp, const char *execn)
{
	int	ret;

	if(resp == NULL)
		return EINVAL;

	cgi_header("SLS", resp);

	cgi_randalbs(resp);

	cgi_footer(resp);

	return 0;
}


void
cgi_header(const char *title, bstr_t *resp)
{
	if(resp == NULL)
		return;

	bprintf(resp, "<html>\n");
	bprintf(resp, " <head>\n");
	bprintf(resp, "  <style>\n");
	bprintf(resp, "body {\n");
	bprintf(resp, "        background-color: black;\n");
	bprintf(resp, "        color: white;\n");
	bprintf(resp, "        font-weight: bold;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "pre {\n");
	bprintf(resp, "        white-space: pre-wrap;\n");
	bprintf(resp, "        font-size: 25pt;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "a {\n");
	bprintf(resp, "        color: white;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "  </style>\n");
	bprintf(resp, "  <title>");
	bstrcat_entenc(resp, title);
	bprintf(resp, "</title>\n");
	bprintf(resp, " </head>\n");
	bprintf(resp, " <body ondblclick=\"location.reload();\">\n");
	bprintf(resp, "  <pre>\n");
}


void cgi_footer(bstr_t *resp)
{
	if(resp == NULL)
		return;

	bprintf(resp, "  </pre>\n");
	bprintf(resp, " </head>\n");
	bprintf(resp, "</html>\n");
}


