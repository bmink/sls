int
cli_loop(void)
{
	return 0;
}

#if 0

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "blog.h"
#include "bstr.h"
#include "blist.h"
#include "cli.h"
#include "pmcat.h"
#include "pmitem.h"

#define PMCLI_PROMPT		"> "
#define PMCLI_DEFCAT		"computer"
#define PMCLI_READSESSLEN	1800

int cli_openurl(bstr_t *);
int cli_catstats(pmcat_t *, int);
int cli_listcats(pmcat_t *);
pmcat_t *cli_getnextnecat(pmcat_t *);

time_t	cli_lastread;
int	cli_thissess;

int cli_pmihist_init(void);
int cli_pmihist_uninit(void);
int cli_pmihist_clear(void);
int cli_pmihist_add(pmitem_t *);
pmitem_t *cli_pmihist_getidx(int);

#define CLI_PMIHIST_MAXCNT	200
static blist_t	*cli_pmihist = NULL;

#define CLI_DISPFMT_LONG	0
#define CLI_DISPFMT_SHORT	1

#define CLI_PAGE_CNT		20	


int
cli_loop(void)
{
	char		*line;
	int		ret;
	pmitem_t	*pmi;
	pmcat_t		*curcat;
	pmcat_t		*newcat;
	time_t		now;
	int		dispfmt;
	int		dispcnt;
	int		i;
	char		*arg;
	blelem_t	*helem;
	int		left;
	int		ffcnt;

	curcat = NULL;
	pmi = NULL;
	cli_lastread = 0;
	cli_thissess = 0;
	dispfmt = CLI_DISPFMT_LONG;

	ret = cli_pmihist_init();
	if(ret != 0) {
		fprintf(stderr, "Couldn't initialize item history.\n");
		return ret;
	}

	curcat = pmcat_get(PMCLI_DEFCAT);
	if(curcat == NULL) {
		fprintf(stderr, "Couldn't find default category '%s'\n",
		    PMCLI_DEFCAT);
		return ENOENT;
	}

	cli_catstats(curcat, 0);


/*
	printf("%s", PMCLI_PROMPT);
*/

	while((line = readline(PMCLI_PROMPT))) {


/* readline() does this for us.
		(void) xstrchopnewline(line);
*/

/*
		if(xstrempty(line)) {
			printf("%s", PMCLI_PROMPT);
			continue;
		}
*/

		if(!xstrempty(line)) {
			add_history(line);
		}

		if(!xstrcmp(line, "n") || !xstrcmp(line, ";") ||
		    xstrempty(line) || !xstrcmp(line, "p")) {
			if(!xstrcmp(line, "p") &&
			    dispfmt != CLI_DISPFMT_SHORT) {
				printf("Switching to short"
				    " display format...\n");
				dispfmt = CLI_DISPFMT_SHORT;
			}

			if(!xstrcmp(line, "p"))
				dispcnt = CLI_PAGE_CNT;
			else
				dispcnt = 1;

			for(i = 0; i < dispcnt; ++i) {
			
				if(pmi != NULL) {
					ret = pmitem_delete(pmi);
					if(ret != 0) {
						printf("Couldn't delete "
						    "current item.\n");
					} else {
						/* Originally we called
						 * pmitem_uninit() here, but
						 * now we just stash the
						 * pmitem away in the history.
						 * It will be uninited when
						 * the history list goes
						 * away */
						ret = cli_pmihist_add(pmi);
						if(ret != 0) {
							blogf("Couldn't add"
							    " item to history");
						}
						pmi = NULL;
					}
				}
				ret = pmitem_loadnext(curcat, &pmi);
				if(ret != 0) {
					if(ret == ENOENT || ret == ERANGE)
						printf("No more visible"
						    " items.\n");
					else
						printf("Couldn't load next"
						    " item.\n");
					break;
				} else {
					if(dispfmt == CLI_DISPFMT_LONG)
						pmitem_print(pmi,
						    PMI_PRINTFMT_LONG, NULL);
					else
					if(dispfmt == CLI_DISPFMT_SHORT)
						pmitem_print(pmi,
						    PMI_PRINTFMT_SHORT, NULL);
	
					now = time(NULL);
					if(now - cli_lastread >
					    PMCLI_READSESSLEN) {
						cli_thissess = 0;
					}
					cli_thissess++;
					cli_lastread = now;
				}

			}
	
			/* Only print footer if we displayed any items
			 * at all. */ 
			if(i > 0) {
				left = pmcat_getleft(curcat);
				printf("       (left: %d | this: %d)\n",
				    left > 0? left - 1: 0, cli_thissess);
			}

		} else
		if(!xstrcmp(line, "ou")) {
			if(pmi == NULL) {
				printf("No item loaded.\n");
			} else
			if(bstrempty(pmi->pi_url)) {
				printf("Item does not have URL.\n");
			} else {
				ret = cli_openurl(pmi->pi_url);
				if(ret != 0) {
					printf("Could not open item URL.\n");
				}
			}
		} else
		if(xstrbeginswith(line, "ou:") ||
		    xstrbeginswith(line, "ou ")) {
			arg = line + 3;
			if(xstrempty(arg)) {
				printf("No title fragment specified.\n");
			} else
			if(pmi && bstrcasebeginswith(pmi->pi_title, arg)) {
				ret = cli_openurl(pmi->pi_url);
				if(ret != 0) {
					printf("Could not open item URL.\n");
				}
			} else
			if(cli_pmihist != NULL){
				for(helem = cli_pmihist->bl_first;
				    helem != NULL; helem = helem->be_next) {
					if(bstrcasebeginswith(
					    ((pmitem_t *)helem->be_data)
					    ->pi_title, arg)) {
						ret = cli_openurl(
						    ((pmitem_t *)helem->be_data)
						    ->pi_url);
						if(ret != 0) {
							printf("Could not open"
							    " item URL.\n");
						}
						break;
					}
				}
				if(helem == NULL) {
					printf("Not found.\n");
				}
			} else {
				printf("Not found.\n");
			}
		} else
		if(!xstrcmp(line, "ft")) {
			if(pmi == NULL) {
				printf("No item loaded.\n");
			} else
			if(bstrempty(pmi->pi_title)) {
				printf("Item has empty title.\n");
			} else {
				printf("%s\n", bget(pmi->pi_title));
			}
		} else
		if(xstrbeginswith(line, "ft ")) {
			arg = line + 3;
			if(xstrempty(arg)) {
				printf("No title fragment specified.\n");
			} else
			if(pmi && bstrcasebeginswith(pmi->pi_title, arg)) {
				printf("%s\n", bget(pmi->pi_title));
			} else
			if(cli_pmihist != NULL) {
				for(helem = cli_pmihist->bl_first;
				    helem != NULL; helem = helem->be_next) {
					if(bstrcasebeginswith(
					    ((pmitem_t *)helem->be_data)
					    ->pi_title, arg)) {
						printf("%s\n", bget(
						    ((pmitem_t *)
					 	    helem->be_data)->pi_title));
						break;
					}
				}
				if(helem == NULL) {
					printf("Not found.\n");
				}
			} else {
				printf("Not found.\n");
			}
		} else
		if(!xstrcmp(line, "ol")) {
			if(pmi == NULL) {
				printf("No item loaded.\n");
			} else
			if(bstrempty(pmi->pi_link)) {
				printf("Item does not have a link.\n");
			} else {
				ret = cli_openurl(pmi->pi_link);
				if(ret != 0) {
					printf("Could not open item URL.\n");
				}
			}
		} else
		if(!xstrcmp(line, "s")) {
			ret = cli_catstats(curcat, 0);
			if(ret != 0) {
				printf("Could not print stats.\n");
			}
		} else
		if(!xstrcmp(line, "sl")) {
			ret = cli_catstats(curcat, 1);
			if(ret != 0) {
				printf("Could not print stats.\n");
			}
		} else
		if(!xstrcmp(line, "ls")) {
			ret = cli_listcats(curcat);
			if(ret != 0) {
				printf("Could not print stats.\n");
			}
		} else
		if(!xstrcmp(line, "cc") || !xstrcmp(line, "cd")) {
			printf("No category specified.\n");
		} else
		if(xstrbeginswith(line, "cc ") ||
		    xstrbeginswith(line, "cd ")) {
			if(xstrlen(line + 3)) {
				newcat = pmcat_get(line + 3);
				if(newcat == NULL) {
					printf("Category not found.\n");
				} else {
					/* If currently viewing an item, then
					 * "forget" about it, so it's the first
					 * item shown when we switch back to
					 * the category. */

					if(pmi != NULL)
						pmitem_uninit(&pmi);
					curcat = newcat;
					cli_catstats(curcat, 0);

					cli_lastread = 0;
					cli_thissess = 0;

					/* Don't erase history on category
					 * change.
					(void) cli_pmihist_clear();
					 */

					dispfmt = CLI_DISPFMT_LONG;
				}
			} else {
				printf("No category specified.\n");
			}
		} else
		if(!xstrcmp(line, "pp")) {
			/* Switch to next non-zero category. */
			newcat = cli_getnextnecat(curcat);
			if(newcat == NULL) {
				printf("No more non-empty categories.\n");
			} else {
				/* If currently viewing an item, then
				 * "forget" about it, so it's the first
				 * item shown when we switch back to
				 * the category. */

				if(pmi != NULL)
					pmitem_uninit(&pmi);
				curcat = newcat;
				cli_catstats(curcat, 0);

				cli_lastread = 0;
				cli_thissess = 0;

				/* Don't erase history on category
				 * change.
				(void) cli_pmihist_clear();
				 */

				dispfmt = CLI_DISPFMT_LONG;
			}
		} else
		if(xstrbeginswith(line, "ff ") && xstrlen(line + 3) > 0) {
			ffcnt = atoi(line + 3);
			if(ffcnt > 0) {
				printf("Fast forwarding %d items...\n", ffcnt);
				for(i = 0; i < ffcnt; ++i) {
					if((i % 20) == 0)
						printf("%d\n", i);

					if(pmi != NULL) {
						ret = pmitem_delete(pmi);
						if(ret != 0) {
							printf("Couldn't"
							    " delete "
							    "current item.\n");
						}
						pmi = NULL;
					}

					ret = pmitem_loadnext(curcat, &pmi);
					if(ret != 0) {
						if(ret == ERANGE)
							printf("No more visible"
							    " items.\n");
						else
						if(ret == ENOENT)
							printf("Category is empty.\n");
						else	
							printf("Couldn't load next"
							    " item.\n");
						break;
					}
				}
				ret = cli_catstats(curcat, 0);
				if(ret != 0) {
					printf("Could not print stats.\n");
				}
			}
		} else
		if(!xstrcmp(line, "disp long")) {
			dispfmt = CLI_DISPFMT_LONG;
			printf("Display format set to long\n");
		} else
		if(!xstrcmp(line, "disp short")) {
			dispfmt = CLI_DISPFMT_SHORT;
			printf("Display format set to short\n");
		} else
		if(!xstrcmp(line, "quit") || !xstrcmp(line, "exit")) {
			break;
		} else {
			printf("Unknown command.\n");
		}

		xfree(&line);
	}

	printf("\n");

	(void) cli_pmihist_uninit();

	return 0;
}


