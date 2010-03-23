/* vi:set ts=8 sts=4 sw=4:
 *
 * CSCOPE support for Vim added by Andy Kahn <kahn@zk3.dec.com>
 *
 * The basic idea/structure of cscope for Vim was borrowed from Nvi.  There
 * might be a few lines of code that look similar to what Nvi has.
 */

#include "vim.h"

#if defined(USE_CSCOPE) || defined(PROTO)

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "if_cscope.h"


#define CS_USAGE_MSG(x) { \
    char buf[64]; \
    (void)sprintf(buf, "Usage: cs[cope] %s", cs_cmds[(x)].usage); \
    (void)EMSG(buf); \
}


static int	    cs_add __ARGS((EXARG *eap));
static int	    cs_add_common __ARGS((char *, char *, char *));
static int	    cs_check_for_connections __ARGS((void));
static int	    cs_check_for_tags __ARGS((void));
static int	    cs_cnt_connections __ARGS((void));
static int	    cs_cnt_matches __ARGS((int idx));
static char *	    cs_create_cmd __ARGS((char *csoption, char *pattern));
static int	    cs_create_connection __ARGS((int i));
static int	    cs_find __ARGS((EXARG *eap));
static int	    cs_find_common __ARGS((char *opt, char *pat, int, int ));
static int	    cs_help __ARGS((EXARG *eap));
static void	    cs_init __ARGS((void));
static void	    clear_csinfo __ARGS((int i));
static int	    cs_insert_filelist __ARGS((char *, char *, char *,
			struct stat *));
static int	    cs_kill __ARGS((EXARG *eap));
static cscmd_t *    cs_lookup_cmd __ARGS((EXARG *eap));
static char *	    cs_make_vim_style_matches __ARGS((char *, char *,
			char *, char *));
static char *	    cs_manage_matches __ARGS((char **, char **, int, mcmd_e));
static void	    cs_parse_results __ARGS((char *, int , int *, char ***,
			char ***, int *));
static void	    cs_print_tags_priv __ARGS((char **, char **, int));
static int	    cs_read_prompt __ARGS((int ));
static void	    cs_release_csp __ARGS((int, int freefnpp));
static int	    cs_reset __ARGS((EXARG *eap));
static char *	    cs_resolve_file __ARGS((int, char *));
static int	    cs_show __ARGS((EXARG *eap));


static csinfo_t	    csinfo[CSCOPE_MAX_CONNECTIONS];
static cscmd_t	    cs_cmds[] =
{
    { "add",	cs_add,
		"Add a new database",     "add file|dir [pre-path] [flags]" },
    { "find",	cs_find,
		"Query for a pattern",    FIND_USAGE },
    { "help",	cs_help,
		"Show this message",      "help" },
    { "kill",	cs_kill,
		"Kill a connection",      "kill #" },
    { "reset",	cs_reset,
		"Reinit all connections", "reset" },
    { "show",	cs_show,
		"Show connections",       "show" },
    { NULL }
};



/*
 * PUBLIC functions
 ****************************************************************************/

/*
 * PUBLIC: do_cscope
 *
 * find the command, print help if invalid, and the then call the
 * corresponding command function
 */
    void
do_cscope(eap)
    EXARG *eap;
{
    cscmd_t *cmdp;

    cs_init();
    if ((cmdp = cs_lookup_cmd(eap)) == NULL)
    {
	cs_help(eap);
	return;
    }

    cmdp->func(eap);
} /* do_cscope */


/*
 * PUBLIC: do_cstag
 *
 */
    void
do_cstag(eap)
    EXARG *eap;
{
    int ret = FALSE;

    cs_init();

    if (eap->arg == NULL || strlen((const char *)(eap->arg)) == 0)
    {
	(void)EMSG("Usage: cstag <ident>");
	return;
    }

    switch (p_csto)
    {
    case 0 :
	if (cs_check_for_connections())
	{
	    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit, FALSE);
	    if (ret == FALSE)
	    {
		cs_free_tags();
		if (msg_col)
		    msg_putchar('\n');

		if (cs_check_for_tags())
		    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	    }
	}
	else if (cs_check_for_tags())
	{
	    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	}
	break;
    case 1 :
	if (cs_check_for_tags())
	{
	    ret = do_tag(eap->arg, DT_JUMP, 0, eap->forceit, FALSE);
	    if (ret == FALSE)
	    {
		if (msg_col)
		    msg_putchar('\n');

		if (cs_check_for_connections())
		{
		    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit,
					 FALSE);
		    if (ret == FALSE)
			cs_free_tags();
		}
	    }
	}
	else if (cs_check_for_connections())
	{
	    ret = cs_find_common("g", (char *)(eap->arg), eap->forceit, FALSE);
	    if (ret == FALSE)
		cs_free_tags();
	}
	break;
    default :
	break;
    }

    if (!ret)
    {
	(void)EMSG("cstag: tag not found");
	g_do_tagpreview = 0;
    }

} /* do_cscope */


/*
 * PUBLIC: cs_find
 *
 * this simulates a vim_fgets(), but for cscope, returns the next line
 * from the cscope output.  should only be called from find_tags()
 *
 * returns TRUE if eof, FALSE otherwise
 */
    int
cs_fgets(buf, size)
    char_u	*buf;
    int		size;
{
    char *p;

    if ((p = cs_manage_matches(NULL, NULL, -1, Get)) == NULL)
	return TRUE;

    if (strlen(p) > size)
    {
	strncpy((char *)buf, p, size - 1);
	buf[size] = '\0';
    }
    else
	(void)strcpy((char *)buf, p);

    return FALSE;
} /* cs_fgets */


/*
 * PUBLIC: cs_free_tags
 *
 * called only from do_tag(), when popping the tag stack
 */
    void
