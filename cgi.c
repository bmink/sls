#include "cgi.h"
#include "bstr.h"
#include "blog.h"
#include "bcurl.h"
#include "errno.h"
#include "slsobj.h"
#include "rediskeys.h"
#include "hiredis_helper.h"

int cgi_index(bstr_t *, const char *);

int cgi_randitems(const char *, int, bstr_t *);

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


#define CGI_MAXITEMS_ALL	20
#define CGI_MAXITEMS_NEW	6

int
cgi_index(bstr_t *resp, const char *execn)
{
	if(resp == NULL)
		return EINVAL;

	cgi_header("SLS", resp);

	bprintf(resp, "NEW:\n\n");

	cgi_randitems(RK_SPOTIFY_S_ALBUMS_NEW, CGI_MAXITEMS_NEW, resp);

	bprintf(resp, "\n\n<hr>\n");
	bprintf(resp, "ALL:\n\n");

	cgi_randitems(RK_SPOTIFY_S_ALBUMS_ALL, CGI_MAXITEMS_ALL, resp);

	bprintf(resp, "\n\n<hr>\n");
	bprintf(resp, "LISTEN TO NEW:\n\n");

	cgi_randitems(RK_SPOTIFY_LT_ALBUMS_NEW, CGI_MAXITEMS_NEW, resp);

	bprintf(resp, "\n\n<hr>\n");
	bprintf(resp, "LISTEN TO ALL:\n\n");

	cgi_randitems(RK_SPOTIFY_LT_ALBUMS_ALL, CGI_MAXITEMS_ALL, resp);

	bprintf(resp, "\n\n<hr>\n");


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
	bprintf(resp, "	background-color: black;\n");
	bprintf(resp, "	color: white;\n");
	bprintf(resp, "	font-weight: bold;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "pre {\n");
	bprintf(resp, "	white-space: pre-wrap;\n");
	bprintf(resp, "	font-size: 25pt;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "a {\n");
	bprintf(resp, "	color: white;\n");
	bprintf(resp, "}\n");
	bprintf(resp, "  </style>\n");
	bprintf(resp, "  <title>");
	bstrcat_entenc(resp, title);
	bprintf(resp, "</title>\n");
	bprintf(resp, " </head>\n");
	/* Using this weird looking assignment below seems to do what we
	 * want, namely to jump back to the top of the page upon reload.
	 * Using location.reload(); doesn't jump back to the top. */
	bprintf(resp, " <body ondblclick=\"location.href = location.href;\">"
	    "\n");
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


int
cgi_randitems(const char *rediskey, int maxcnt, bstr_t *resp)
{
	int 		err;
	barr_t		*elems;
	bstr_t		*elem;
	int		ret;
	slsalb_t	*alb;
	int		cnt;

	if(xstrempty(rediskey) || !resp)
		return EINVAL;

	err = 0;
	elems = NULL;
	alb = NULL;

	elems = barr_init(sizeof(bstr_t));
	if(elems == NULL) {
		blogf("Couldn't allocate elems");
		err = ENOMEM;
		goto end_label;
	}

	ret = hiredis_srandmember(rediskey, maxcnt, elems);
	if(ret != 0) {
		blogf("Couldn't do srandmember: %s", strerror(ret));
		err = ret;
		goto end_label;
	}

	bprintf(resp, "<table width=\"100%\">\n");
	
	cnt = 0;

	for(elem = (bstr_t *) barr_begin(elems);
	    elem < (bstr_t *) barr_end(elems); ++elem) {
		alb = slsalb_init(NULL);
		if(!alb) {
			blogf("Couldn't allocate alb");
			err = ENOMEM;
			goto end_label;
		}

		ret = slsalb_fromjson(bget(elem), alb);
		if(ret != 0) {
			blogf("Couldn't parse album");
			err = ret;
			goto end_label;
		}

		if(cnt % 2 == 0)
			bprintf(resp, " <tr>\n");

		bprintf(resp, "  <td width=\"50%\"><pre>\n");

		if(!bstrempty(alb->sa_caurl_med)) {
			bprintf(resp, "<img src=\"");
			bstrcat_entenc(resp, bget(alb->sa_caurl_med));
			bprintf(resp, "\" xwidth=\"200\">");
			bprintf(resp, "\n");
		}
		bstrcat_entenc(resp, bget(alb->sa_artist));
		bprintf(resp, "\n");
		bstrcat_entenc(resp, bget(alb->sa_name));
		bprintf(resp, "\n");
		bprintf(resp, "<a target=_blank href=\"");
		bstrcat_entenc(resp, bget(alb->sa_url));
		bprintf(resp, "\">Open</a>");
		bprintf(resp, "\n");

		slsalb_uninit(&alb);

		bprintf(resp, "  </pre></td>\n");

		++cnt;
		if(cnt % 2 == 0)
			bprintf(resp, " </tr>\n");
	}

	if(cnt % 2 == 1)
		bprintf(resp, " <td></td></tr>\n");

	bprintf(resp, "</table>\n");


end_label:

	for(elem = (bstr_t *) barr_begin(elems);
	    elem < (bstr_t *) barr_end(elems); ++elem) {
		buninit_(elem);
	}
	barr_uninit(&elems);

	slsalb_uninit(&alb);

	return err;
}