int
cli_openurl(bstr_t *url)
{
	bstr_t	*cmd;
	int	err;
	int	ret;

	if(bstrempty(url))
		return EINVAL;

	err = 0;

	cmd = binit();
	if(cmd == NULL) {
		blogf("Could not initialize cmd");
		err = ENOMEM;
		goto end_label;
	}

	bprintf(cmd, "open -g \"%s\"", bget(url));

	ret = system(bget(cmd));
	if(ret != 0) {
		blogf("Couldn't execute command: %d", ret);
		err = ret;
		goto end_label;
	}

end_label:
	
	if(cmd != NULL)
		buninit(&cmd);

	return err;
}


int
cli_catstats(pmcat_t *cat, int detail)
{
	int		err;
	int		ret;
	pmcat_stats_t	stats;
	int		stattype;

	if(cat == NULL)
		return EINVAL;

	err = 0;

	if(detail == 0)
		stattype = PMCAT_STATTYPE_QUICK;
	else
		stattype = PMCAT_STATTYPE_FULL;

	memset(&stats, 0, sizeof(pmcat_stats_t));
	ret = pmcat_getstats(cat, stattype, &stats);
	if(ret != 0) {
		blogf("Couldn't get category stats");
		err = ret;
		goto end_label;
	}

	printf("%s: %d overall, %d visible, oldest is %d hours old\n",
	    bget(cat->pc_name), stats.ps_all_cnt, stats.ps_visible_cnt,
	    stats.ps_oldesthours);

	if(detail != 0) {
		printf("Last 1 day  : %d added, %d seen (net=%d)\n",
		    stats.ps_1dadded_cnt, stats.ps_1dseen_cnt, 
		    stats.ps_1dadded_cnt - stats.ps_1dseen_cnt);
		printf("Last 2 days : %d added, %d seen (net=%d)\n",
		    stats.ps_2dadded_cnt, stats.ps_2dseen_cnt, 
		    stats.ps_2dadded_cnt - stats.ps_2dseen_cnt);
		printf("Last 7 days : %d added, %d seen (net=%d)\n",
		    stats.ps_7dadded_cnt, stats.ps_7dseen_cnt, 
		    stats.ps_7dadded_cnt - stats.ps_7dseen_cnt);
		printf("Last 30 days: %d added, %d seen (net=%d)\n",
		    stats.ps_30dadded_cnt, stats.ps_30dseen_cnt, 
		    stats.ps_30dadded_cnt - stats.ps_30dseen_cnt);
		printf("Last 90 days: %d added, %d seen (net=%d)\n",
		    stats.ps_90dadded_cnt, stats.ps_90dseen_cnt, 
		    stats.ps_90dadded_cnt - stats.ps_90dseen_cnt);
	}

end_label:

	return err;
}