cs_free_tags()
{
    cs_manage_matches(NULL, NULL, -1, Free);
}


/*
 * PUBLIC: cs_print_tags
 *
 * called from do_tag()
 */
    void
cs_print_tags()
{
    cs_manage_matches(NULL, NULL, -1, Print);
}


/*
 * PRIVATE functions
 ****************************************************************************/

/*
 * PRIVATE: cs_add
 *
 * add cscope database or a directory name (to look for cscope.out)
 * the the cscope connection list
 *
 * MAXPATHL 256
 */
/* ARGSUSED */
    static int
cs_add(eap)
    EXARG *eap;
{
    char *fname, *ppath, *flags = NULL;

    if ((fname = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	CS_USAGE_MSG(Add);
	return CSCOPE_FAILURE;
    }
    if ((ppath = strtok((char *)NULL, (const char *)" ")) != NULL)
	flags = strtok((char *)NULL, (const char *)" ");

    return cs_add_common(fname, ppath, flags);
}


/*
 * PRIVATE: cs_add_common
 *
 * the common routine to add a new cscope connection.  called by
 * cs_add() and cs_reset().  i really don't like to do this, but this
 * routine uses a number of goto statements.
 */
    static int
cs_add_common(arg1, arg2, flags)
    char *arg1;	    /* filename - may contain environment variables */
    char *arg2;	    /* prepend path - may contain environment variables */
    char *flags;
{
    struct stat statbuf;
    int ret;
    char *fname = NULL;
    char *ppath = NULL;
    char *buf = NULL;
    int i;

    /* buf is for general purpose msgs.  let's hope MAXPATHL*2 is big enough */
    if ((buf = (char *)alloc(MAXPATHL * 2)) == NULL)
    {
	(void)EMSG("cs_add_common: alloc fail #1");
	goto add_err;
    }

    /* get the filename (arg1), expand it, and try to stat it */
    if ((fname = (char *)alloc(MAXPATHL+1)) == NULL)
    {
	(void)EMSG("cs_add_common: alloc fail #2");
	goto add_err;
    }

    expand_env((char_u *)arg1, (char_u *)fname, MAXPATHL);
    ret = stat(fname, &statbuf);
    if (ret < 0)
    {
	if (p_csverbose)
	{
	    (void)sprintf(buf, "stat(%s) error: %d", fname, errno);
	    (void)EMSG(buf);
	}
	goto add_err;
    }

    /* get the prepend path (arg2), expand it, and try to stat it */
    if (arg2 != NULL)
    {
	struct stat statbuf2;

	if ((ppath = (char *)alloc(MAXPATHL+1)) == NULL)
	{
	    (void)EMSG("cs_add_common: alloc fail #3");
	    goto add_err;
	}

	expand_env((char_u *)arg2, (char_u *)ppath, MAXPATHL);
	ret = stat(ppath, &statbuf2);
	if (ret < 0)
	{
	    if (p_csverbose)
	    {
		(void)sprintf(buf, "stat(%s) error: %d", fname, errno);
		(void)EMSG(buf);
	    }
	    goto add_err;
	}
    }

    /* if filename is a directory, append the cscope database name to it */
    if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
    {
	char *fname2;
	fname2 = (char *)alloc(strlen(CSCOPE_DBFILE) + strlen(fname) + 2);
	if (fname2 == NULL)
	{
	    (void)EMSG("cs_add_common: alloc fail #4");
	    goto add_err;
	}

	while (fname[strlen(fname)-1] == '/')
	{
	    fname[strlen(fname)-1] = '\0';
	    if (strlen(fname) == 0)
		break;
	}
	if (fname[0] == '\0')
	    (void)sprintf(fname2, "/%s", CSCOPE_DBFILE);
	else
	    (void)sprintf(fname2, "%s/%s", fname, CSCOPE_DBFILE);

	ret = stat(fname2, &statbuf);
	if (ret < 0)
	{
	    if (p_csverbose)
	    {
		(void)sprintf(buf, "stat(%s) error: %d", fname2, errno);
		(void)EMSG(buf);
	    }
	    vim_free(fname2);
	    goto add_err;
	}

	i = cs_insert_filelist(fname2, ppath, flags, &statbuf);
	if (p_csverbose)
	    (void)sprintf(buf, "Added cscope database %s", fname2);
	vim_free(fname2);
    }
    else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
    {
	i = cs_insert_filelist(fname, ppath, flags, &statbuf);
	if (p_csverbose)
	    (void)sprintf(buf, "Added cscope database %s", fname);
    }
    else
    {
	if (p_csverbose)
	{
	    (void)sprintf(buf,
		"%s is not a directory or a valid cscope database", fname);
	    (void)EMSG(buf);
	}
	goto add_err;
    }

    if (i != -1)
    {
	(void)cs_create_connection(i);
	(void)cs_read_prompt(i);
	if (p_csverbose)
	{
	    msg_clr_eos();
	    MSG_PUTS_ATTR(buf, highlight_attr[HLF_R]);
	}
    }

    vim_free(fname);
    vim_free(ppath);
    vim_free(buf);
    return CSCOPE_SUCCESS;

add_err:
    vim_free(fname);
    vim_free(ppath);
    vim_free(buf);
    return CSCOPE_FAILURE;
} /* cs_add_common */


    static int
cs_check_for_connections()
{
    return (cs_cnt_connections() > 0);
} /* cs_check_for_connections */


    static int
cs_check_for_tags()
{
    return (p_tags != NULL && p_tags[0] != '\0');
} /* cs_check_for_tags */


/*
 * PRIVATE: cs_cnt_connections
 *
 * count the number of cscope connections
 */
    static int
cs_cnt_connections()
{
    short i;
    short cnt = 0;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (csinfo[i].fname != NULL)
	    cnt++;
    }
    return cnt;
} /* cs_cnt_connections */


