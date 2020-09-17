#include "cgi.h"
#include "bstr.h"
#include "blog.h"
#include "bcurl.h"
#include "errno.h"
#include "pmcat.h"
#include "pmitem.h"

int cgi_index(bstr_t *, const char *);
int cgi_category(bstr_t *, const char *, const char *);

void cgi_header(const char *, bstr_t *);
void cgi_footer(bstr_t *);

int cgi_listcats(bstr_t *, const char *);


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
		ret = cgi_category(resp, execn, envstr + 4);
		if(ret != 0) {
			blogf("Couldn't generate category page");
			err = ENOEXEC;
			goto end_label;
		}

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

	cgi_header("Pusherman", resp);

	ret = cgi_listcats(resp, execn);
	if(ret != 0) {
		blogf("Couldn't list categories.");
	}

	cgi_footer(resp);

	return 0;
}


#define CGI_CATPAGE_MAXCNT	20

int
cgi_category(bstr_t *resp, const char *execn, const char *catnam)
{
	int		ret;
	pmcat_t		*cat;
	pmitem_t	*pmi;
	int		i;

	if(resp == NULL)
		return EINVAL;

	cat = NULL;
	pmi = NULL;

	cat = pmcat_get(catnam);
	if(cat == NULL) {
		blogf("Couldn't find category.");
		return ENOENT;
	}

	cgi_header("Pusherman", resp);

	bprintf(resp, "%s (%d)\n<hr><hr>", catnam, pmcat_getleft(cat));
	for(i = 0; i < CGI_CATPAGE_MAXCNT; ++i) {

		ret = pmitem_loadnext(cat, &pmi);
		if(ret != 0) {
			if(ret == ENOENT || ret == ERANGE)
				bprintf(resp, "No more visible items.\n");
			else
				bprintf(resp, "Couldn't load next item.\n");
			break;
		}

		pmitem_print(pmi, PMI_PRINTFMT_HTML_SHORT, resp);
		bprintf(resp, "<hr>");

		ret = pmitem_delete(pmi); 
		if(ret != 0) {
			blogf("Couldn't delete pmi");
		}
	}

	if(i >= CGI_CATPAGE_MAXCNT) {
		bprintf(resp, "<a href=\"%s?cat=%s\">More</a> |", execn,
		    catnam);
	}
	bprintf(resp, "<a href=\"%s\">Top</a>", execn);

	bprintf(resp, "\n\n\n\n");
	ret = cgi_listcats(resp, execn);
	if(ret != 0) {
		blogf("Couldn't list categories.");
	}

	cgi_footer(resp);

	if(pmi) {
		ret = pmitem_delete(pmi); 
		if(ret != 0) {
			blogf("Couldn't delete pmi");
		}
	}

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


int
cgi_listcats(bstr_t *resp, const char *execn)
{
	pmcat_t	 	*cat;
	pmcat_stats_t	stats;
	int		err;
	int		ret;
	bstr_t		*lin;

	err = 0;
	lin = NULL;

	lin = binit();
	if(!lin) {
		err = ENOMEM;
		goto end_label;
	}

	for(cat = (pmcat_t *)barr_begin(pmcats);
	    cat < (pmcat_t *)barr_end(pmcats); ++cat) {

		memset(&stats, 0, sizeof(pmcat_stats_t));

		ret = pmcat_getstats(cat, PMCAT_STATTYPE_QUICK, &stats);
		if(ret != 0) {
			blogf("Couldn't get category stats");
			err = ret;
			goto end_label;
		}

		bprintf(resp, "<a href=\"%s?cat=", execn);
		bstrcat_urlenc(resp, bget(cat->pc_name));
		bstrcat(resp, "\">");
		bstrcat_entenc(resp, bget(cat->pc_name));
		bstrcat(resp, "</a> ");

		bclear(lin);
		bprintf(lin, "%d overall, %d visible, oldest is %d hours old\n",
		    stats.ps_all_cnt, stats.ps_visible_cnt,
		    stats.ps_oldesthours);
		bstrcat_entenc(resp, bget(lin));
	}

end_label:
	
	buninit(&lin);

	return err;
}