int
cli_listcats(pmcat_t *curcat)
{
	pmcat_t		*pmc;

	for(pmc = (pmcat_t *)barr_begin(pmcats);
	    pmc < (pmcat_t *)barr_end(pmcats); ++pmc) {
		if(pmc == curcat)
			printf("*");
		else
			printf(" ");
		cli_catstats(pmc, 0);
	}

	return 0;
}


pmcat_t *
cli_getnextnecat(pmcat_t *curcat)
{
	/*
	 * Return the next non-empty category.
	 */
	pmcat_t	*pmc;

	/* Find current cat */
	for(pmc = (pmcat_t *)barr_begin(pmcats);
	    pmc < (pmcat_t *)barr_end(pmcats); ++pmc) {
		if(pmc == curcat)
			break;
	}

	/* Now find the next non-empty. */
	while(1) {
		++pmc;
		
		/* Wrap around the end of the array to restart the category
		 * list. */
		if(pmc == (pmcat_t *)barr_end(pmcats))
			pmc = (pmcat_t *)barr_begin(pmcats);

		if(pmc == curcat)	/* No more empty categories */
			return NULL;

		if(pmcat_getleft(pmc) > 0)
			break;

		printf("Skipping empty category '%s'\n", bget(pmc->pc_name));
	}

	return pmc;
}