/*
 * PRIVATE: cs_cnt_matches
 *
 * count the number of matches for a given cscope connection.
 */
    static int
cs_cnt_matches(idx)
    int idx;
{
    char *stok;
    char buf[2048];
    int nlines;

    for (;;)
    {
	if (!fgets(buf, sizeof(buf), csinfo[idx].fr_fp))
	{
	    if (feof(csinfo[idx].fr_fp))
		errno = EIO;
	    (void)sprintf(buf, "error reading cscope connection %d", idx);
	    (void)EMSG(buf);
	    return -1;
	}

	/*
	 * If the database is out of date, or there's some other problem,
	 * cscope will output error messages before the number-of-lines output.
	 * Display/discard any output that doesn't match what we want.
	 */
	if ((stok = strtok(buf, (const char *)" ")) == NULL)
	    continue;
	if (strcmp((const char *)stok, "cscope:"))
	    continue;

	if ((stok = strtok(NULL, (const char *)" ")) == NULL)
	    continue;
	nlines = atoi(stok);
	if (nlines < 0)
	{
	    nlines = 0;
	    break;
	}

	if ((stok = strtok(NULL, (const char *)" ")) == NULL)
	    continue;
	if (strncmp((const char *)stok, "lines", 5))
	    continue;

	break;
    }

    return nlines;
} /* cs_cnt_matches */


/*
 * PRIVATE: cs_create_cmd
 *
 * Creates the actual cscope command query from what the user entered.
 */
    static char *
cs_create_cmd(csoption, pattern)
    char *csoption;
    char *pattern;
{
    char *cmd;
    short search;

    switch (csoption[0])
    {
    case '0' : case 's' :
	search = 0;
	break;
    case '1' : case 'g' :
	search = 1;
	break;
    case '2' : case 'd' :
	search = 2;
	break;
    case '3' : case 'c' :
	search = 3;
	break;
    case '4' : case 't' :
	search = 4;
	break;
    case '6' : case 'e' :
	search = 6;
	break;
    case '7' : case 'f' :
	search = 7;
	break;
    case '8' : case 'i' :
	search = 8;
	break;
    default :
	(void)EMSG("unknown cscope search type");
	CS_USAGE_MSG(Find);
	return NULL;
    }

    if ((cmd = (char *) alloc(strlen(pattern) + 2)) == NULL)
	return NULL;

    (void)sprintf(cmd, "%d%s", search, pattern);

    return cmd;
} /* cs_create_cmd */


/*
 * PRIVATE: cs_create_connection
 *
 * This piece of code was taken/adapted from nvi.  do we need to add
 * the BSD license notice?
 */
    static int
cs_create_connection(i)
    int i;
{
    int to_cs[2], from_cs[2], len;
    char *prog, *cmd, *ppath = NULL;

    /*
     * Cscope reads from to_cs[0] and writes to from_cs[1]; vi reads from
     * from_cs[0] and writes to to_cs[1].
     */
    to_cs[0] = to_cs[1] = from_cs[0] = from_cs[1] = -1;
    if (pipe(to_cs) < 0 || pipe(from_cs) < 0)
	goto err;

    switch (csinfo[i].pid = fork())
    {
    case -1:
err:
	if (to_cs[0] != -1)
	    (void)close(to_cs[0]);
	if (to_cs[1] != -1)
	    (void)close(to_cs[1]);
	if (from_cs[0] != -1)
	    (void)close(from_cs[0]);
	if (from_cs[1] != -1)
	    (void)close(from_cs[1]);
	(void)EMSG("Could not create cscope pipes");
	return CSCOPE_FAILURE;
    case 0:				/* child: run cscope. */
	if (dup2(to_cs[0], STDIN_FILENO) == -1)
	    perror("cs_create_connection 1");
	if (dup2(from_cs[1], STDOUT_FILENO) == -1)
	    perror("cs_create_connection 2");
	if (dup2(from_cs[1], STDERR_FILENO) == -1)
	    perror("cs_create_connection 3");

	/* close unused */
	(void)close(to_cs[1]);
	(void)close(from_cs[0]);

	/* expand the cscope exec for env var's */
	if ((prog = (char *)alloc(MAXPATHL + 1)) == NULL)
	{
	    perror("cs_create_connection: alloc fail #1");
	    return CSCOPE_FAILURE;
	}
	expand_env((char_u *)p_csprg, (char_u *)prog, MAXPATHL);

	/* alloc space to hold the cscope command */
	len = strlen(prog) + strlen(csinfo[i].fname) + 32;
	if (csinfo[i].ppath)
	{
	    /* expand the prepend path for env var's */
	    if ((ppath = (char *)alloc(MAXPATHL + 1)) == NULL)
	    {
		perror("cs_create_connection: alloc fail #2");
		vim_free(prog);
		return CSCOPE_FAILURE;
	    }
	    expand_env((char_u *)csinfo[i].ppath, (char_u *)ppath, MAXPATHL);

	    len += strlen(ppath);
	}

	if (csinfo[i].flags)
	    len += strlen(csinfo[i].flags);

	if ((cmd = (char *)alloc(len)) == NULL)
	{
	    perror("cs_create_connection: alloc fail #3");
	    vim_free(prog);
	    vim_free(ppath);
	    return CSCOPE_FAILURE;
	}

	/* run the cscope command; is there execl for non-unix systems? */
	(void)sprintf(cmd, "exec %s -dl -f %s", prog, csinfo[i].fname);
	if (csinfo[i].ppath != NULL)
	{
	    (void)strcat(cmd, " -P");
	    (void)strcat(cmd, csinfo[i].ppath);
	}
	if (csinfo[i].flags != NULL)
	{
	    (void)strcat(cmd, " ");
	    (void)strcat(cmd, csinfo[i].flags);
	}
	vim_free(prog);
	vim_free(ppath);

	if (execl("/bin/sh", "sh", "-c", cmd, NULL) == -1)
	    perror("cs_create_connection exec failed");

	exit (127);
	/* NOTREACHED */
    default:	/* parent. */
	/*
	 * Save the file descriptors for later duplication, and
	 * reopen as streams.
	 */
	if ((csinfo[i].to_fp = fdopen(to_cs[1], "w")) == NULL)
	    perror("cs_create_connection: fdopen for to_fp failed");
	if ((csinfo[i].fr_fp = fdopen(from_cs[0], "r")) == NULL)
	    perror("cs_create_connection: fdopen for fr_fp failed");

	/* close unused */
	(void)close(to_cs[0]);
	(void)close(from_cs[1]);

	break;
    }
    return CSCOPE_SUCCESS;
} /* cs_create_connection */


/*
 * PRIVATE: cs_find
 *
 * query cscope using command line interface.  parse the output and use tselect
 * to allow choices.  like Nvi, creates a pipe to send to/from query/cscope.
 *
 * returns TRUE if we jump to a tag or abort, FALSE if not.
 */
    static int
cs_find(eap)
    EXARG *eap;
{
    char *opt, *pat;

    if (cs_check_for_connections() == FALSE)
    {
	(void)EMSG("no cscope connections");
	return FALSE;
    }

    if ((opt = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	CS_USAGE_MSG(Find);
	return FALSE;
    }

    pat = opt + strlen(opt) + 1;
    if (pat == NULL || (pat != NULL && pat[0] == '\0'))
    {
	CS_USAGE_MSG(Find);
	return FALSE;
    }

    return cs_find_common(opt, pat, eap->forceit, TRUE);
} /* cs_find */


/*
 * PRIVATE: cs_find_common
 *
 * common code for cscope find, shared by cs_find() and do_cstag()
 */
    static int
cs_find_common(opt, pat, forceit, verbose)
    char *opt;
    char *pat;
    int forceit;
    int verbose;
{
    int i;
    char *cmd;
    char **matches, **contexts;
    int nummatches[CSCOPE_MAX_CONNECTIONS], totmatches, matched;

    /* create the actual command to send to cscope */
    cmd = cs_create_cmd(opt, pat);
    if (cmd == NULL)
	return FALSE;

    /* send query to all open connections, then count the total number
     * of matches so we can alloc matchesp all in one swell foop
     */
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	nummatches[i] = 0;
    totmatches = 0;
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (csinfo[i].fname == NULL)
	    continue;

	/* send cmd to cscope */
	(void)fprintf(csinfo[i].to_fp, "%s\n", cmd);
	(void)fflush(csinfo[i].to_fp);

	nummatches[i] = cs_cnt_matches(i);

	if (nummatches[i] > -1)
	    totmatches += nummatches[i];

	if (nummatches[i] == 0)
	    (void)cs_read_prompt(i);
    }
    vim_free(cmd);

    if (totmatches == 0)
    {
	char *nf = "no matches found in cscope connections";
	char *buf;

	if (!verbose)
	    return FALSE;

	buf = (char *) alloc(strlen(opt) + strlen(pat) + strlen(nf) + 1);
	if (buf == NULL)
	    (void)EMSG(nf);
	else
	{
	    sprintf(buf, "no matches found for cscope query %s of %s",
		opt, pat);
	    (void)EMSG(buf);
	    vim_free(buf);
	}
	return FALSE;
    }

    /* read output */
    cs_parse_results((char *)pat, totmatches, nummatches, &matches, &contexts,
		     &matched);
    if (matches == NULL)
	return FALSE;

    (void)cs_manage_matches(matches, contexts, totmatches, Store);

    return do_tag((char_u *)pat, DT_CSCOPE, 0, forceit, verbose);

} /* cs_find_common */


/*
 * PRIVATE: cs_help
 *
 * print help
 */
/* ARGSUSED */
    static int
cs_help(eap)
    EXARG *eap;
{
    char buf[MAXPATHL];
    cscmd_t *cmdp = cs_cmds;

    (void)MSG_PUTS("cscope commands:\n");
    while (cmdp->name != NULL)
    {
	(void)sprintf(buf, "%-5s: %-30s (Usage: %s)\n",
		      cmdp->name, cmdp->help, cmdp->usage);
	(void)MSG_PUTS(buf);

	if (strcmp(cmdp->name, "find") == 0)
	    MSG_PUTS(FIND_HELP);
	cmdp++;
    }

    wait_return(TRUE);
    return 0;
} /* cs_help */


/*
 * PRIVATE: cs_init
 *
 * initialize cscope structure if not already
 */
    static void
cs_init()
{
    short i;
    static bool_t init_already = FALSE;

    if (init_already)
	return;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	clear_csinfo(i);

    init_already = TRUE;
} /* cs_init */

    static void
clear_csinfo(i)
    int	    i;
{
    csinfo[i].fname  = NULL;
    csinfo[i].ppath  = NULL;
    csinfo[i].flags  = NULL;
    csinfo[i].pid    = -1;
    csinfo[i].st_dev = (dev_t)0;
    csinfo[i].st_ino = (ino_t)0;
    csinfo[i].fr_fp  = NULL;
    csinfo[i].to_fp  = NULL;
}

/*
 * PRIVATE: cs_insert_filelist
 *
 * insert a new cscope database filename into the filelist
 */
    static int