int
cli_pmihist_init(void)
{
	if(cli_pmihist != NULL)
		return EEXIST;

	cli_pmihist = blist_init();

	if(cli_pmihist == NULL)
		return ENOEXEC;

	return 0;
}


int
cli_pmihist_uninit(void)
{
	if(cli_pmihist == NULL)
		return ENOEXEC;

	(void) cli_pmihist_clear();
	blist_uninit(&cli_pmihist);

	return 0;
}


int
cli_pmihist_clear(void)
{
	pmitem_t	*pmi;

	while((pmi = (pmitem_t *) blist_lpop(cli_pmihist))) {
		if(pmi != NULL) {
			pmitem_uninit(&pmi);
		} else {
			blogf("History element was NULL?");
		}
	}
	
	return 0;
}


int
cli_pmihist_add(pmitem_t *pmi)
{
	int		ret;
	pmitem_t	*trimpmi;

	if(cli_pmihist == 0)
		return ENOEXEC;
	if(pmi == NULL)
		return EINVAL;

	/* Trim if needed. */
	while(cli_pmihist->bl_cnt >= CLI_PMIHIST_MAXCNT) {
		trimpmi = (pmitem_t *)blist_rpop(cli_pmihist);
		if(trimpmi == NULL) {
			blogf("History element was NULL?");
		} else {
			pmitem_uninit(&trimpmi);
		}
	}

	ret = blist_lpush(cli_pmihist, (void *) pmi);
	if(ret != 0) {
		blogf("Couldn't add pmi to history!");
		return ret;
	}

	return 0;
}


pmitem_t *
cli_pmihist_getidx(int idx)
{
	blelem_t	*elem;

	if(cli_pmihist == NULL)
		return NULL;

	elem = blist_getidx(cli_pmihist, idx);
	if(elem != NULL && elem->be_data != NULL)
		return (pmitem_t *) elem->be_data;

	return NULL;
}

#endif