cs_insert_filelist(fname, ppath, flags, sb)
    char *fname;
    char *ppath;
    char *flags;
    struct stat *sb;
{
    short i;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (csinfo[i].fname &&
	    csinfo[i].st_dev == sb->st_dev && csinfo[i].st_ino == sb->st_ino)
	{
	    (void)EMSG("duplicate cscope database not added");
	    return -1;
	}

	if (csinfo[i].fname == NULL)
	    break;
    }

    if (i == CSCOPE_MAX_CONNECTIONS)
    {
	(void)EMSG("maximum number of cscope connections reached");
	return -1;
    }

    if ((csinfo[i].fname = (char *) alloc(strlen(fname)+1)) == NULL)
	return -1;

    (void)strcpy(csinfo[i].fname, (const char *)fname);

    if (ppath != NULL)
    {
	if ((csinfo[i].ppath = (char *) alloc(strlen(ppath) + 1)) == NULL)
	{
	    vim_free(csinfo[i].fname);
	    csinfo[i].fname = NULL;
	    return -1;
	}
	(void)strcpy(csinfo[i].ppath, (const char *)ppath);
    } else
	csinfo[i].ppath = NULL;

    if (flags != NULL)
    {
	if ((csinfo[i].flags = (char *) alloc(strlen(flags) + 1)) == NULL)
	{
	    vim_free(csinfo[i].fname);
	    vim_free(csinfo[i].ppath);
	    csinfo[i].fname = NULL;
	    csinfo[i].ppath = NULL;
	    return -1;
	}
	(void)strcpy(csinfo[i].flags, (const char *)flags);
    } else
	csinfo[i].flags = NULL;

    csinfo[i].st_dev = sb->st_dev;
    csinfo[i].st_ino = sb->st_ino;

    return i;
} /* cs_insert_filelist */


/*
 * PRIVATE: cs_lookup_cmd
 *
 * find cscope command in command table
 */
    static cscmd_t *
cs_lookup_cmd(exp)
    EXARG *exp;
{
    cscmd_t *cmdp;
    char *stok;
    size_t len;

    if (exp->arg == NULL)
	return NULL;

    if ((stok = strtok((char *)(exp->arg), (const char *)" ")) == NULL)
	return NULL;

    len = strlen(stok);
    for (cmdp = cs_cmds; cmdp->name != NULL; ++cmdp)
    {
	if (strncmp((const char *)(stok), cmdp->name, len) == 0)
	    return (cmdp);
    }
    return NULL;
} /* cs_lookup_cmd */


/*
 * PRIVATE: cs_kill
 *
 * nuke em
 */
/* ARGSUSED */
    static int
cs_kill(eap)
    EXARG *eap;
{
    char *stok, *printbuf = NULL;
    short i;

    if ((stok = strtok((char *)NULL, (const char *)" ")) == NULL)
    {
	CS_USAGE_MSG(Kill);
	return CSCOPE_FAILURE;
    }

    if (strlen(stok) < 2 && isdigit((int)(stok[0])))
    {
	i = atoi(stok);
	printbuf = (char *)alloc(strlen(stok) + 32);
    }
    else
    {
	/* It must be part of a name.  We will try to find a match
	 * within all the names in the csinfo data structure
	 */
	for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	{
	    if (csinfo[i].fname != NULL && strstr(csinfo[i].fname, stok))
	    {
		printbuf = (char *)alloc(strlen(csinfo[i].fname) + 32);
		break;
	    }
	}
    }

    if (i >= CSCOPE_MAX_CONNECTIONS || i < 0 || csinfo[i].fname == NULL)
    {
	if (printbuf == NULL)
	    (void)EMSG("cscope connection not found");
	else
	{
	    sprintf(printbuf, "cscope connection %s not found", stok);
	    (void)EMSG(printbuf);
	}
    }
    else
    {
	if (printbuf == NULL)
	    MSG_PUTS_ATTR("cscope connection closed",
					    highlight_attr[HLF_R] | MSG_HIST);
	else
	{
	    cs_release_csp(i, TRUE);
	    msg_clr_eos();
	    sprintf(printbuf, "cscope connection %s closed", stok);
	    MSG_PUTS_ATTR(printbuf, highlight_attr[HLF_R] | MSG_HIST);
	}
    }

    vim_free(printbuf);
    return 0;
} /* cs_kill */


/*
 * PRIVATE: cs_make_vim_style_matches
 *
 * convert the cscope output into into a ctags style entry (as might be found
 * in a ctags tags file).  there's one catch though: cscope doesn't tell you
 * the type of the tag you are looking for.  for example, in Darren Hiebert's
 * ctags (the one that comes with vim), #define's use a line number to find the
 * tag in a file while function definitions use a regexp search pattern.
 *
 * i'm going to always use the line number because cscope does something
 * quirky (and probably other things i don't know about):
 *
 *     if you have "#  define" in your source file, which is
 *     perfectly legal, cscope thinks you have "#define".  this
 *     will result in a failed regexp search. :(
 *
 * besides, even if this particular case didn't happen, the search pattern
 * would still have to be modified to escape all the special regular expression
 * characters to comply with ctags formatting.
 */
    static char *
cs_make_vim_style_matches(fname, slno, search, tagstr)
    char *fname;
    char *slno;
    char *search;
    char *tagstr;
{
    /* vim style is ctags:
     *
     *	    <tagstr>\t<filename>\t<linenum_or_search>"\t<extra>
     *
     * but as mentioned above, we'll always use the line number and
     * put the search pattern (if one exists) as "extra"
     *
     * buf is used as part of vim's method of handling tags, and
     * (i think) vim frees it when you pop your tags and get replaced
     * by new ones on the tag stack.
     */
    char *buf;
    int amt;

    if (search != NULL)
    {
	amt = strlen(fname) + strlen(slno) + strlen(tagstr) + strlen(search)+6;
	if ((buf = (char *) alloc(amt)) == NULL)
	    return NULL;

	(void)sprintf(buf, "%s\t%s\t%s;\"\t%s", tagstr, fname, slno, search);
    }
    else
    {
	amt = strlen(fname) + strlen(slno) + strlen(tagstr) + 5;
	if ((buf = (char *) alloc(amt)) == NULL)
	    return NULL;

	(void)sprintf(buf, "%s\t%s\t%s;\"", tagstr, fname, slno);
    }

    return buf;
} /* cs_make_vim_style_matches */


/*
 * PRIVATE: cs_manage_matches
 *
 * this is kind of hokey, but i don't see an easy way round this..
 *
 * Store: keep a ptr to the (malloc'd) memory of matches originally
 * generated from cs_find().  the matches are originally lines directly
 * from cscope output, but transformed to look like something out of a
 * ctags.  see cs_make_vim_style_matches for more details.
 *
 * Get: used only from cs_fgets(), this simulates a vim_fgets() to return
 * the next line from the cscope output.  it basically keeps track of which
 * lines have been "used" and returns the next one.
 *
 * Free: frees up everything and resets
 *
 * Print: prints the tags
 */
    static char *
cs_manage_matches(matches, contexts, totmatches, cmd)
    char **matches;
    char **contexts;
    int totmatches;
    mcmd_e cmd;
{
    static char **mp = NULL;
    static char **cp = NULL;
    static int cnt = -1;
    static int next = -1;
    char *p = NULL;

    switch (cmd)
    {
    case Store:
	assert(matches != NULL);
	assert(totmatches > 0);
	if (mp != NULL || cp != NULL)
	    (void)cs_manage_matches(NULL, NULL, -1, Free);
	mp = matches;
	cp = contexts;
	cnt = totmatches;
	next = 0;
	break;
    case Get:
	if (next >= cnt)
	    return NULL;

	p = mp[next];
	next++;
	break;
    case Free:
	if (mp != NULL)
	{
	    if (cnt > 0)
		while (cnt--)
		{
		    vim_free(mp[cnt]);
		    if (cp != NULL)
			vim_free(cp[cnt]);
		}
	    vim_free(mp);
	    vim_free(cp);
	}
	mp = NULL;
	cp = NULL;
	cnt = 0;
	next = 0;
	break;
    case Print:
	cs_print_tags_priv(mp, cp, cnt);
	break;
    default:	/* should not reach here */
	(void)EMSG("fatal error in cs_manage_matches");
	return NULL;
    }

    return p;
} /* cs_manage_matches */


/*
 * PRIVATE: cs_parse_results
 *
 * parse cscope output and calls cs_make_vim_style_matches to convert
 * into ctags format
 */
    static void
cs_parse_results(tagstr, totmatches, nummatches_a, matches_p, cntxts_p, matched)
    char *tagstr;
    int totmatches;
    int *nummatches_a;
    char ***matches_p;
    char ***cntxts_p;
    int *matched;
{
    int i, j;
    int ch;
    char buf[2048];
    char *name, *search, *p, *slno;
    int totsofar = 0;
    char **matches = NULL;
    char **cntxts = NULL;
    char *fullname;
    char *cntx;

    assert(totmatches > 0);

    if ((matches = (char **) alloc(sizeof(char *) * totmatches)) == NULL)
	goto parse_out;
    if ((cntxts = (char **) alloc(sizeof(char *) * totmatches)) == NULL)
	goto parse_out;

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (nummatches_a[i] < 1)
	    continue;

	for (j = 0; j < nummatches_a[i]; j++)
	{
	    if (fgets(buf, sizeof(buf), csinfo[i].fr_fp) == NULL)
	    {
		if (feof(csinfo[i].fr_fp))
		    errno = EIO;
		(void)sprintf(buf, "error reading cscope connection %d", i);
		(void)EMSG(buf);
		goto parse_out;
	    }

	    /* If the line's too long for the buffer, discard it. */
	    if ((p = strchr(buf, '\n')) == NULL)
	    {
		while ((ch = getc(csinfo[i].fr_fp)) != EOF && ch != '\n')
		    ;
		continue;
	    }
	    *p = '\0';

	    /*
	     * cscope output is in the following format:
	     *
	     *	<filename> <context> <line number> <pattern>
	     */
	    if ((name = strtok((char *)buf, (const char *)" ")) == NULL)
		continue;
	    if ((cntx = strtok(NULL, (const char *)" ")) == NULL)
		continue;
	    if ((slno = strtok(NULL, (const char *)" ")) == NULL)
		continue;
	    search = slno + strlen(slno) + 1;	/* +1 to skip \0 */

	    /* --- nvi ---
	     * If the file is older than the cscope database, that is,
	     * the database was built since the file was last modified,
	     * or there wasn't a search string, use the line number.
	     */
	    if (strcmp(search, "<unknown>") == 0)
		search = NULL;

	    fullname = cs_resolve_file(i, name);
	    if (fullname == NULL)
		continue;

	    matches[totsofar] = cs_make_vim_style_matches(fullname, slno,
							  search, tagstr);

	    vim_free(fullname);

	    if (strcmp(cntx, "<global>") == 0)
		cntxts[totsofar] = NULL;
	    else
		/* note: if vim_strsave returns NULL, then the context
		 * will be "<global>", which is misleading.
		 */
		cntxts[totsofar] = (char *)vim_strsave((char_u *)cntx);

	    if (matches[totsofar] != NULL)
		totsofar++;

	} /* for all matches */

	(void)cs_read_prompt(i);

    } /* for all cscope connections */

parse_out:
    *matched = totsofar;
    *matches_p = matches;
    *cntxts_p = cntxts;
} /* cs_parse_results */


/*
 * PRIVATE: cs_print_tags_priv
 *
 * called from cs_manage_matches()
 */
    static void
cs_print_tags_priv(matches, cntxts, num_matches)
    char **matches;
    char **cntxts;
    int num_matches;
{
    char buf[1024]; /* hope that 1024 is enough, else we need to malloc */
    char *fname, *lno, *extra, *tbuf;
    int i, j, idx, num;
    bool_t *in_cur_file;

    assert (num_matches > 0);

    if ((tbuf = (char *)alloc(strlen(matches[0]) + 1)) == NULL)
    {
	MSG_PUTS_ATTR("couldn't malloc\n", highlight_attr[HLF_T] | MSG_HIST);
	return;
    }

    strcpy(tbuf, matches[0]);
    (void)sprintf(buf, "Cscope tag: %s\n", strtok(tbuf, "\t"));
    vim_free(tbuf);
    MSG_PUTS_ATTR(buf, highlight_attr[HLF_T]);

    MSG_PUTS_ATTR("   #   line", highlight_attr[HLF_T]);    /* strlen is 7 */
    msg_advance(msg_col + 2);
    MSG_PUTS_ATTR("filename / context / line\n", highlight_attr[HLF_T]);

    /*
     * for normal tags (non-cscope tags), vim sorts the tags before printing.
     * hence, the output from 'matches' needs to be sorted as well (but we're
     * not actually sorting the contents of 'matches').  for sorting, we simply
     * print all tags in the current file before any other tags.  this idea
     * was suggested by Jeffrey George <jgeorge@texas.net>
     */
    in_cur_file = (bool_t *)alloc_clear(num_matches * sizeof(bool_t));
    if (in_cur_file != NULL)
    {
	if (curbuf->b_fname != NULL)
	{
	    char *f;
	    int len;

	    len = strlen((const char *)(curbuf->b_fname));
	    for (i = 0; i < num_matches; i++)
	    {
		if ((f = strchr(matches[i], '\t')) != NULL)
		{
		    f++;
		    if (strncmp((const char *)(curbuf->b_fname), f, len) == 0)
			in_cur_file[i] = TRUE;
		}
	    }
	}
    }

    /*
     * if we were able to allocate 'in_cur_file', then we make two passes
     * through 'matches'.  the first pass prints the tags in the current file,
     * while the second prints all remaining tags.  if we were unable to
     * allocate 'in_cur_file', we'll just make one pass through 'matches' and
     * print them unsorted.
     */
    num = 1;
    for (j = (in_cur_file) ? 2 : 1; j > 0; j--)
    {
	for (i = 0; i < num_matches; i++)
	{
	    if (in_cur_file)
	    {
		if (((j == 2) && (in_cur_file[i] == TRUE)) ||
		    ((j == 1) && (in_cur_file[i] == FALSE)))
			idx = i;
		else
		    continue;
	    }
	    else
		idx = i;

	    /* if we really wanted to, we could avoid this malloc and strcpy
	     * by parsing matches[i] on the fly and placing stuff into buf
	     * directly, but that's too much of a hassle
	     */
	    if ((tbuf = (char *)alloc(strlen(matches[idx]) + 1)) == NULL)
		continue;
	    (void)strcpy(tbuf, matches[idx]);

	    if ((fname = strtok(tbuf, (const char *)"\t")) == NULL)
		continue;
	    if ((fname = strtok(NULL, (const char *)"\t")) == NULL)
		continue;
	    if ((lno = strtok(NULL, (const char *)"\t")) == NULL)
	    {
		/* if NULL, then no "extra", although in cscope's case, there
		 * should always be "extra".
		 */
		extra = NULL;
	    }

	    extra = lno + strlen(lno) + 1;

	    lno[strlen(lno)-2] = '\0';  /* ignore ;" at the end */

	    (void)sprintf(buf, "%4d %6s  ", num, lno);
	    MSG_PUTS_ATTR(buf, highlight_attr[HLF_CM]);
	    MSG_PUTS_LONG_ATTR(fname, highlight_attr[HLF_CM]);

	    (void)sprintf(buf, " <<%s>>",
			  (cntxts[idx] == NULL) ? "GLOBAL" : cntxts[idx]);

	    /* print the context only if it fits on the same line */
	    if (msg_col + strlen(buf) >= (int)Columns)
		msg_putchar('\n');
	    msg_advance(12);
	    MSG_PUTS_LONG(buf);
	    msg_putchar('\n');

	    if (extra != NULL)
	    {
		msg_advance(13);
		MSG_PUTS_LONG(extra);
	    }

	    vim_free(tbuf); /* only after printing extra due to strtok use */

	    if (msg_col)
		msg_putchar('\n');

	    ui_breakcheck();
	    if (got_int)
	    {
		got_int = FALSE;	/* don't print any more matches */
		break;
	    }

	    num++;
	} /* for all matches */
    } /* 1 or 2 passes through 'matches' */

    vim_free(in_cur_file);
} /* cs_print_tags_priv */


/*
 * PRIVATE: cs_read_prompt
 *
 * read a cscope prompt (basically, skip over the ">> ")
 */
    static int
cs_read_prompt(i)
    int i;
{
    int ch;

    for (;;)
    {
	while ((ch = getc(csinfo[i].fr_fp)) != EOF && ch != CSCOPE_PROMPT[0])
	    ;

	if (ch == EOF)
	{
	    perror("cs_read_prompt EOF(1)");
	    cs_release_csp(i, TRUE);
	    return CSCOPE_FAILURE;
	}

	ch = getc(csinfo[i].fr_fp);
	if (ch == EOF)
	    perror("cs_read_prompt EOF(2)");
	if (ch != CSCOPE_PROMPT[1])
	    continue;

	ch = getc(csinfo[i].fr_fp);
	if (ch == EOF)
	    perror("cs_read_prompt EOF(3)");
	if (ch != CSCOPE_PROMPT[2])
	    continue;
	break;
    }
    return CSCOPE_SUCCESS;
} /* cs_read_prompt */


/*
 * PRIVATE: cs_release_csp
 *
 * does the actual free'ing for the cs ptr with an optional flag of whether
 * or not to free the filename.  called by cs_kill and cs_reset.
 */
    static void
cs_release_csp(i, freefnpp)
    int i;
    int freefnpp;
{
    int pstat;

    if (csinfo[i].fr_fp != NULL)
	(void)fclose(csinfo[i].fr_fp);
    if (csinfo[i].to_fp != NULL)
	(void)fclose(csinfo[i].to_fp);

    kill(csinfo[i].pid, SIGTERM);
    (void)waitpid(csinfo[i].pid, &pstat, 0);

    if (freefnpp)
    {
	vim_free(csinfo[i].fname);
	vim_free(csinfo[i].ppath);
	vim_free(csinfo[i].flags);
    }

    clear_csinfo(i);
} /* cs_release_csp */


/*
 * PRIVATE: cs_reset
 *
 * calls cs_kill on all cscope connections then reinits
 */
/* ARGSUSED */
    static int
cs_reset(eap)
    EXARG *eap;
{
    char	**dblist = NULL, **pplist = NULL, **fllist = NULL;
    int	i;
    char	buf[8];

    /* malloc our db and ppath list */
    dblist = (char **) alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    pplist = (char **) alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    fllist = (char **) alloc(CSCOPE_MAX_CONNECTIONS * sizeof(char *));
    if (dblist == NULL || pplist == NULL || fllist == NULL)
    {
	vim_free(dblist);
	vim_free(pplist);
	vim_free(fllist);
	return CSCOPE_FAILURE;
    }

    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	dblist[i] = csinfo[i].fname;
	pplist[i] = csinfo[i].ppath;
	fllist[i] = csinfo[i].flags;
	if (csinfo[i].fname != NULL)
	    cs_release_csp(i, FALSE);
    }

    /* rebuild the cscope connection list */
    for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
    {
	if (dblist[i] != NULL)
	{
	    cs_add_common(dblist[i], pplist[i], fllist[i]);
	    (void)sprintf(buf, " (#%d)\n", i);
	    MSG_PUTS_ATTR(buf, highlight_attr[HLF_R]);
	}
	vim_free(dblist[i]);
	vim_free(pplist[i]);
	vim_free(fllist[i]);
    }
    vim_free(dblist);
    vim_free(pplist);
    vim_free(fllist);

    MSG_PUTS_ATTR("All cscope databases reset",
					    highlight_attr[HLF_R] | MSG_HIST);
    return CSCOPE_SUCCESS;
} /* cs_reset */


/*
 * PRIVATE: cs_resolve_file
 *
 * construct the full pathname to a file found in the cscope database.
 * (Prepends ppath, if there is one and if it's not already prepended,
 * otherwise just uses the name found.)
 *
 * we need to prepend the prefix because on some cscope's (e.g., the one that
 * ships with Solaris 2.6), the output never has the prefix prepended.
 * contrast this with my development system (Digital Unix), which does.
 */
    static char *
cs_resolve_file(i, name)
    int i;
    char *name;
{
    char *fullname;
    int len;

    /*
     * ppath is freed when we destroy the cscope connection.
     * fullname is freed after cs_make_vim_style_matches, after it's been
     * copied into the tag buffer used by vim
     */
    len = strlen(name) + 2;
    if (csinfo[i].ppath != NULL)
	len += strlen(csinfo[i].ppath);

    if ((fullname = (char *)alloc(len)) == NULL)
	return NULL;

    /*
     * note/example: this won't work if the cscope output already starts
     * "../.." and the prefix path is also "../..".  if something like this
     * happens, you are screwed up and need to fix how you're using cscope.
     */
    if (csinfo[i].ppath != NULL &&
	(strncmp(name, csinfo[i].ppath, strlen(csinfo[i].ppath)) != 0) &&
	(name[0] != '/'))
	(void)sprintf(fullname, "%s/%s", csinfo[i].ppath, name);
    else
	(void)sprintf(fullname, "%s", name);

    return fullname;
} /* cs_resolve_file */


/*
 * PRIVATE: cs_show
 *
 * show all cscope connections
 */
/* ARGSUSED */
    static int
cs_show(eap)
    EXARG *eap;
{
    char buf[MAXPATHL*2];
    short i;

    if (cs_cnt_connections() == 0)
	MSG_PUTS("no cscope connections\n");
    else
    {
	MSG_PUTS_ATTR(
	    " # pid    database name                       prepend path\n",
	    highlight_attr[HLF_T]);
	for (i = 0; i < CSCOPE_MAX_CONNECTIONS; i++)
	{
	    if (csinfo[i].fname == NULL)
		continue;

	    if (csinfo[i].ppath != NULL)
		(void)sprintf(buf, "%2d %-5ld  %-34s  %-32s\n",
		    i, (long)csinfo[i].pid, csinfo[i].fname, csinfo[i].ppath);
	    else
		(void)sprintf(buf, "%2d %-5ld  %-34s  <none>\n",
		    i, (long)csinfo[i].pid, csinfo[i].fname);
	    MSG_PUTS(buf);
	}
    }

    wait_return(TRUE);
    return CSCOPE_SUCCESS;
} /* cs_show */

#endif	/* USE_CSCOPE */

/* the end */
