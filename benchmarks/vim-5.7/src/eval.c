/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * eval.c: Expression evaluation.
 */

#include "vim.h"

#ifdef AMIGA
# include <time.h>	/* for strftime() */
#endif

#if defined(WANT_EVAL) || defined(PROTO)

#if SIZEOF_INT <= 3		/* use long if int is smaller than 32 bits */
typedef long	var_number_type;
#else
typedef int	var_number_type;
#endif

/*
 * Structure to hold an internal variable.
 */
typedef struct
{
    char_u	*var_name;	/* name of variable */
    char	var_type;	/* VAR_NUMBER or VAR_STRING */
    union
    {
	var_number_type	var_number;   /* number value */
	char_u		*var_string;  /* string value (Careful: can be NULL!) */
    } var_val;
} var;

#define VAR_UNKNOWN 0
#define VAR_NUMBER  1
#define VAR_STRING  2

typedef var *	VAR;

/*
 * All user-defined internal variables are stored in "variables".
 */
struct growarray    variables = {0, 0, sizeof(var), 4, NULL};

#define VAR_ENTRY(idx)	(((VAR)(variables.ga_data))[idx])
#define VAR_GAP_ENTRY(idx, gap)	(((VAR)(gap->ga_data))[idx])
#define BVAR_ENTRY(idx)	(((VAR)(curbuf->b_vars.ga_data))[idx])
#define WVAR_ENTRY(idx)	(((VAR)(curwin->w_vars.ga_data))[idx])

static int echo_attr = 0;   /* attributes used for ":echo" */

/*
 * Structure to hold info for a user function.
 */
struct ufunc
{
    struct ufunc	*next;		/* next function in list */
    char_u		*name;		/* name of function */
    int			varargs;	/* variable nr of arguments */
    int			flags;
    int			calls;		/* nr of active calls */
    struct growarray	args;		/* arguments */
    struct growarray	lines;		/* function lines */
};

/* function flags */
#define FC_ABORT    1		/* abort function on error */
#define FC_RANGE    2		/* function accepts range */

/*
 * All user-defined functions are found in the forward-linked function list.
 * The first function is pointed at by firstfunc.
 */
struct ufunc	    *firstfunc = NULL;

#define FUNCARG(fp, j)	((char_u **)(fp->args.ga_data))[j]
#define FUNCLINE(fp, j)	((char_u **)(fp->lines.ga_data))[j]

/* structure to hold info for a function that is currently being executed. */
struct funccall
{
    struct ufunc *func;		/* function being called */
    int		linenr;		/* next line to be executed */
    int		argcount;	/* nr of arguments */
    VAR		argvars;	/* arguments */
    var		a0_var;		/* "a:0" variable */
    var		firstline;	/* "a:firstline" variable */
    var		lastline;	/* "a:lastline" variable */
    struct growarray l_vars;	/* local function variables */
    VAR		retvar;		/* return value variable */
};

/* pointer to funccal for currently active function */
struct funccall *current_funccal = NULL;

/*
 * Array to hold the value of v: variables.
 */
#include "version.h"

/* values for flags: */
#define VV_COMPAT   1	    /* compatible, also used without "v:" */
#define VV_RO	    2	    /* read-only */

struct vimvar
{
    char	*name;		/* name of variable, without v: */
    int		len;		/* length of name */
    char_u	*val;		/* current value (can also be a number!) */
    char	type;		/* VAR_NUMBER or VAR_STRING */
    char	flags;		/* VV_COMPAT and VV_RO */
} vimvars[VV_LEN] =
{   /* The order here must match the VV_ defines in vim.h! */
    {"count", sizeof("count") - 1, NULL, VAR_NUMBER, VV_COMPAT+VV_RO},
    {"count1", sizeof("count1") - 1, NULL, VAR_NUMBER, VV_RO},
    {"errmsg", sizeof("errmsg") - 1, NULL, VAR_STRING, VV_COMPAT},
    {"warningmsg", sizeof("warningmsg") - 1, NULL, VAR_STRING, 0},
    {"statusmsg", sizeof("statusmsg") - 1, NULL, VAR_STRING, 0},
    {"shell_error", sizeof("shell_error") - 1, NULL, VAR_NUMBER,
							     VV_COMPAT+VV_RO},
    {"this_session", sizeof("this_session") - 1, NULL, VAR_STRING, VV_COMPAT},
    {"version", sizeof("version") - 1, (char_u *)(VIM_VERSION_MAJOR * 100
			   + VIM_VERSION_MINOR), VAR_NUMBER, VV_COMPAT+VV_RO},
};

static int eval0 __ARGS((char_u *arg,  VAR retvar, char_u **nextcmd, int evaluate));
static int eval1 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval2 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval3 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval4 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval5 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int eval6 __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_option_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_string_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_lit_string_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int get_env_var __ARGS((char_u **arg, VAR retvar, int evaluate));
static int find_internal_func __ARGS((char_u *name));
static int get_func_var __ARGS((char_u *name, int len, VAR retvar, char_u **arg, linenr_t firstline, linenr_t lastline, int *doesrange, int evaluate));
static linenr_t get_arg_lnum __ARGS((VAR argvars, VAR retvar));
static void f_append __ARGS((VAR argvars, VAR retvar));
static void f_argc __ARGS((VAR argvars, VAR retvar));
static void f_argv __ARGS((VAR argvars, VAR retvar));
static void f_browse __ARGS((VAR argvars, VAR retvar));
static BUF *find_buffer __ARGS((VAR avar));
static void f_bufexists __ARGS((VAR argvars, VAR retvar));
static void f_bufloaded __ARGS((VAR argvars, VAR retvar));
static BUF *get_buf_var __ARGS((VAR avar));
static void f_bufname __ARGS((VAR argvars, VAR retvar));
static void f_bufnr __ARGS((VAR argvars, VAR retvar));
static void f_bufwinnr __ARGS((VAR argvars, VAR retvar));
static void f_byte2line __ARGS((VAR argvars, VAR retvar));
static void f_char2nr __ARGS((VAR argvars, VAR retvar));
static void f_col __ARGS((VAR argvars, VAR retvar));
static void f_confirm __ARGS((VAR argvars, VAR retvar));
static void f_delete __ARGS((VAR argvars, VAR retvar));
static void f_did_filetype __ARGS((VAR argvars, VAR retvar));
static void f_escape __ARGS((VAR argvars, VAR retvar));
static void f_exists __ARGS((VAR argvars, VAR retvar));
static void f_expand __ARGS((VAR argvars, VAR retvar));
static void f_filereadable __ARGS((VAR argvars, VAR retvar));
static void f_fnamemodify __ARGS((VAR argvars, VAR retvar));
static void f_getcwd __ARGS((VAR argvars, VAR retvar));
static void f_getftime __ARGS((VAR argvars, VAR retvar));
static void f_getline __ARGS((VAR argvars, VAR retvar));
static void f_getwinposx __ARGS((VAR argvars, VAR retvar));
static void f_getwinposy __ARGS((VAR argvars, VAR retvar));
static void f_glob __ARGS((VAR argvars, VAR retvar));
static void f_has __ARGS((VAR argvars, VAR retvar));
static void f_histadd __ARGS((VAR argvars, VAR retvar));
static void f_histdel __ARGS((VAR argvars, VAR retvar));
static void f_histget __ARGS((VAR argvars, VAR retvar));
static void f_histnr __ARGS((VAR argvars, VAR retvar));
static void f_hlexists __ARGS((VAR argvars, VAR retvar));
static void f_hlID __ARGS((VAR argvars, VAR retvar));
static void f_hostname __ARGS((VAR argvars, VAR retvar));
static void f_isdirectory __ARGS((VAR argvars, VAR retvar));
static void f_input __ARGS((VAR argvars, VAR retvar));
static void f_last_buffer_nr __ARGS((VAR argvars, VAR retvar));
static void f_libcall __ARGS((VAR argvars, VAR retvar));
static void f_line __ARGS((VAR argvars, VAR retvar));
static void f_line2byte __ARGS((VAR argvars, VAR retvar));
static void f_localtime __ARGS((VAR argvars, VAR retvar));
static void f_maparg __ARGS((VAR argvars, VAR retvar));
static void f_mapcheck __ARGS((VAR argvars, VAR retvar));
static void get_maparg __ARGS((VAR argvars, VAR retvar, int exact));
static void f_match __ARGS((VAR argvars, VAR retvar));
static void f_matchend __ARGS((VAR argvars, VAR retvar));
static void f_matchstr __ARGS((VAR argvars, VAR retvar));
static void f_nr2char __ARGS((VAR argvars, VAR retvar));
static void f_rename __ARGS((VAR argvars, VAR retvar));
static void f_setline __ARGS((VAR argvars, VAR retvar));
static void f_some_match __ARGS((VAR argvars, VAR retvar, int start));
static void f_strftime __ARGS((VAR argvars, VAR retvar));
static void f_strlen __ARGS((VAR argvars, VAR retvar));
static void f_strpart __ARGS((VAR argvars, VAR retvar));
static void f_strtrans __ARGS((VAR argvars, VAR retvar));
static void f_synID __ARGS((VAR argvars, VAR retvar));
static void f_synIDattr __ARGS((VAR argvars, VAR retvar));
static void f_synIDtrans __ARGS((VAR argvars, VAR retvar));
static void f_system __ARGS((VAR argvars, VAR retvar));
static void f_substitute __ARGS((VAR argvars, VAR retvar));
static void f_tempname __ARGS((VAR argvars, VAR retvar));
static void f_virtcol __ARGS((VAR argvars, VAR retvar));
static void f_visualmode __ARGS((VAR argvars, VAR retvar));
static void f_winbufnr __ARGS((VAR argvars, VAR retvar));
static void f_winheight __ARGS((VAR argvars, VAR retvar));
static void f_winnr __ARGS((VAR argvars, VAR retvar));
static WIN *find_win_by_nr __ARGS((VAR vp));
static FPOS *var2fpos __ARGS((VAR varp));
static int get_env_len __ARGS((char_u **arg));
static int get_id_len __ARGS((char_u **arg));
static int eval_isnamec __ARGS((int c));
static int find_vim_var __ARGS((char_u *name, int len));
static int get_var_var __ARGS((char_u *name, int len, VAR retvar));
static VAR alloc_var __ARGS((void));
static VAR alloc_string_var __ARGS((char_u *string));
static void free_var __ARGS((VAR varp));
static void clear_var __ARGS((VAR varp));
static long get_var_number __ARGS((VAR varp));
static char_u *get_var_string __ARGS((VAR varp));
static char_u *get_var_string_buf __ARGS((VAR varp, char_u *buf));
static VAR find_var __ARGS((char_u *name, int writing));
static struct growarray *find_var_ga __ARGS((char_u *name, char_u **varname));
static void var_free_one __ARGS((VAR v));
static void list_one_var __ARGS((VAR v, char_u *prefix));
static void list_vim_var __ARGS((int i));
static void list_one_var_a __ARGS((char_u *prefix, char_u *name, int type, char_u *string));
static void set_var __ARGS((char_u *name, VAR varp));
static char_u *find_option_end __ARGS((char_u *p));
static void list_func_head __ARGS((struct ufunc *fp));
static struct ufunc *find_func __ARGS((char_u *name));
static void call_func __ARGS((struct ufunc *fp, int argcount, VAR argvars, VAR retvar, linenr_t firstline, linenr_t lastline));


#if defined(STATUSLINE) || defined(PROTO)
/*
 * Set an internal variable to a string value. Creates the variable if it does
 * not already exist.
 */
    void
set_internal_string_var(name, value)
    char_u	*name;
    char_u	*value;
{
    char_u  *val;
    VAR	    varp;

    val = vim_strsave(value);
    if (val != NULL)
    {
	varp = alloc_string_var(val);
	if (varp != NULL)
	{
	    set_var(name, varp);
	    free_var(varp);
	}
    }
}
#endif

/*
 * Top level evaluation function, returning a boolean.
 * Sets "error" to TRUE if there was an error.
 * Return TRUE or FALSE.
 */
    int
eval_to_bool(arg, error, nextcmd, skip)
    char_u	*arg;
    int		*error;
    char_u	**nextcmd;
    int		skip;	    /* only parse, don't execute */
{
    var		retvar;
    int		retval = FALSE;

    if (skip)
	++emsg_off;
    if (eval0(arg, &retvar, nextcmd, !skip) == FAIL)
    {
	*error = TRUE;
    }
    else if (!skip)
    {
	*error = FALSE;
	retval = (get_var_number(&retvar) != 0);
	clear_var(&retvar);
    }
    if (skip)
	--emsg_off;

    return retval;
}

/*
 * Top level evaluation function, returning a string.
 * Return pointer to allocated memory, or NULL for failure.
 */
    char_u *
eval_to_string(arg, nextcmd)
    char_u	*arg;
    char_u	**nextcmd;
{
    var		retvar;
    char_u	*retval;

    if (eval0(arg, &retvar, nextcmd, TRUE) == FAIL)
	retval = NULL;
    else
    {
	retval = vim_strsave(get_var_string(&retvar));
	clear_var(&retvar);
    }

    return retval;
}

/*
 * ":let var = expr"	assignment command.
 * ":let var"		list one variable value
 * ":let"		list all variable values
 */
    void
do_let(eap)
    EXARG	*eap;
{
    char_u	*arg = eap->arg;
    char_u	*expr;
    char_u	*name;
    VAR		varp;
    var		retvar;
    char_u	*p;
    int		c1, c2;
    int		i;

    expr = vim_strchr(arg, '=');
    if (expr == NULL)
    {
	if (ends_excmd(*arg))
	{
	    if (!eap->skip)
	    {
		/*
		 * List all variables.
		 */
		for (i = 0; i < variables.ga_len; ++i)
		    if (VAR_ENTRY(i).var_name != NULL)
			list_one_var(&VAR_ENTRY(i), (char_u *)"");
		for (i = 0; i < curbuf->b_vars.ga_len; ++i)
		    if (BVAR_ENTRY(i).var_name != NULL)
			list_one_var(&BVAR_ENTRY(i), (char_u *)"b:");
		for (i = 0; i < curwin->w_vars.ga_len; ++i)
		    if (WVAR_ENTRY(i).var_name != NULL)
			list_one_var(&WVAR_ENTRY(i), (char_u *)"w:");
		for (i = 0; i < VV_LEN; ++i)
		    if (vimvars[i].type == VAR_NUMBER || vimvars[i].val != NULL)
			list_vim_var(i);
	    }
	}
	else
	{
	    /*
	     * List variables.
	     */
	    while (!ends_excmd(*arg))
	    {
		for (p = arg; eval_isnamec(*p); ++p)
		    ;
		if (!vim_iswhite(*p) && !ends_excmd(*p))
		{
		    EMSG(e_trailing);
		    break;
		}
		if (!eap->skip)
		{
		    c1 = *p;
		    *p = NUL;
		    i = find_vim_var(arg, (int)(p - arg));
		    if (i >= 0)
			list_vim_var(i);
		    else
		    {
			varp = find_var(arg, FALSE);
			if (varp == NULL)
			    EMSG2("Unknown variable: \"%s\"", arg);
			else
			{
			    name = vim_strchr(arg, ':');
			    if (name != NULL)
			    {
				/* "a:" vars have no name stored, use whole
				 * arg */
				if (arg[0] == 'a' && arg[1] == ':')
				    c2 = NUL;
				else
				{
				    c2 = *++name;
				    *name = NUL;
				}
				list_one_var(varp, arg);
				if (c2 != NUL)
				    *name = c2;
			    }
			    else
				list_one_var(varp, (char_u *)"");
			}
		    }
		    *p = c1;
		}
		arg = skipwhite(p);
	    }
	}
	eap->nextcmd = check_nextcmd(arg);
    }
    else
    {
	if (eap->skip)
	    ++emsg_off;
	i = eval0(expr + 1, &retvar, &eap->nextcmd, !eap->skip);
	if (eap->skip)
	{
	    if (i != FAIL)
		clear_var(&retvar);
	    --emsg_off;
	}
	else if (i != FAIL)
	{
	    /*
	     * ":let $VAR = expr": Set environment variable.
	     */
	    if (*arg == '$')
	    {
		int	len;
		int	cc;

		/* Find the end of the name. */
		++arg;
		name = arg;
		len = get_env_len(&arg);
		if (len == 0)
		    EMSG2(e_invarg2, name - 1);
		else
		{
		    if (*skipwhite(arg) != '=')
			EMSG(e_letunexp);
		    else
		    {
			cc = name[len];
			name[len] = NUL;
			p = get_var_string(&retvar);
			vim_setenv(name, p);
			if (STRICMP(name, "HOME") == 0)
			    init_homedir();
			else if (didset_vim && STRICMP(name, "VIM") == 0)
			    didset_vim = FALSE;
			else if (didset_vimruntime
					  && STRICMP(name, "VIMRUNTIME") == 0)
			    didset_vimruntime = FALSE;
			name[len] = cc;
		    }
		}
	    }

	    /*
	     * ":let &option = expr": Set option value.
	     */
	    else if (*arg == '&')
	    {
		/*
		 * Find the end of the name;
		 */
		++arg;
		p = find_option_end(arg);
		if (*skipwhite(p) != '=')
		    EMSG(e_letunexp);
		else
		{
		    c1 = *p;
		    *p = NUL;
		    set_option_value(arg, get_var_number(&retvar),
						     get_var_string(&retvar));
		    *p = c1;		    /* put back for error messages */
		}
	    }

	    /*
	     * ":let @r = expr": Set register contents.
	     */
	    else if (*arg == '@')
	    {
		++arg;
		if (*skipwhite(arg + 1) != '=')
		    EMSG(e_letunexp);
		else
		    write_reg_contents(*arg == '@' ? '"' : *arg,
						     get_var_string(&retvar));
	    }

	    /*
	     * ":let var = expr": Set internal variable.
	     */
	    else if (eval_isnamec(*arg) && !isdigit(*arg))
	    {
		/*
		 * Find the end of the name;
		 */
		for (p = arg; eval_isnamec(*p); ++p)
		    ;
		if (*skipwhite(p) != '=')
		    EMSG(e_letunexp);
		else
		{
		    c1 = *p;
		    *p = NUL;
		    set_var(arg, &retvar);
		    *p = c1;		/* put char back for error messages */
		}
	    }

	    else
	    {
		EMSG2(e_invarg2, arg);
	    }

	    clear_var(&retvar);
	}
    }
}

#ifdef CMDLINE_COMPL

    void
set_context_for_expression(arg, cmdidx)
    char_u  *arg;
    CMDIDX  cmdidx;
{
    int	    got_eq = FALSE;
    int	    c;

    expand_context = cmdidx == CMD_let ? EXPAND_USER_VARS
				       : cmdidx == CMD_call ? EXPAND_FUNCTIONS
				       : EXPAND_EXPRESSION;
    while ((expand_pattern = vim_strpbrk(arg,
				  (char_u *)"\"'+-*/%.=!?~|&([<>,#")) != NULL)
    {
	c = *expand_pattern;
	if (c == '&')
	{
	    c = expand_pattern[1];
	    if (c == '&')
	    {
		++expand_pattern;
		expand_context = cmdidx!=CMD_let || got_eq ? EXPAND_EXPRESSION
							   : EXPAND_NOTHING;
	    }
	    else if (c != ' ')
		expand_context = EXPAND_SETTINGS;
	}
	else if (c == '$')
	    /* TODO: environment variables? */
	    expand_context = EXPAND_NOTHING;
	else if (*expand_pattern == '=')
	{
	    got_eq = TRUE;
	    expand_context = EXPAND_EXPRESSION;
	}
	else if (cmdidx != CMD_let || got_eq)
	{
	    if (c == '"')	    /* string */
	    {
		while ((c = *++expand_pattern) != NUL && c != '"')
		    if (c == '\\' && expand_pattern[1] != NUL)
			++expand_pattern;
		expand_context = EXPAND_NOTHING;
	    }
	    else if (c == '\'')	    /* literal string */
	    {
		while ((c = *++expand_pattern) != NUL && c != '\'')
		    /* skip */ ;
		expand_context = EXPAND_NOTHING;
	    }
	    else if (c == '|')
	    {
		if (expand_pattern[1] == '|')
		{
		    ++expand_pattern;
		    expand_context = EXPAND_EXPRESSION;
		}
		else
		    expand_context = EXPAND_COMMANDS;
	    }
	    else
		expand_context = EXPAND_EXPRESSION;
	}
	else
	    expand_context = EXPAND_NOTHING;
	arg = expand_pattern;
	if (*arg != NUL)
	    while ((c = *++arg) != NUL && (c == ' ' || c == '\t'))
		/* skip */ ;
    }
    expand_pattern = arg;
}

#endif /* CMDLINE_COMPL */

/*
 * ":1,25call func(arg1, arg2)"	function call.
 */
    void
do_call(eap)
    EXARG	*eap;
{
    char_u	*arg = eap->arg;
    char_u	*startarg;
    char_u	*name;
    var		retvar;
    int		len;
    linenr_t	lnum;
    int		doesrange;
    int		failed = FALSE;

    name = arg;
    len = get_id_len(&arg);
    startarg = arg;
    retvar.var_type = VAR_UNKNOWN;	/* clear_var() uses this */

    if (*startarg != '(')
    {
	EMSG2("Missing braces: %s", name);
	return;
    }

    /*
     * When skipping, evaluate the function once, to find the end of the
     * arguments.
     * When the function takes a range, this is discovered after the first
     * call, and the loop is broken.
     */
    if (eap->skip)
	++emsg_off;
    for (lnum = eap->line1; lnum <= eap->line2; ++lnum)
    {
	if (!eap->skip && eap->line1 != eap->line2)
	{
	    curwin->w_cursor.lnum = lnum;
	    curwin->w_cursor.col = 0;
	}
	arg = startarg;
	if (get_func_var(name, len, &retvar, &arg,
		      eap->line1, eap->line2, &doesrange, !eap->skip) == FAIL)
	{
	    failed = TRUE;
	    break;
	}
	clear_var(&retvar);
	if (doesrange || eap->skip)
	    break;
    }
    if (eap->skip)
	--emsg_off;

    if (!failed)
    {
	/* Check for trailing illegal characters and a following command. */
	if (!ends_excmd(*arg))
	    emsg(e_trailing);
	else
	    eap->nextcmd = check_nextcmd(arg);
    }
}

/*
 * ":unlet[!] var1 ... " command.
 */
    void
do_unlet(arg, forceit)
    char_u	*arg;
    int		forceit;
{
    char_u	*name_end;
    VAR		v;
    char_u	cc;

    do
    {
	name_end = skiptowhite(arg);
	cc = *name_end;
	*name_end = NUL;

	v = find_var(arg, TRUE);
	if (v != NULL)	    /* existing variable, may need to free string */
	    var_free_one(v);
	else if (!forceit)  /* non-existing variable */
	{
	    *name_end = cc;
	    EMSG2("No such variable: \"%s\"", arg);
	    break;
	}

	*name_end = cc;
	arg = skipwhite(name_end);
    } while (*arg != NUL);
}

#ifdef CMDLINE_COMPL

/*
 * Local string buffer for the next two functions to store a variable name
 * with its prefix. Allocated in cat_prefix_varname(), freed later in
 * get_user_var_name().
 */

static char_u *cat_prefix_varname __ARGS((int prefix, char_u *name));

static char_u	*varnamebuf = NULL;
static int	varnamebuflen = 0;

/*
 * Function to concatenate a prefix and a variable name.
 */
    static char_u *
cat_prefix_varname(prefix, name)
    int		prefix;
    char_u	*name;
{
    int		len;

    len = STRLEN(name) + 3;
    if (len > varnamebuflen)
    {
	vim_free(varnamebuf);
	len += 10;			/* some additional space */
	varnamebuf = alloc(len);
	if (varnamebuf == NULL)
	{
	    varnamebuflen = 0;
	    return NULL;
	}
	varnamebuflen = len;
    }
    *varnamebuf = prefix;
    varnamebuf[1] = ':';
    STRCPY(varnamebuf + 2, name);
    return varnamebuf;
}

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * (global/buffer/window/built-in) variable names.
 */
    char_u *
get_user_var_name(idx)
    int	    idx;
{
    static int	gidx;
    static int	bidx;
    static int	widx;
    static int	vidx;
    char_u	*name;

    if (idx == 0)
	gidx = bidx = widx = vidx = 0;
    if (gidx < variables.ga_len)			/* Global variables */
    {
	while ((name = VAR_ENTRY(gidx++).var_name) == NULL
		&& gidx < variables.ga_len)
	    /* skip */;
	if (name != NULL)
	    return name;
    }
    if (bidx < curbuf->b_vars.ga_len)		/* Current buffer variables */
    {
	while ((name = BVAR_ENTRY(bidx++).var_name) == NULL
		&& bidx < curbuf->b_vars.ga_len)
	    /* skip */;
	if (name != NULL)
	    return cat_prefix_varname('b', name);
    }
    if (widx < curwin->w_vars.ga_len)		/* Current window variables */
    {
	while ((name = WVAR_ENTRY(widx++).var_name) == NULL
		&& widx < curwin->w_vars.ga_len)
	    /* skip */;
	if (name != NULL)
	    return cat_prefix_varname('w', name);
    }
    if (vidx < VV_LEN)				      /* Built-in variables */
	return cat_prefix_varname('v', (char_u *)vimvars[vidx++].name);

    vim_free(varnamebuf);
    varnamebuf = NULL;
    varnamebuflen = 0;
    return NULL;
}

#endif /* CMDLINE_COMPL */

/*
 * types for expressions.
 */
enum exp_type
{
    TYPE_UNKNOWN = 0,
    TYPE_EQUAL,		/* == */
    TYPE_NEQUAL,	/* != */
    TYPE_GREATER,	/* >  */
    TYPE_GEQUAL,	/* >= */
    TYPE_SMALLER,	/* <  */
    TYPE_SEQUAL,	/* <= */
    TYPE_MATCH,		/* =~ */
    TYPE_NOMATCH	/* !~ */
};

/*
 * The "evaluate" argument: When FALSE, the argument is only parsed but not
 * executed.  The function may return OK, but the retvar will be of type
 * VAR_UNKNOWN.  The function still returns FAIL for a syntax error.
 */

/*
 * Handle zero level expression.
 * This calls eval1() and handles error message and nextcmd.
 * Return OK or FAIL.
 */
    static int
eval0(arg, retvar, nextcmd, evaluate)
    char_u	*arg;
    VAR		retvar;
    char_u	**nextcmd;
    int		evaluate;
{
    int		ret;
    char_u	*p;

    p = skipwhite(arg);
    ret = eval1(&p, retvar, evaluate);
    if (ret == FAIL || !ends_excmd(*p))
    {
	if (ret != FAIL)
	    clear_var(retvar);
	EMSG2(e_invexpr2, arg);
	ret = FAIL;
    }
    if (nextcmd != NULL)
	*nextcmd = check_nextcmd(p);

    return ret;
}

/*
 * Handle first level expression:
 *	expr2 || expr2 || expr2	    logical OR
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval1(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	result = FALSE;
    int		first = TRUE;

    /*
     * Get the first variable.
     */
    if (eval2(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat until there is no following "||".
     */
    while ((*arg)[0] == '|' && (*arg)[1] == '|')
    {
	if (evaluate && first)
	{
	    if (get_var_number(retvar) != 0)
		result = TRUE;
	    clear_var(retvar);
	    first = FALSE;
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 2);
	if (eval2(arg, &var2, evaluate && !result) == FAIL)
	    return FAIL;

	/*
	 * Compute the result.
	 */
	if (evaluate && !result)
	{
	    if (get_var_number(&var2) != 0)
		result = TRUE;
	    clear_var(&var2);
	}
	if (evaluate)
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = result;
	}
    }

    return OK;
}

/*
 * Handle second level expression:
 *	expr3 && expr3 && expr3	    logical AND
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval2(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	result = TRUE;
    int		first = TRUE;

    /*
     * Get the first variable.
     */
    if (eval3(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat until there is no following "&&".
     */
    while ((*arg)[0] == '&' && (*arg)[1] == '&')
    {
	if (evaluate && first)
	{
	    if (get_var_number(retvar) == 0)
		result = FALSE;
	    clear_var(retvar);
	    first = FALSE;
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 2);
	if (eval3(arg, &var2, evaluate && result) == FAIL)
	    return FAIL;

	/*
	 * Compute the result.
	 */
	if (evaluate && result)
	{
	    if (get_var_number(&var2) == 0)
		result = FALSE;
	    clear_var(&var2);
	}
	if (evaluate)
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = result;
	}
    }

    return OK;
}

/*
 * Handle third level expression:
 *	var1 == var2
 *	var1 =~ var2
 *	var1 != var2
 *	var1 !~ var2
 *	var1 > var2
 *	var1 >= var2
 *	var1 < var2
 *	var1 <= var2
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval3(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var			var2;
    char_u		*p;
    int			i = 0;
    enum exp_type	type = TYPE_UNKNOWN;
    int			len = 2;
    long		n1 = FALSE, n2;
    char_u		*s1, *s2;
    char_u		buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    vim_regexp		*prog;
    int			ic;
    char_u		*save_cpo;

    /*
     * Get the first variable.
     */
    if (eval4(arg, retvar, evaluate) == FAIL)
	return FAIL;

    p = *arg;
    switch (p[0])
    {
	case '=':   if (p[1] == '=')
			type = TYPE_EQUAL;
		    else if (p[1] == '~')
			type = TYPE_MATCH;
		    break;
	case '!':   if (p[1] == '=')
			type = TYPE_NEQUAL;
		    else if (p[1] == '~')
			type = TYPE_NOMATCH;
		    break;
	case '>':   if (p[1] != '=')
		    {
			type = TYPE_GREATER;
			len = 1;
		    }
		    else
			type = TYPE_GEQUAL;
		    break;
	case '<':   if (p[1] != '=')
		    {
			type = TYPE_SMALLER;
			len = 1;
		    }
		    else
			type = TYPE_SEQUAL;
		    break;
    }

    /*
     * If there is a comparitive operator, use it.
     */
    if (type != TYPE_UNKNOWN)
    {
	/* extra question mark appended: ignore case */
	if (p[len] == '?')
	{
	    ic = TRUE;
	    ++len;
	}
	/* extra '#' appended: match case */
	else if (p[len] == '#')
	{
	    ic = FALSE;
	    ++len;
	}
	/* nothing appened: use 'ignorecase' */
	else
	    ic = p_ic;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(p + len);
	if (eval4(arg, &var2, evaluate) == FAIL)
	{
	    clear_var(retvar);
	    return FAIL;
	}

	if (evaluate)
	{
	    /*
	     * If one of the two variables is a number, compare as a number.
	     * When using "=~" or "!~", always compare as string.
	     */
	    if ((retvar->var_type == VAR_NUMBER || var2.var_type == VAR_NUMBER)
		    && type != TYPE_MATCH && type != TYPE_NOMATCH)
	    {
		n1 = get_var_number(retvar);
		n2 = get_var_number(&var2);
		switch (type)
		{
		    case TYPE_EQUAL:    n1 = (n1 == n2); break;
		    case TYPE_NEQUAL:   n1 = (n1 != n2); break;
		    case TYPE_GREATER:  n1 = (n1 > n2); break;
		    case TYPE_GEQUAL:   n1 = (n1 >= n2); break;
		    case TYPE_SMALLER:  n1 = (n1 < n2); break;
		    case TYPE_SEQUAL:   n1 = (n1 <= n2); break;
		    case TYPE_UNKNOWN:
		    case TYPE_MATCH:
		    case TYPE_NOMATCH:  break;  /* avoid gcc warning */
		}
	    }
	    else
	    {
		s1 = get_var_string_buf(retvar, buf1);
		s2 = get_var_string_buf(&var2, buf2);
		if (type != TYPE_MATCH && type != TYPE_NOMATCH)
		    i = ic ? STRICMP(s1, s2) : STRCMP(s1, s2);
		switch (type)
		{
		    case TYPE_EQUAL:    n1 = (i == 0); break;
		    case TYPE_NEQUAL:   n1 = (i != 0); break;
		    case TYPE_GREATER:  n1 = (i > 0); break;
		    case TYPE_GEQUAL:   n1 = (i >= 0); break;
		    case TYPE_SMALLER:  n1 = (i < 0); break;
		    case TYPE_SEQUAL:   n1 = (i <= 0); break;
		    case TYPE_MATCH:
		    case TYPE_NOMATCH:  reg_ic = ic;
					/* avoid 'l' flag in 'cpoptions' */
					save_cpo = p_cpo;
					p_cpo = (char_u *)"";
					prog = vim_regcomp(s2, TRUE);
					if (prog != NULL)
					{
					    n1 = vim_regexec(prog, s1, TRUE);
					    vim_free(prog);
					    if (type == TYPE_NOMATCH)
						n1 = !n1;
					}
					p_cpo = save_cpo;
					break;
		    case TYPE_UNKNOWN:  break;  /* avoid gcc warning */
		}
	    }
	    clear_var(retvar);
	    clear_var(&var2);
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = n1;
	}
    }

    return OK;
}

/*
 * Handle fourth level expression:
 *	+	number addition
 *	-	number subtraction
 *	.	string concatenation
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval4(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    int		op;
    long	n1, n2;
    char_u	*s1, *s2;
    char_u	buf1[NUMBUFLEN], buf2[NUMBUFLEN];
    char_u	*p;

    /*
     * Get the first variable.
     */
    if (eval5(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no '+', '-' or '.' is following.
     */
    for (;;)
    {
	op = **arg;
	if (op != '+' && op != '-' && op != '.')
	    break;

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval5(arg, &var2, evaluate) == FAIL)
	{
	    clear_var(retvar);
	    return FAIL;
	}

	if (evaluate)
	{
	    /*
	     * Compute the result.
	     */
	    if (op == '.')
	    {
		s1 = get_var_string_buf(retvar, buf1);
		s2 = get_var_string_buf(&var2, buf2);
		op = STRLEN(s1);
		p = alloc((unsigned)(op + STRLEN(s2) + 1));
		if (p != NULL)
		{
		    STRCPY(p, s1);
		    STRCPY(p + op, s2);
		}
		clear_var(retvar);
		retvar->var_type = VAR_STRING;
		retvar->var_val.var_string = p;
	    }
	    else
	    {
		n1 = get_var_number(retvar);
		n2 = get_var_number(&var2);
		clear_var(retvar);
		if (op == '+')
		    n1 = n1 + n2;
		else
		    n1 = n1 - n2;
		retvar->var_type = VAR_NUMBER;
		retvar->var_val.var_number = n1;
	    }
	    clear_var(&var2);
	}
    }
    return OK;
}

/*
 * Handle fifth level expression:
 *	*	number multiplication
 *	/	number division
 *	%	number modulo
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval5(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    int		op;
    long	n1 = 0, n2;

    /*
     * Get the first variable.
     */
    if (eval6(arg, retvar, evaluate) == FAIL)
	return FAIL;

    /*
     * Repeat computing, until no '*', '/' or '%' is following.
     */
    for (;;)
    {
	op = **arg;
	if (op != '*' && op != '/' && op != '%')
	    break;

	if (evaluate)
	{
	    n1 = get_var_number(retvar);
	    clear_var(retvar);
	}

	/*
	 * Get the second variable.
	 */
	*arg = skipwhite(*arg + 1);
	if (eval6(arg, &var2, evaluate) == FAIL)
	    return FAIL;

	if (evaluate)
	{
	    n2 = get_var_number(&var2);
	    clear_var(&var2);

	    /*
	     * Compute the result.
	     */
	    if (op == '*')
		n1 = n1 * n2;
	    else if (op == '/')
	    {
		if (n2 == 0)	/* give an error message? */
		    n1 = 0x7fffffff;
		else
		    n1 = n1 / n2;
	    }
	    else
	    {
		if (n2 == 0)	/* give an error message? */
		    n1 = 0;
		else
		    n1 = n1 % n2;
	    }
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = n1;
	}
    }

    return OK;
}

/*
 * Handle sixth level expression:
 *  number		number constant
 *  "string"		string contstant
 *  *option-name	option value
 *  @r			register contents
 *  identifier		variable value
 *  funcion()		function call
 *  $VAR		environment variable
 *  (expression)	nested expression
 *
 *  Also handle:
 *  ! in front		logical NOT
 *  - in front		unary minus
 *  trailing []		subscript in String
 *
 * "arg" must point to the first non-white of the expression.
 * "arg" is advanced to the next non-white after the recognized expression.
 *
 * Return OK or FAIL.
 */
    static int
eval6(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    var		var2;
    long	n;
    int		len;
    char_u	*s;
    int		val;
    char_u	*start_leader, *end_leader;
    int		ret = OK;

    /*
     * Initialise variable so that clear_var() can't mistake this for a string
     * and free a string that isn't there.
     */
    retvar->var_type = VAR_UNKNOWN;

    /*
     * Skip '!' and '-' characters.  They are handled later.
     */
    start_leader = *arg;
    while (**arg == '!' || **arg == '-')
	*arg = skipwhite(*arg + 1);
    end_leader = *arg;

    switch (**arg)
    {
    /*
     * Number constant.
     */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
		vim_str2nr(*arg, NULL, &len, TRUE, TRUE, &n, NULL);
		*arg += len;
		if (evaluate)
		{
		    retvar->var_type = VAR_NUMBER;
		    retvar->var_val.var_number = n;
		}
		break;

    /*
     * String constant: "string".
     */
    case '"':	ret = get_string_var(arg, retvar, evaluate);
		break;

    /*
     * Literal string constant: 'string'.
     */
    case '\'':	ret = get_lit_string_var(arg, retvar, evaluate);
		break;

    /*
     * Option value: &name
     */
    case '&':	ret = get_option_var(arg, retvar, evaluate);
		break;

    /*
     * Environment variable: $VAR.
     */
    case '$':	ret = get_env_var(arg, retvar, evaluate);
		break;

    /*
     * Register contents: @r.
     */
    case '@':	++*arg;
		if (evaluate)
		{
		    retvar->var_type = VAR_STRING;
		    retvar->var_val.var_string = get_reg_contents(**arg);
		}
		if (**arg != NUL)
		    ++*arg;
		break;

    /*
     * nested expression: (expression).
     */
    case '(':	*arg = skipwhite(*arg + 1);
		ret = eval1(arg, retvar, evaluate);	/* recursive! */
		if (**arg == ')')
		    ++*arg;
		else if (ret == OK)
		{
		    EMSG("Missing ')'");
		    ret = FAIL;
		}
		break;

    /*
     * Must be a variable or function name then.
     */
    default:	s = *arg;
		len = get_id_len(arg);
		if (len)
		{
		    if (**arg == '(')		/* recursive! */
			ret = get_func_var(s, len, retvar, arg,
				  curwin->w_cursor.lnum, curwin->w_cursor.lnum,
				  &len, evaluate);
		    else if (evaluate)
			ret = get_var_var(s, len, retvar);
		}
		else
		    ret = FAIL;
		break;
    }
    *arg = skipwhite(*arg);

    /*
     * Handle expr[expr] subscript.
     */
    if (**arg == '[' && ret == OK)
    {
	/*
	 * Get the variable from inside the [].
	 */
	*arg = skipwhite(*arg + 1);
	if (eval1(arg, &var2, evaluate) == FAIL)	/* recursive! */
	{
	    clear_var(retvar);
	    return FAIL;
	}

	/* Check for the ']'. */
	if (**arg != ']')
	{
	    EMSG("Missing ']'");
	    clear_var(retvar);
	    return FAIL;
	}

	if (evaluate)
	{
	    n = get_var_number(&var2);
	    clear_var(&var2);

	    /*
	     * The resulting variable is a string of a single character.
	     * If the index is too big or negative, the result is empty.
	     */
	    s = get_var_string(retvar);
	    if (n >= (long)STRLEN(s) || n < 0)
		s = NULL;
	    else
		s = vim_strnsave(s + n, 1);
	    clear_var(retvar);
	    retvar->var_type = VAR_STRING;
	    retvar->var_val.var_string = s;
	}
	*arg = skipwhite(*arg + 1);	/* skip the ']' */
    }

    /*
     * Apply logical NOT and unary '-', from right to left.
     */
    if (ret == OK && evaluate && end_leader > start_leader)
    {
	val = get_var_number(retvar);
	while (end_leader > start_leader)
	{
	    --end_leader;
	    if (*end_leader == '!')
		val = !val;
	    else if (*end_leader == '-')
		val = -val;
	}
	clear_var(retvar);
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = val;
    }

    return ret;
}

/*
 * Get an option value.
 * "arg" points to the '&' before the option name.
 * "arg" is advanced to character after the option name.
 * Return OK or FAIL.
 */
    static int
get_option_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;		/* when NULL, only check if option exists */
    int		evaluate;
{
    char_u	*option_end;
    long	numval;
    char_u	*stringval;
    int		opt_type;
    int		c;
    int		ret = OK;

    /*
     * Isolate the option name and find its value.
     */
    option_end = find_option_end(*arg + 1);
    if (option_end == *arg + 1)
    {
	if (retvar != NULL)
	    EMSG2("Option name missing: %s", *arg);
	return FAIL;
    }

    if (!evaluate)
    {
	*arg = option_end;
	return OK;
    }

    c = *option_end;
    *option_end = NUL;
    opt_type = get_option_value(*arg + 1, &numval,
					  retvar == NULL ? NULL : &stringval);

    if (opt_type == -2)			/* invalid name */
    {
	if (retvar != NULL)
	    EMSG2("Unknown option: %s", *arg + 1);
	ret = FAIL;
    }
    else if (retvar != NULL)
    {
	if (opt_type == -1)		/* hidden option */
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = 0;
	}
	else if (opt_type == 1)		/* number option */
	{
	    retvar->var_type = VAR_NUMBER;
	    retvar->var_val.var_number = numval;
	}
	else				/* string option */
	{
	    retvar->var_type = VAR_STRING;
	    retvar->var_val.var_string = stringval;
	}
    }

    *option_end = c;		    /* put back for error messages */
    *arg = option_end;

    return ret;
}

/*
 * Allocate a variable for an string constant.
 * Return OK or FAIL.
 */
    static int
get_string_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*p;
    char_u	*name;
    int		i;
    int		extra = 0;

    /*
     * Find the end of the string, skipping backslashed characters.
     */
    for (p = *arg + 1; *p && *p != '"'; ++p)
	if (*p == '\\' && p[1] != NUL)
	{
	    ++p;
	    /* A "\<x>" form occupies at least 4 characters, and produces up
	     * to 6 characters: reserve space for 2 extra */
	    if (*p == '<')
		extra += 2;
	}
    if (*p != '"')
    {
	EMSG2("Missing quote: %s", *arg);
	return FAIL;
    }

    /* If only parsing, set *arg and return here */
    if (!evaluate)
    {
	*arg = p + 1;
	return OK;
    }

    /*
     * Copy the string into allocated memory, handling backslashed
     * characters.
     */
    name = alloc((unsigned)(p - *arg + extra));
    if (name == NULL)
	return FAIL;

    i = 0;
    for (p = *arg + 1; *p && *p != '"'; ++p)
    {
	if (*p == '\\')
	{
	    switch (*++p)
	    {
		case 'b': name[i++] = BS; break;
		case 'e': name[i++] = ESC; break;
		case 'f': name[i++] = FF; break;
		case 'n': name[i++] = NL; break;
		case 'r': name[i++] = CR; break;
		case 't': name[i++] = TAB; break;

			  /* hex: "\x1", "\x12" */
		case 'X':
		case 'x': if (isxdigit(p[1]))
			  {
			      ++p;
			      name[i] = hex2nr(*p);
			      if (isxdigit(p[1]))
			      {
				  ++p;
				  name[i] = (name[i] << 4) + hex2nr(*p);
			      }
			      ++i;
			  }
			  else
			      name[i++] = *p;
			  break;

			  /* octal: "\1", "\12", "\123" */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': name[i] = *p - '0';
			  if (p[1] >= '0' && p[1] <= '7')
			  {
			      ++p;
			      name[i] = (name[i] << 3) + *p - '0';
			      if (p[1] >= '0' && p[1] <= '7')
			      {
				  ++p;
				  name[i] = (name[i] << 3) + *p - '0';
			      }
			  }
			  ++i;
			  break;

			    /* Special key, e.g.: "\<C-W>" */
		case '<': extra = trans_special(&p, name + i, FALSE);
			  if (extra)
			  {
			      i += extra;
			      --p;
			      break;
			  }
			  /* FALLTHROUGH */

		default:  name[i++] = *p;
			  break;
	    }
	}
	else
	    name[i++] = *p;
    }
    name[i] = NUL;
    *arg = p + 1;

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = name;

    return OK;
}

/*
 * Allocate a variable for an backtick-string constant.
 * Return OK or FAIL.
 */
    static int
get_lit_string_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*p;
    char_u	*name;

    /*
     * Find the end of the string.
     */
    p = vim_strchr(*arg + 1, '\'');
    if (p == NULL)
    {
	EMSG2("Missing quote: %s", *arg);
	return FAIL;
    }

    if (evaluate)
    {
	/*
	 * Copy the string into allocated memory.
	 */
	name = vim_strnsave(*arg + 1, (int)(p - (*arg + 1)));
	if (name == NULL)
	    return FAIL;

	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = name;
    }

    *arg = p + 1;

    return OK;
}

/*
 * Get the value of an environment variable.
 * "arg" is pointing to the '$'.  It is advanced to after the name.
 * If the environment variable was not set, silently assume it is empty.
 * Always return OK.
 */
    static int
get_env_var(arg, retvar, evaluate)
    char_u	**arg;
    VAR		retvar;
    int		evaluate;
{
    char_u	*string = NULL;
    int		len;
    int		cc;
    char_u	*name;

    ++*arg;
    name = *arg;
    len = get_env_len(arg);
    if (evaluate)
    {
	if (len != 0)
	{
	    cc = name[len];
	    name[len] = NUL;
	    /* first try mch_getenv(), fast for normal environment vars */
	    string = mch_getenv(name);
	    if (string != NULL && *string != NUL)
		string = vim_strsave(string);
	    else
	    {
		/* next try expanding things like $VIM and ${HOME} */
		string = expand_env_save(name - 1);
		if (string != NULL && *string == '$')
		{
		    vim_free(string);
		    string = NULL;
		}
	    }
	    name[len] = cc;
	}
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = string;
    }

    return OK;
}

/*
 * Array with names and number of arguments of all internal functions
 * MUST BE KEPT SORTED IN strcmp() ORDER FOR BINARY SEARCH!
 */
static struct fst
{
    char	*f_name;	/* function name */
    char	f_min_argc;	/* minimal number of arguments */
    char	f_max_argc;	/* maximal number of arguments */
    void	(*f_func) __ARGS((VAR args, VAR rvar)); /* impl. function */
} functions[] =
{
    {"append",		2, 2, f_append},
    {"argc",		0, 0, f_argc},
    {"argv",		1, 1, f_argv},
    {"browse",		4, 4, f_browse},
    {"bufexists",	1, 1, f_bufexists},
    {"buffer_exists",	1, 1, f_bufexists},	/* obsolete */
    {"buffer_name",	1, 1, f_bufname},	/* obsolete */
    {"buffer_number",	1, 1, f_bufnr},		/* obsolete */
    {"bufloaded",	1, 1, f_bufloaded},
    {"bufname",		1, 1, f_bufname},
    {"bufnr",		1, 1, f_bufnr},
    {"bufwinnr",	1, 1, f_bufwinnr},
    {"byte2line",	1, 1, f_byte2line},
    {"char2nr",		1, 1, f_char2nr},
    {"col",		1, 1, f_col},
    {"confirm",		2, 4, f_confirm},
    {"delete",		1, 1, f_delete},
    {"did_filetype",	0, 0, f_did_filetype},
    {"escape",		2, 2, f_escape},
    {"exists",		1, 1, f_exists},
    {"expand",		1, 2, f_expand},
    {"file_readable",	1, 1, f_filereadable},	/* obsolete */
    {"filereadable",	1, 1, f_filereadable},
    {"fnamemodify",	2, 2, f_fnamemodify},
    {"getcwd",		0, 0, f_getcwd},
    {"getftime",	1, 1, f_getftime},
    {"getline",		1, 1, f_getline},
    {"getwinposx",	0, 0, f_getwinposx},
    {"getwinposy",	0, 0, f_getwinposy},
    {"glob",		1, 1, f_glob},
    {"has",		1, 1, f_has},
    {"highlightID",	1, 1, f_hlID},		/* obsolete */
    {"highlight_exists",1, 1, f_hlexists},	/* obsolete */
    {"histadd",		2, 2, f_histadd},
    {"histdel",		1, 2, f_histdel},
    {"histget",		1, 2, f_histget},
    {"histnr",		1, 1, f_histnr},
    {"hlID",		1, 1, f_hlID},
    {"hlexists",	1, 1, f_hlexists},
    {"hostname",	0, 0, f_hostname},
    {"input",		1, 1, f_input},
    {"isdirectory",	1, 1, f_isdirectory},
    {"last_buffer_nr",	0, 0, f_last_buffer_nr},/* obsolete */
    {"libcall",		3, 3, f_libcall},
    {"line",		1, 1, f_line},
    {"line2byte",	1, 1, f_line2byte},
    {"localtime",	0, 0, f_localtime},
    {"maparg",		1, 2, f_maparg},
    {"mapcheck",	1, 2, f_mapcheck},
    {"match",		2, 2, f_match},
    {"matchend",	2, 2, f_matchend},
    {"matchstr",	2, 2, f_matchstr},
    {"nr2char",		1, 1, f_nr2char},
    {"rename",		2, 2, f_rename},
    {"setline",		2, 2, f_setline},
#ifdef HAVE_STRFTIME
    {"strftime",	1, 2, f_strftime},
#endif
    {"strlen",		1, 1, f_strlen},
    {"strpart",		3, 3, f_strpart},
    {"strtrans",	1, 1, f_strtrans},
    {"substitute",	4, 4, f_substitute},
    {"synID",		3, 3, f_synID},
    {"synIDattr",	2, 3, f_synIDattr},
    {"synIDtrans",	1, 1, f_synIDtrans},
    {"system",		1, 1, f_system},
    {"tempname",	0, 0, f_tempname},
    {"virtcol",		1, 1, f_virtcol},
    {"visualmode",	0, 0, f_visualmode},
    {"winbufnr",	1, 1, f_winbufnr},
    {"winheight",	1, 1, f_winheight},
    {"winnr",		0, 0, f_winnr},
};

#ifdef CMDLINE_COMPL

/*
 * Function given to ExpandGeneric() to obtain the list of internal
 * or user defined function names.
 */
    char_u *
get_function_name(idx)
    int	    idx;
{
    static int	intidx = -1;
    char_u	*name;
    int		len;

    if (idx == 0)
	intidx = -1;
    if (intidx < 0)
    {
	name = get_user_func_name(idx);
	if (name != NULL)
	{
	    len = STRLEN(name);
	    if (len >= IOSIZE-1)
		return name;	/* prevents overflow */

	    STRCPY(IObuff, name);
	    IObuff[len] = '(';
	    IObuff[++len] = NUL;
	    return IObuff;
	}
    }
    if (++intidx < (int)(sizeof(functions) / sizeof(struct fst)))
    {
	STRCAT(STRCPY(IObuff, functions[intidx].f_name), "(");
	return IObuff;
    }

    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the list of internal or
 * user defined variable or function names.
 */
    char_u *
get_expr_name(idx)
    int	    idx;
{
    static int	intidx = -1;
    char_u	*name;

    if (idx == 0)
	intidx = -1;
    if (intidx < 0)
    {
	name = get_function_name(idx);
	if (name != NULL)
	    return name;
    }
    return get_user_var_name(++intidx);
}

#endif /* CMDLINE_COMPL */

/*
 * Find internal function in table above.
 * Return index, or -1 if not found
 */
    static int
find_internal_func(name)
    char_u	*name;		/* name of the function */
{
    int		first = 0;
    int		last = (int)(sizeof(functions) / sizeof(struct fst)) - 1;
    int		cmp;
    int		x;

    /*
     * Find the function name in the table. Binary search.
     */
    while (first <= last)
    {
	x = first + ((unsigned)(last - first) >> 1);
	cmp = STRCMP(name, functions[x].f_name);
	if (cmp < 0)
	    last = x - 1;
	else if (cmp > 0)
	    first = x + 1;
	else
	    return x;
    }
    return -1;
}

/*
 * Allocate a variable for the result of a function.
 * Return OK or FAIL.
 */
    static int
get_func_var(name, len, retvar, arg, firstline, lastline, doesrange, evaluate)
    char_u	*name;		/* name of the function */
    int		len;		/* length of "name" */
    VAR		retvar;
    char_u	**arg;		/* argument, pointing to the '(' */
    linenr_t	firstline;	/* first line of range */
    linenr_t	lastline;	/* last line of range */
    int		*doesrange;	/* return: function handled range */
    int		evaluate;
{
    char_u	*argp;
    int		ret = FAIL;
#define MAX_FUNC_ARGS	20
    var		argvars[MAX_FUNC_ARGS];	/* vars for arguments */
    int		argcount = 0;		/* number of arguments found */
    static char *errors[] =
		{"Invalid arguments for function %s",
		 "Unknown function: %s",
		 "Too many arguments for function: %s",
		 "Not enough arguments for function: %s",
		};
#define ERROR_INVARG	0
#define ERROR_UNKNOWN	1
#define ERROR_TOOMANY	2
#define ERROR_TOOFEW	3
#define ERROR_NONE	4
#define ERROR_OTHER	5
    int		error = ERROR_NONE;
    int		i;
    struct ufunc *fp;
    int		cc;

    cc = name[len];
    name[len] = NUL;
    *doesrange = FALSE;

    /*
     * Get the arguments.
     */
    argp = *arg;
    while (argcount < MAX_FUNC_ARGS)
    {
	argp = skipwhite(argp + 1);	    /* skip the '(' or ',' */
	if (*argp == ')' || *argp == ',' || *argp == NUL)
	    break;
	if (eval1(&argp, &argvars[argcount], evaluate) == FAIL)
	{
	    error = ERROR_OTHER;
	    break;
	}
	++argcount;
	if (*argp != ',')
	    break;
    }
    if (*argp != ')' && error == ERROR_NONE)
	error = ERROR_INVARG;

    /* execute the function if no errors detected and executing */
    if (evaluate && error == ERROR_NONE)
    {
	retvar->var_type = VAR_NUMBER;	/* default is number retvar */
	error = ERROR_UNKNOWN;

	if (!islower(name[0]))
	{
	    /*
	     * User defined function.
	     */
	    fp = find_func(name);
	    if (fp != NULL)
	    {
		if (fp->flags & FC_RANGE)
		    *doesrange = TRUE;
		if (argcount < fp->args.ga_len)
		    error = ERROR_TOOFEW;
		else if (!fp->varargs && argcount > fp->args.ga_len)
		    error = ERROR_TOOMANY;
		else
		{
		    /*
		     * Call the user function.
		     * Save and restore search patterns and redo buffer.
		     */
		    save_search_patterns();
		    saveRedobuff();
		    ++fp->calls;
		    call_func(fp, argcount, argvars, retvar,
							 firstline, lastline);
		    --fp->calls;
		    restoreRedobuff();
		    restore_search_patterns();
		    error = ERROR_NONE;
		}
	    }
	}
	else
	{
	    /*
	     * Find the function name in the table, call its implementation.
	     */
	    i = find_internal_func(name);
	    if (i >= 0)
	    {
		if (argcount < functions[i].f_min_argc)
		    error = ERROR_TOOFEW;
		else if (argcount > functions[i].f_max_argc)
		    error = ERROR_TOOMANY;
		else
		{
		    argvars[argcount].var_type = VAR_UNKNOWN;
		    functions[i].f_func(argvars, retvar);
		    error = ERROR_NONE;
		}
	    }
	}
    }
    if (error == ERROR_NONE)
	ret = OK;

    *arg = skipwhite(argp + 1);

    while (--argcount >= 0)
	clear_var(&argvars[argcount]);

    if (error < ERROR_NONE)
	EMSG2((char_u *)errors[error], name);

    name[len] = cc;

    return ret;
}

/*
 * Get the lnum from the first argument.  Also accepts ".", "$", etc.
 */
    static linenr_t
get_arg_lnum(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    long	lnum;

    lnum = get_var_number(&argvars[0]);
    if (lnum == 0)  /* no valid number, try using line() */
    {
	f_line(argvars, retvar);
	lnum = retvar->var_val.var_number;
	clear_var(retvar);
    }
    return lnum;
}

/*********************************************
 * Implementation of the built-in functions
 */

/*
 * "append(lnum, string)" function
 */
    static void
f_append(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    long	lnum;

    lnum = get_arg_lnum(argvars, retvar);
    retvar->var_val.var_number = 1; /* Default: Failed */

    if (lnum >= 0 && lnum <= curbuf->b_ml.ml_line_count
	    && u_save(lnum, lnum + 1) == OK)
    {
#ifdef SYNTAX_HL
	/* recompute syntax hl., starting with lnum */
	syn_changed(lnum + 1);
#endif
	mark_adjust(lnum + 1, (linenr_t)MAXLNUM, 1L, 0L);
	ml_append(lnum, get_var_string(&argvars[1]), (colnr_t)0, FALSE);
	if (curwin->w_cursor.lnum > lnum)
	{
	    ++curwin->w_cursor.lnum;
	    changed_line_abv_curs();
	}
	changed();
	update_curbuf(NOT_VALID);
	retvar->var_val.var_number = 0;
    }
}

/*
 * "argc()" function
 */
/* ARGSUSED */
    static void
f_argc(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = arg_file_count;
}

/*
 * "argv(nr)" function
 */
    static void
f_argv(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		idx;

    idx = get_var_number(&argvars[0]);
    if (idx >= 0 && idx < arg_file_count)
	retvar->var_val.var_string = vim_strsave(arg_files[idx]);
    else
	retvar->var_val.var_string = NULL;
    retvar->var_type = VAR_STRING;
}

/*
 * "browse(save, title, initdir, default)" function
 */
/* ARGSUSED */
    static void
f_browse(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef USE_BROWSE
    int		save;
    char_u	*title;
    char_u	*initdir;
    char_u	*defname;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];

    save = get_var_number(&argvars[0]);
    title = get_var_string(&argvars[1]);
    initdir = get_var_string_buf(&argvars[2], buf);
    defname = get_var_string_buf(&argvars[3], buf2);

    retvar->var_val.var_string =
		 do_browse(save, title, defname, NULL, initdir, NULL, curbuf);
#else
    retvar->var_val.var_string = NULL;
#endif
    retvar->var_type = VAR_STRING;
}

/*
 * Find a buffer by number or exact name.
 */
    static BUF *
find_buffer(avar)
    VAR		avar;
{
    BUF		*buf = NULL;
    char_u	*name;

    if (avar->var_type == VAR_NUMBER)
	buf = buflist_findnr((int)avar->var_val.var_number);
    else if (avar->var_val.var_string != NULL)
    {
	/* First make the name into a full path name */
	name = FullName_save(avar->var_val.var_string,
#ifdef UNIX
		TRUE	    /* force expansion, get rid of symbolic links */
#else
		FALSE
#endif
		);
	if (name != NULL)
	{
	    buf = buflist_findname(name);
	    vim_free(name);
	}
    }
    return buf;
}

/*
 * "bufexists(expr)" function
 */
    static void
f_bufexists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = (find_buffer(&argvars[0]) != NULL);
}

/*
 * "bufloaded(expr)" function
 */
    static void
f_bufloaded(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    BUF		*buf;

    buf = find_buffer(&argvars[0]);
    retvar->var_val.var_number = (buf != NULL && buf->b_ml.ml_mfp != NULL);
}

/*
 * Get buffer by number or pattern.
 */
    static BUF *
get_buf_var(avar)
    VAR		avar;
{
    char_u	*name = avar->var_val.var_string;
    int		save_magic;
    char_u	*save_cpo;
    BUF		*buf;

    if (avar->var_type == VAR_NUMBER)
	return buflist_findnr((int)avar->var_val.var_number);
    if (name == NULL || *name == NUL)
	return curbuf;
    if (name[0] == '$' && name[1] == NUL)
	return lastbuf;

    /* Ignore 'magic' and 'cpoptions' here to make scripts portable */
    save_magic = p_magic;
    p_magic = TRUE;
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    buf = buflist_findnr(buflist_findpat(name, name + STRLEN(name)));

    p_magic = save_magic;
    p_cpo = save_cpo;
    return buf;
}

/*
 * "bufname(expr)" function
 */
    static void
f_bufname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    BUF		*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    retvar->var_type = VAR_STRING;
    if (buf != NULL && buf->b_fname != NULL)
	retvar->var_val.var_string = vim_strsave(buf->b_fname);
    else
	retvar->var_val.var_string = NULL;
    --emsg_off;
}

/*
 * "bufnr(expr)" function
 */
    static void
f_bufnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    BUF		*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    if (buf != NULL)
	retvar->var_val.var_number = buf->b_fnum;
    else
	retvar->var_val.var_number = -1;
    --emsg_off;
}

/*
 * "bufwinnr(nr)" function
 */
    static void
f_bufwinnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    WIN		*wp;
    int		winnr = 0;
    BUF		*buf;

    ++emsg_off;
    buf = get_buf_var(&argvars[0]);
    for (wp = firstwin; wp; wp = wp->w_next)
    {
	++winnr;
	if (wp->w_buffer == buf)
	    break;
    }
    retvar->var_val.var_number = (wp ? winnr : -1);
    --emsg_off;
}

/*
 * "byte2line(byte)" function
 */
/*ARGSUSED*/
    static void
f_byte2line(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifndef BYTE_OFFSET
    retvar->var_val.var_number = -1;
#else
    long	boff = 0;

    boff = get_var_number(&argvars[0]) - 1;
    if (boff < 0)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = ml_find_line_or_offset(curbuf,
							  (linenr_t)0, &boff);
#endif
}

/*
 * "char2nr(string)" function
 */
    static void
f_char2nr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = get_var_string(&argvars[0])[0];
}

/*
 * "col(string)" function
 */
    static void
f_col(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    colnr_t	col = 0;
    FPOS	*fp;

    fp = var2fpos(&argvars[0]);
    if (fp != NULL && fp->lnum > 0)
	col = fp->col + 1;

    retvar->var_val.var_number = col;
}

/*
 * "confirm(message, buttons[, default [, type]])" function
 */
/*ARGSUSED*/
    static void
f_confirm(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#if defined(GUI_DIALOG) || defined(CON_DIALOG)
    char_u	*message;
    char_u	*buttons;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		def = 0;
    int		type = VIM_GENERIC;
    int		c;

    message = get_var_string(&argvars[0]);
    buttons = get_var_string_buf(&argvars[1], buf);
    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	def = get_var_number(&argvars[2]);
	if (argvars[3].var_type != VAR_UNKNOWN)
	{
	    /* avoid that TO_UPPER calls get_var_string_buf() twice */
	    c = *get_var_string_buf(&argvars[3], buf2);
	    switch (TO_UPPER(c))
	    {
		case 'E': type = VIM_ERROR; break;
		case 'Q': type = VIM_QUESTION; break;
		case 'I': type = VIM_INFO; break;
		case 'W': type = VIM_WARNING; break;
		case 'G': type = VIM_GENERIC; break;
	    }
	}
    }

    retvar->var_val.var_number = do_dialog(type, NULL, message, buttons, def);
#else
    retvar->var_val.var_number = 0;
#endif
}


/*
 * "libcall(libname, funcname, argument)" function
 */
/*ARGSUSED*/
    static void
f_libcall(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    /* Add other platforms here as there are supported */
#ifdef WIN32
    char_u  *retval;
#endif

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

#ifdef WIN32
    /* The first two args must be strings, otherwise meaningless */
    if (argvars[0].var_type == VAR_STRING && argvars[1].var_type == VAR_STRING)
    {
	if (argvars[2].var_type == VAR_NUMBER)
	    retval = mch_libcall(argvars[0].var_val.var_string,
				 argvars[1].var_val.var_string,
				 NULL,
				 argvars[2].var_val.var_number);
	else
	    retval = mch_libcall(argvars[0].var_val.var_string,
				 argvars[1].var_val.var_string,
				 argvars[2].var_val.var_string,
				 0);

	if (retval != NULL)
	    retvar->var_val.var_string = retval;
    }
#endif
}

/*
 * "delete()" function
 */
    static void
f_delete(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = mch_remove(get_var_string(&argvars[0]));
}

/*
 * "did_filetype()" function
 */
/*ARGSUSED*/
    static void
f_did_filetype(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifdef AUTOCMD
    retvar->var_val.var_number = did_filetype;
#else
    retvar->var_val.var_number = 0;
#endif
}

/*
 * "escape({string}, {chars})" function
 */
    static void
f_escape(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];

    retvar->var_val.var_string =
	vim_strsave_escaped(get_var_string(&argvars[0]),
		get_var_string_buf(&argvars[1], buf));
    retvar->var_type = VAR_STRING;
}

/*
 * "exists()" function
 */
    static void
f_exists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;
    char_u	*name;
    int		n = FALSE;
    int		len;

    p = get_var_string(&argvars[0]);
    if (*p == '$')			/* environment variable */
    {
	/* first try "normal" environment variables (fast) */
	if (mch_getenv(p + 1) != NULL)
	    n = TRUE;
	else
	{
	    /* try expanding things like $VIM and ${HOME} */
	    p = expand_env_save(p);
	    if (p != NULL && *p != '$')
		n = TRUE;
	    vim_free(p);
	}
    }
    else if (*p == '&')			/* option */
	n = (get_option_var(&p, NULL, TRUE) == OK);
    else if (*p == '*')			/* internal or user defined function */
    {
	++p;
	if (islower(*p))
	    n = (find_internal_func(p) >= 0);
	else if (isupper(*p))
	    n = (find_func(p) != NULL);
    }
    else				/* internal variable */
    {
	name = p;
	len = get_id_len(&p);
	if (len != 0)
	    n = (get_var_var(name, len, NULL) == OK);
    }

    retvar->var_val.var_number = n;
}

/*
 * "expand()" function
 */
    static void
f_expand(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*s;
    int		len;
    char_u	*errormsg;
    int		flags = WILD_SILENT|WILD_USE_NL;

    retvar->var_type = VAR_STRING;
    s = get_var_string(&argvars[0]);
    if (*s == '%' || *s == '#' || *s == '<')
    {
	++emsg_off;
	retvar->var_val.var_string = eval_vars(s, &len, NULL, &errormsg, s);
	--emsg_off;
    }
    else
    {
	/* When the optional second argument is non-zero, don't remove matches
	 * for 'suffixes' and 'wildignore' */
	if (argvars[1].var_type != VAR_UNKNOWN && get_var_number(&argvars[1]))
	    flags |= WILD_KEEP_ALL;
	expand_context = EXPAND_FILES;
	retvar->var_val.var_string = ExpandOne(s, NULL, flags, WILD_ALL);
    }
}

/*
 * "filereadable()" function
 */
    static void
f_filereadable(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    FILE	*fd;
    char_u	*p;
    int		n;

    p = get_var_string(&argvars[0]);
    if (*p && !mch_isdir(p) && (fd = mch_fopen((char *)p, "r")) != NULL)
    {
	n = TRUE;
	fclose(fd);
    }
    else
	n = FALSE;

    retvar->var_val.var_number = n;
}

/*
 * "fnamemodify({fname}, {mods})" function
 */
    static void
f_fnamemodify(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*fname;
    char_u	*mods;
    int		usedlen = 0;
    int		len;
    char_u	*fbuf = NULL;
    char_u	buf[NUMBUFLEN];

    fname = get_var_string(&argvars[0]);
    mods = get_var_string_buf(&argvars[1], buf);
    len = STRLEN(fname);

    (void)modify_fname(mods, &usedlen, &fname, &fbuf, &len);

    retvar->var_type = VAR_STRING;
    if (fname == NULL)
	retvar->var_val.var_string = NULL;
    else
	retvar->var_val.var_string = vim_strnsave(fname, len);
    vim_free(fbuf);
}

/*
 * "getcwd()" function
 */
/*ARGSUSED*/
    static void
f_getcwd(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	cwd[MAXPATHL];

    retvar->var_type = VAR_STRING;
    if (mch_dirname(cwd, MAXPATHL) == FAIL)
	retvar->var_val.var_string = NULL;
    else
	retvar->var_val.var_string = vim_strsave(cwd);
}

/*
 * "getftime({fname})" function
 */
    static void
f_getftime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*fname;
    struct stat	st;

    fname = get_var_string(&argvars[0]);

    if (mch_stat((char *)fname, &st) >= 0)
	retvar->var_val.var_number = (var_number_type)st.st_mtime;
    else
	retvar->var_val.var_number = -1;
}

/*
 * "getline(lnum)" function
 */
    static void
f_getline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_t	lnum;
    char_u	*p;

    lnum = get_arg_lnum(argvars, retvar);
    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
	p = ml_get(lnum);
    else
	p = (char_u *)"";

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(p);
}

/*
 * "getwinposx()" function
 */
/*ARGSUSED*/
    static void
f_getwinposx(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = -1;
#ifdef USE_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    retvar->var_val.var_number = x;
    }
#endif
}

/*
 * "getwinposy()" function
 */
/*ARGSUSED*/
    static void
f_getwinposy(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = -1;
#ifdef USE_GUI
    if (gui.in_use)
    {
	int	    x, y;

	if (gui_mch_get_winpos(&x, &y) == OK)
	    retvar->var_val.var_number = y;
    }
#endif
}

/*
 * "glob()" function
 */
    static void
f_glob(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    expand_context = EXPAND_FILES;
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = ExpandOne(get_var_string(&argvars[0]),
				     NULL, WILD_USE_NL|WILD_SILENT, WILD_ALL);
}

/*
 * "has()" function
 */
    static void
f_has(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		i;
    char_u	*name;
    int		n = FALSE;
    static char	*(has_list[]) =
    {
#ifdef AMIGA
	"amiga",
# ifndef NO_ARP
	"arp",
# endif
#endif
#ifdef __BEOS__
	"beos",
#endif
#ifdef MSDOS
# ifdef DJGPP
	"dos32",
# else
	"dos16",
# endif
#endif
#ifdef macintosh
	"mac",
#endif
#ifdef OS2
	"os2",
#endif
#ifdef RISCOS
	"riscos",
#endif
#ifdef UNIX
	"unix",
#endif
#ifdef VMS
	"vms",
#endif
#ifdef WIN16
	"win16",
#endif
#ifdef WIN32
	"win32",
#endif
#ifndef CASE_INSENSITIVE_FILENAME
	"fname_case",
#endif
#ifdef AUTOCMD
	"autocmd",
#endif
#if defined(SOME_BUILTIN_TCAPS) || defined(ALL_BUILTIN_TCAPS)
	"builtin_terms",
# ifdef ALL_BUILTIN_TCAPS
	"all_builtin_terms",
# endif
#endif
#ifdef BYTE_OFFSET
	"byte_offset",
#endif
#ifdef CINDENT
	"cindent",
#endif
#ifdef USE_CLIPBOARD
	"clipboard",
#endif
#ifdef CMDLINE_COMPL
	"cmdline_compl",
#endif
#ifdef COMMENTS
	"comments",
#endif
#ifdef CRYPTV
	"cryptv",
#endif
#ifdef USE_CSCOPE
	"cscope",
#endif
#ifdef DEBUG
	"debug",
#endif
#ifdef CON_DIALOG
	"dialog_con",
#endif
#ifdef GUI_DIALOG
	"dialog_gui",
#endif
#ifdef DIGRAPHS
	"digraphs",
#endif
#ifdef EMACS_TAGS
	"emacs_tags",
#endif
	"eval",	    /* always present, of course! */
#ifdef EX_EXTRA
	"ex_extra",
#endif
#ifdef EXTRA_SEARCH
	"extra_search",
#endif
#ifdef FKMAP
	"farsi",
#endif
#ifdef FILE_IN_PATH
	"file_in_path",
#endif
#ifdef WANT_OSFILETYPE
	"osfiletype",
#endif
#ifdef FIND_IN_PATH
	"find_in_path",
#endif
#if !defined(USE_SYSTEM) && defined(UNIX)
	"fork",
#endif
#ifdef USE_GUI
	"gui",
#endif
#ifdef USE_GUI_ATHENA
	"gui_athena",
#endif
#ifdef USE_GUI_BEOS
	"gui_beos",
#endif
#ifdef USE_GUI_GTK
	"gui_gtk",
#endif
#ifdef USE_GUI_MAC
	"gui_mac",
#endif
#ifdef USE_GUI_MOTIF
	"gui_motif",
#endif
#ifdef USE_GUI_WIN16
	"gui_win16",
#endif
#ifdef USE_GUI_WIN32
	"gui_win32",
#endif
#ifdef HANGUL_INPUT
	"hangul_input",
#endif
#ifdef INSERT_EXPAND
	"insert_expand",
#endif
#ifdef HAVE_LANGMAP
	"langmap",
#endif
#ifdef LINEBREAK
	"linebreak",
#endif
#ifdef LISPINDENT
	"lispindent",
#endif
#ifdef WANT_MENU
	"menu",
#endif
#ifdef MKSESSION
	"mksession",
#endif
#ifdef WANT_MODIFY_FNAME
	"modify_fname",
#endif
#ifdef USE_MOUSE
	"mouse",
#endif
#if defined(UNIX) || defined(VMS)
# ifdef DEC_MOUSE
	"mouse_dec",
# endif
# ifdef GPM_MOUSE
	"mouse_gpm",
# endif
# ifdef NETTERM_MOUSE
	"mouse_netterm",
# endif
# ifdef XTERM_MOUSE
	"mouse_xterm",
# endif
#endif
#ifdef MULTI_BYTE
	"multi_byte",
#endif
#ifdef MULTI_BYTE_IME
	"multi_byte_ime",
#endif
#ifdef HAVE_OLE
	"ole",
#endif
#ifdef HAVE_PERL_INTERP
	"perl",
#endif
#ifdef HAVE_PYTHON
	"python",
#endif
#ifdef QUICKFIX
	"quickfix",
#endif
#ifdef RIGHTLEFT
	"rightleft",
#endif
#ifdef SCROLLBIND
	"scrollbind",
#endif
#ifdef CMDLINE_INFO
	"showcmd",
	"cmdline_info",
#endif
#ifdef SMARTINDENT
	"smartindent",
#endif
#ifdef USE_SNIFF
	"sniff",
#endif
#ifdef STATUSLINE
	"statusline",
#endif
#ifdef SYNTAX_HL
	"syntax",
#endif
#if defined(USE_SYSTEM) || !defined(UNIX)
	"system",
#endif
#ifdef BINARY_TAGS
	"tag_binary",
#endif
#ifdef OLD_STATIC_TAGS
	"tag_old_static",
#endif
#ifdef TAG_ANY_WHITE
	"tag_any_white",
#endif
#ifdef HAVE_TCL
	"tcl",
#endif
#ifdef TERMINFO
	"terminfo",
#endif
#ifdef TEXT_OBJECTS
	"textobjects",
#endif
#ifdef HAVE_TGETENT
	"tgetent",
#endif
#ifdef WANT_TITLE
	"title",
#endif
#ifdef USER_COMMANDS
	"user-commands",    /* was accidentally included in 5.4 */
	"user_commands",
#endif
#ifdef VIMINFO
	"viminfo",
#endif
#ifdef VISUALEXTRA
	"visualextra",
#endif
#ifdef WILDIGNORE
	"wildignore",
#endif
#ifdef WILDMENU
	"wildmenu",
#endif
#ifdef HAS_WAK
	"winaltkeys",
#endif
#ifdef WRITEBACKUP
	"writebackup",
#endif
#ifdef USE_XIM
	"xim",
#endif
#ifdef USE_FONTSET
	"xfontset",
#endif
#ifdef XTERM_CLIP
	"xterm_clipboard",
#endif
#ifdef SAVE_XTERM_SCREEN
	"xterm_save",
#endif
#if defined(UNIX) && defined(WANT_X11) && defined(HAVE_X11)
	"X11",
#endif
	NULL
    };

    name = get_var_string(&argvars[0]);
    for (i = 0; has_list[i] != NULL; ++i)
	if (STRICMP(name, has_list[i]) == 0)
	{
	    n = TRUE;
	    break;
	}

    if (n == FALSE)
    {
	if (0)
	    ;
#ifdef USE_GUI
	else if (STRICMP(name, "gui_running") == 0)
	    n = (gui.in_use || gui.starting);
# ifdef USE_GUI_WIN32
	else if (STRICMP(name, "gui_win32s") == 0)
	    n = gui_is_win32s();
# endif
# ifdef USE_BROWSE
	else if (STRICMP(name, "browse") == 0)
	    n = gui.in_use;	/* gui_mch_browse() works when GUI is running */
# endif
#endif
#ifdef SYNTAX_HL
	else if (STRICMP(name, "syntax_items") == 0)
	    n = syntax_present(curbuf);
#endif
	else if (STRICMP(name, "vim_starting") == 0)
	    n = (starting != 0);
    }

    retvar->var_val.var_number = n;
}

/*
 * "histadd()" function
 */
    static void
f_histadd(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		histype;
    char_u	*str;

    histype = get_histtype(get_var_string(&argvars[0]));
    if (histype >= 0)
    {
	str = get_var_string(&argvars[1]);
	if (*str != NUL)
	{
	    add_to_history(histype, str, FALSE);
	    retvar->var_val.var_number = TRUE;
	    return;
	}
    }
    retvar->var_val.var_number = FALSE;
}

/*
 * "histdel()" function
 */
    static void
f_histdel(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		n;

    if (argvars[1].var_type == VAR_UNKNOWN)
	/* only one argument: clear entire history */
	n = clr_history(get_histtype(get_var_string(&argvars[0])));
    else if (argvars[1].var_type == VAR_NUMBER)
	/* index given: remove that entry */
	n = del_history_idx(get_histtype(get_var_string(&argvars[0])),
					    (int)get_var_number(&argvars[1]));
    else
	/* string given: remove all matching entries */
	n = del_history_entry(get_histtype(get_var_string(&argvars[0])),
					   get_var_string(&argvars[1]));
    retvar->var_val.var_number = n;
}

/*
 * "histget()" function
 */
    static void
f_histget(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		type;
    int		idx;

    type = get_histtype(get_var_string(&argvars[0]));
    retvar->var_type = VAR_STRING;
    if (argvars[1].var_type == VAR_UNKNOWN)
	idx = get_history_idx(type);
    else
	idx = (int)get_var_number(&argvars[1]);
    retvar->var_val.var_string = vim_strsave(get_history_entry(type, idx));
}

/*
 * "histnr()" function
 */
    static void
f_histnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		i;

    i = get_histtype(get_var_string(&argvars[0]));
    if (i >= HIST_CMD && i <= HIST_INPUT)
	i = get_history_idx(i);
    else
	i = -1;
    retvar->var_val.var_number = i;
}

/*
 * "highlight_exists()" function
 */
    static void
f_hlexists(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = highlight_exists(get_var_string(&argvars[0]));
}

/*
 * "highlightID(name)" function
 */
    static void
f_hlID(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = syn_name2id(get_var_string(&argvars[0]));
}

/*
 * "hostname()" function
 */
/*ARGSUSED*/
    static void
f_hostname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u hostname[256];

    mch_get_host_name(hostname, 256);
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(hostname);
}

/*
 * "input()" function
 */
    static void
f_input(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*prompt = get_var_string(&argvars[0]);
    char_u	*p = NULL;
    int		c;

    retvar->var_type = VAR_STRING;

#ifdef NO_CONSOLE
    /*
     * While starting up, there is no place to enter text.
     */
    if (!gui.in_use || gui.starting)
    {
	retvar->var_val.var_string = NULL;
	return;
    }
#endif

    if (prompt != NULL)
    {
	/* Only the part of the message after the last NL is considered as
	 * prompt for the command line */
	p = vim_strrchr(prompt, '\n');
	if (p == NULL)
	    p = prompt;
	else
	{
	    ++p;
	    c = *p;
	    *p = NUL;
	    msg_start();
	    msg_clr_eos();
	    msg_puts_attr(prompt, echo_attr);
	    msg_didout = FALSE;
	    msg_starthere();
	    *p = c;
	}
	cmdline_row = msg_row;
    }

    retvar->var_val.var_string = getcmdline_prompt('@', p, echo_attr);

    /* since the user typed this, no need to wait for return */
    need_wait_return = FALSE;
    msg_didout = FALSE;
}

/*
 * "isdirectory()" function
 */
    static void
f_isdirectory(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = mch_isdir(get_var_string(&argvars[0]));
}

/*
 * "last_buffer_nr()" function.
 */
/*ARGSUSED*/
    static void
f_last_buffer_nr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		n = 0;
    BUF		*buf;

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (n < buf->b_fnum)
	    n = buf->b_fnum;

    retvar->var_val.var_number = n;
}

/*
 * "line(string)" function
 */
    static void
f_line(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_t	lnum = 0;
    FPOS	*fp;

    fp = var2fpos(&argvars[0]);
    if (fp != NULL)
	lnum = fp->lnum;
    else if (get_var_string(&argvars[0])[0] == '$')   /* last line in buffer */
	lnum = curbuf->b_ml.ml_line_count;

    retvar->var_val.var_number = lnum;
}

/*
 * "line2byte(lnum)" function
 */
/*ARGSUSED*/
    static void
f_line2byte(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
#ifndef BYTE_OFFSET
    retvar->var_val.var_number = -1;
#else
    linenr_t	lnum;

    lnum = get_arg_lnum(argvars, retvar);
    if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count + 1)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = ml_find_line_or_offset(curbuf, lnum, NULL);
    if (retvar->var_val.var_number >= 0)
	++retvar->var_val.var_number;
#endif
}

/*
 * "localtime()" function
 */
/*ARGSUSED*/
    static void
f_localtime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = (var_number_type)time(NULL);
}

/*
 * "maparg()" function
 */
    static void
f_maparg(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    get_maparg(argvars, retvar, TRUE);
}

/*
 * "mapcheck()" function
 */
    static void
f_mapcheck(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    get_maparg(argvars, retvar, FALSE);
}

    static void
get_maparg(argvars, retvar, exact)
    VAR		argvars;
    VAR		retvar;
    int		exact;
{
    char_u		*keys;
    char_u		*which;
    char_u		buf[NUMBUFLEN];
    char_u		*keys_buf = NULL;
    char_u		*rhs;
    int			mode;
    struct growarray	ga;

    /* return empty string for failure */
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = NULL;

    keys = get_var_string(&argvars[0]);
    if (*keys == NUL)
	return;

    if (argvars[1].var_type != VAR_UNKNOWN)
	which = get_var_string_buf(&argvars[1], buf);
    else
	which = (char_u *)"";
    mode = get_map_mode(&which, 0);

    keys = replace_termcodes(keys, &keys_buf, TRUE, TRUE);
    rhs = check_map(keys, mode, exact);
    vim_free(keys_buf);
    if (rhs != NULL)
    {
	ga_init(&ga);
	ga.ga_itemsize = 1;
	ga.ga_growsize = 40;

	while (*rhs != NUL)
	    ga_concat(&ga, str2special(&rhs, FALSE));

	ga_append(&ga, NUL);
	retvar->var_val.var_string = (char_u *)ga.ga_data;
    }
}

/*
 * "match()" function
 */
    static void
f_match(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    f_some_match(argvars, retvar, 1);
}

/*
 * "matchend()" function
 */
    static void
f_matchend(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    f_some_match(argvars, retvar, 0);
}

/*
 * "matchstr()" function
 */
    static void
f_matchstr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    f_some_match(argvars, retvar, 2);
}

    static void
f_some_match(argvars, retvar, type)
    VAR		argvars;
    VAR		retvar;
    int		type;
{
    char_u		*str;
    char_u		*pat;
    vim_regexp		*prog;
    char_u		patbuf[NUMBUFLEN];
    char_u		*save_cpo;

    /* Make 'cpoptions' empty, so that the 'l' flag doesn't work here */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    str = get_var_string(&argvars[0]);
    pat = get_var_string_buf(&argvars[1], patbuf);

    if (type == 2)
    {
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = NULL;
    }
    else
	retvar->var_val.var_number = -1;
    prog = vim_regcomp(pat, TRUE);
    if (prog != NULL)
    {
	reg_ic = p_ic;
	if (vim_regexec(prog, str, TRUE))
	{
	    if (type == 2)
		retvar->var_val.var_string = vim_strnsave(prog->startp[0],
				      (int)(prog->endp[0] - prog->startp[0]));
	    else if (type)
		retvar->var_val.var_number = prog->startp[0] - str;
	    else
		retvar->var_val.var_number = prog->endp[0] - str;
	}
	vim_free(prog);
    }

    p_cpo = save_cpo;
}

/*
 * "nr2char()" function
 */
    static void
f_nr2char(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[2];

    buf[0] = (char_u)get_var_number(&argvars[0]);
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strnsave(buf, 1);
}

/*
 * "rename({from}, {to})" function
 */
    static void
f_rename(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	buf[NUMBUFLEN];

    retvar->var_val.var_number = vim_rename(get_var_string(&argvars[0]),
					get_var_string_buf(&argvars[1], buf));
}

/*
 * "setline()" function
 */
    static void
f_setline(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    linenr_t	lnum;
    char_u	*line;

    lnum = get_arg_lnum(argvars, retvar);
    line = get_var_string(&argvars[1]);
    retvar->var_val.var_number = 1;		/* FAIL is default */

    if (lnum >= 1 && lnum <= curbuf->b_ml.ml_line_count)
    {
	if (u_savesub(lnum) == OK && ml_replace(lnum, line, TRUE) == OK)
	{
	    changed();
	    changed_cline_bef_curs();
	    approximate_botline();	/* w_botline might have changed */
	    check_cursor_col();
#ifdef SYNTAX_HL
	    /* recompute syntax hl. for this line */
	    syn_changed(lnum);
#endif
	    redraw_curbuf_later(NOT_VALID);
	    retvar->var_val.var_number = 0;
	}
    }
}

#ifdef HAVE_STRFTIME
/*
 * "strftime({format}[, {time}])" function
 */
    static void
f_strftime(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	result_buf[80];
    struct tm	*curtime;
    time_t	seconds;
    char_u	*p;

    p = get_var_string(&argvars[0]);
    if (argvars[1].var_type == VAR_UNKNOWN)
	seconds = time(NULL);
    else
	seconds = (time_t)get_var_number(&argvars[1]);
    curtime = localtime(&seconds);
    (void)strftime((char *)result_buf, (size_t)80, (char *)p, curtime);

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strsave(result_buf);
}
#endif

/*
 * "strlen()" function
 */
    static void
f_strlen(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_val.var_number = STRLEN(get_var_string(&argvars[0]));
}

/*
 * "strpart()" function
 */
    static void
f_strpart(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;
    int		n;
    int		len;
    int		slen;

    p = get_var_string(&argvars[0]);
    n = get_var_number(&argvars[1]);
    len = get_var_number(&argvars[2]);
    slen = STRLEN(p);
    /*
     * Only return the overlap between the specified part and the actual
     * string.
     */
    if (n < 0)
    {
	len += n;
	n = 0;
    }
    else if (n > slen)
	n = slen;
    if (len < 0)
	len = 0;
    else if (n + len > slen)
	len = slen - n;

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_strnsave(p + n, len);
}

/*
 * "strtrans()" function
 */
    static void
f_strtrans(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = transstr(get_var_string(&argvars[0]));
}

/*
 * "synID(line, col, trans)" function
 */
/*ARGSUSED*/
    static void
f_synID(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		id = 0;
#ifdef SYNTAX_HL
    long	line;
    long	col;
    int		trans;

    line = get_arg_lnum(argvars, retvar);
    col = get_var_number(&argvars[1]) - 1;
    trans = get_var_number(&argvars[2]);

    if (line >= 1 && line <= curbuf->b_ml.ml_line_count
	    && col >= 0 && col < (long)STRLEN(ml_get(line)))
	id = syn_get_id(line, col, trans);
#endif

    retvar->var_val.var_number = id;
}

/*
 * "synIDattr(id, what [, mode])" function
 */
/*ARGSUSED*/
    static void
f_synIDattr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p = NULL;
#ifdef SYNTAX_HL
    int		id;
    char_u	*what;
    char_u	*mode;
    char_u	modebuf[NUMBUFLEN];
    int		modec;

    id = get_var_number(&argvars[0]);
    what = get_var_string(&argvars[1]);
    if (argvars[2].var_type != VAR_UNKNOWN)
    {
	mode = get_var_string_buf(&argvars[2], modebuf);
	modec = TO_LOWER(mode[0]);
	if (modec != 't' && modec != 'c'
#ifdef USE_GUI
		&& modec != 'g'
#endif
		)
	    modec = 0;	/* replace invalid with current */
    }
    else
    {
#ifdef USE_GUI
	if (gui.in_use)
	    modec = 'g';
	else
#endif
	    if (*T_CCO)
	    modec = 'c';
	else
	    modec = 't';
    }


    switch (TO_LOWER(what[0]))
    {
	case 'b':
		if (TO_LOWER(what[1]) == 'g')		/* bg[#] */
		    p = highlight_color(id, what, modec);
		else					/* bold */
		    p = highlight_has_attr(id, HL_BOLD, modec);
		break;

	case 'f':					/* fg[#] */
		p = highlight_color(id, what, modec);
		break;

	case 'i':
		if (TO_LOWER(what[1]) == 'n')		/* inverse */
		    p = highlight_has_attr(id, HL_INVERSE, modec);
		else					/* italic */
		    p = highlight_has_attr(id, HL_ITALIC, modec);
		break;

	case 'n':					/* name */
		p = get_highlight_name(id - 1);
		break;

	case 'r':					/* reverse */
		p = highlight_has_attr(id, HL_INVERSE, modec);
		break;

	case 's':					/* standout */
		p = highlight_has_attr(id, HL_STANDOUT, modec);
		break;

	case 'u':					/* underline */
		p = highlight_has_attr(id, HL_UNDERLINE, modec);
		break;
    }

    if (p != NULL)
	p = vim_strsave(p);
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;
}

/*
 * "synIDtrans(id)" function
 */
/*ARGSUSED*/
    static void
f_synIDtrans(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		id;

#ifdef SYNTAX_HL
    id = get_var_number(&argvars[0]);

    if (id > 0)
	id = syn_get_final_id(id);
    else
#endif
	id = 0;

    retvar->var_val.var_number = id;
}

/*
 * "system()" function
 */
    static void
f_system(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	*p;

    p = get_cmd_output(get_var_string(&argvars[0]), SHELL_SILENT);
#ifdef USE_CR
    /* translate <CR> into <NL> */
    if (p != NULL)
    {
	char_u	*s;

	for (s = p; *s; ++s)
	{
	    if (*s == CR)
		*s = NL;
	}
    }
#else
# ifdef USE_CRNL
    /* translate <CR><NL> into <NL> */
    if (p != NULL)
    {
	char_u	*s, *d;

	d = p;
	for (s = p; *s; ++s)
	{
	    if (s[0] == CR && s[1] == NL)
		++s;
	    *d++ = *s;
	}
	*d = NUL;
    }
# endif
#endif
    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = p;
}

/*
 * "substitute()" function
 */
    static void
f_substitute(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	patbuf[NUMBUFLEN];
    char_u	subbuf[NUMBUFLEN];
    char_u	flagsbuf[NUMBUFLEN];

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = do_string_sub(
	    get_var_string(&argvars[0]),
	    get_var_string_buf(&argvars[1], patbuf),
	    get_var_string_buf(&argvars[2], subbuf),
	    get_var_string_buf(&argvars[3], flagsbuf));
}

/*
 * "tempname()" function
 */
/*ARGSUSED*/
    static void
f_tempname(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    static int	x = 'A';

    retvar->var_type = VAR_STRING;
    retvar->var_val.var_string = vim_tempname(x);
    /* advance 'x', so that there are at least 26 different names */
    if (x == 'Z')
	x = 'A';
    else
	++x;
}

/*
 * "virtcol(string)" function
 */
    static void
f_virtcol(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    colnr_t	vcol = 0;
    FPOS	*fp;

    fp = var2fpos(&argvars[0]);
    if (fp != NULL)
    {
	getvcol(curwin, fp, NULL, NULL, &vcol);
	++vcol;
    }

    retvar->var_val.var_number = vcol;
}

/*
 * "visualmode()" function
 */
/*ARGSUSED*/
    static void
f_visualmode(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    char_u	str[2];

    retvar->var_type = VAR_STRING;
    str[0] = curbuf->b_visual_mode;
    str[1] = NUL;
    retvar->var_val.var_string = vim_strsave(str);
}

/*
 * "winbufnr(nr)" function
 */
    static void
f_winbufnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    WIN		*wp;

    wp = find_win_by_nr(&argvars[0]);
    if (wp == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = wp->w_buffer->b_fnum;
}

/*
 * "winheight(nr)" function
 */
    static void
f_winheight(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    WIN		*wp;

    wp = find_win_by_nr(&argvars[0]);
    if (wp == NULL)
	retvar->var_val.var_number = -1;
    else
	retvar->var_val.var_number = wp->w_height;
}

/*
 * "winnr()" function
 */
/* ARGSUSED */
    static void
f_winnr(argvars, retvar)
    VAR		argvars;
    VAR		retvar;
{
    int		nr;
    WIN		*wp;

    nr = 1;
    for (wp = firstwin; wp != curwin; wp = wp->w_next)
	++nr;
    retvar->var_val.var_number = nr;
}

    static WIN *
find_win_by_nr(vp)
    VAR		vp;
{
    WIN		*wp;
    int		nr;

    nr = get_var_number(vp);

    if (nr == 0)
	return curwin;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
    {
	if (--nr <= 0)
	    break;
    }
    return wp;
}

/*
 * Translate a String variable into a position (for col() and virtcol()).
 */
    static FPOS *
var2fpos(varp)
    VAR		varp;
{
    char_u	*name;

    name = get_var_string(varp);
    if (name[0] == '.')		/* cursor */
	return &curwin->w_cursor;
    if (name[0] == '\'')	/* mark */
	return getmark(name[1], FALSE);
    return NULL;
}

/*
 * Get the length of an environment variable name.
 * Advance "arg" to the first character after the name.
 * Return 0 for error.
 */
    static int
get_env_len(arg)
    char_u	**arg;
{
    char_u	*p;
    int		len;

    for (p = *arg; vim_isIDc(*p); ++p)
	;
    if (p == *arg)	    /* no name found */
	return 0;

    len = p - *arg;
    *arg = p;
    return len;
}

/*
 * Get the length of the name of a function or internal variable.
 * "arg" is advanced to the first non-white character after the name.
 * Return 0 if something is wrong.
 */
    static int
get_id_len(arg)
    char_u	**arg;
{
    char_u	*p;
    int		len;

    /* Find the end of the name. */
    for (p = *arg; eval_isnamec(*p); ++p)
	;
    if (p == *arg)	    /* no name found */
	return 0;

    len = p - *arg;
    *arg = skipwhite(p);

    return len;
}

    static int
eval_isnamec(c)
    int	    c;
{
    return (isalpha(c) || isdigit(c) || c == '_' || c == ':');
}

/*
 * Find a v: variable.
 * Return it's index, or -1 if not found.
 */
    static int
find_vim_var(name, len)
    char_u	*name;
    int		len;		/* length of "name" */
{
    char_u	*vname;
    int		vlen;
    int		i;

    /*
     * Ignore "v:" for old built-in variables, require it for new ones.
     */
    if (name[0] == 'v' && name[1] == ':')
    {
	vname = name + 2;
	vlen = len - 2;
    }
    else
    {
	vname = name;
	vlen = len;
    }
    for (i = 0; i < VV_LEN; ++i)
	if (vlen == vimvars[i].len && STRCMP(vname, vimvars[i].name) == 0
			 && ((vimvars[i].flags & VV_COMPAT) || vname != name))
	    return i;
    return -1;
}

/*
 * Set number v: variable to "val".
 */
    void
set_vim_var_nr(idx, val)
    int		idx;
    long	val;
{
    vimvars[idx].val = (char_u *)val;
}

/*
 * Set string v: variable to a copy of "val".
 */
    void
set_vim_var_string(idx, val)
    int		idx;
    char_u	*val;
{
    vim_free(vimvars[idx].val);
    if (val == NULL)
	vimvars[idx].val = NULL;
    else
	vimvars[idx].val = vim_strsave(val);
}

/*
 * Get the value of internal variable "name".
 * Return OK or FAIL.
 */
    static int
get_var_var(name, len, retvar)
    char_u	*name;
    int		len;		/* length of "name" */
    VAR		retvar;		/* NULL when only checking existence */
{
    int		ret = OK;
    int		type = VAR_UNKNOWN;
    long	number = 1;
    char_u	*string = NULL;
    VAR		v;
    int		cc;
    int		i;

    /* truncate the name, so that we can use strcmp() */
    cc = name[len];
    name[len] = NUL;

    /*
     * Check for built-in v: variables.
     */
    i = find_vim_var(name, len);
    if (i >= 0)
    {
	type = vimvars[i].type;
	number = (long)vimvars[i].val;
	string = vimvars[i].val;
    }

    /*
     * Check for user-defined variables.
     */
    else
    {
	v = find_var(name, FALSE);
	if (v != NULL)
	{
	    type = v->var_type;
	    number = v->var_val.var_number;
	    string = v->var_val.var_string;
	}
    }

    if (type == VAR_UNKNOWN)
    {
	if (retvar != NULL)
	    EMSG2("Undefined variable: %s", name);
	ret = FAIL;
    }
    else if (retvar != NULL)
    {
	retvar->var_type = type;
	if (type == VAR_NUMBER)
	    retvar->var_val.var_number = number;
	else if (type == VAR_STRING)
	{
	    if (string != NULL)
		string = vim_strsave(string);
	    retvar->var_val.var_string = string;
	}
    }

    name[len] = cc;

    return ret;
}

/*
 * Allocate memory for a variable, and make it emtpy (0 or NULL value).
 */
    static VAR
alloc_var()
{
    return (VAR)alloc_clear((unsigned)sizeof(var));
}

/*
 * Allocate memory for a variable, and assign a string to it.
 * The string "s" must have been allocated, it is consumed.
 * Return NULL for out of memory, the variable otherwise.
 */
    static VAR
alloc_string_var(s)
    char_u	*s;
{
    VAR	    retvar;

    retvar = alloc_var();
    if (retvar != NULL)
    {
	retvar->var_type = VAR_STRING;
	retvar->var_val.var_string = s;
    }
    else
	vim_free(s);
    return retvar;
}

/*
 * Free the memory for a variable.
 */
    static void
free_var(varp)
    VAR	    varp;
{
    if (varp != NULL)
    {
	if (varp->var_type == VAR_STRING)
	    vim_free(varp->var_val.var_string);
	vim_free(varp->var_name);
	vim_free(varp);
    }
}

/*
 * Free the memory for a variable value and set the value to NULL or 0.
 */
    static void
clear_var(varp)
    VAR	    varp;
{
    if (varp != NULL)
    {
	if (varp->var_type == VAR_STRING)
	{
	    vim_free(varp->var_val.var_string);
	    varp->var_val.var_string = NULL;
	}
	else
	    varp->var_val.var_number = 0;
    }
}

/*
 * Get the number value of a variable.
 * If it is a String variable, use vim_str2nr().
 */
    static long
get_var_number(varp)
    VAR		varp;
{
    long	n;

    if (varp->var_type == VAR_NUMBER)
	return (long)(varp->var_val.var_number);
    else if (varp->var_type == VAR_UNKNOWN || varp->var_val.var_string == NULL)
	return 0L;
    else
    {
	vim_str2nr(varp->var_val.var_string, NULL, NULL, TRUE, TRUE, &n, NULL);
	return n;
    }
}

/*
 * Get the string value of a variable.
 * If it is a Number variable, the number is converted into a string.
 * get_var_string() uses a single, static buffer.  You can only use it once!
 * get_var_string_buf() uses a given buffer.
 * If the String variable has never been set, return an empty string.
 * Never returns NULL;
 */
    static char_u *
get_var_string(varp)
    VAR	    varp;
{
    static char_u   mybuf[NUMBUFLEN];

    return get_var_string_buf(varp, mybuf);
}

    static char_u *
get_var_string_buf(varp, buf)
    VAR	    varp;
    char_u  *buf;
{
    if (varp->var_type == VAR_NUMBER)
    {
	sprintf((char *)buf, "%ld", (long)varp->var_val.var_number);
	return buf;
    }
    else if (varp->var_val.var_string == NULL)
	return (char_u *)"";
    else
	return varp->var_val.var_string;
}

/*
 * Find variable "name" in the list of variables.
 * Return a pointer to it if found, NULL if not found.
 */
    static VAR
find_var(name, writing)
    char_u	*name;
    int		writing;
{
    int			i;
    char_u		*varname;
    struct growarray	*gap;

    /* Check for function arguments "a:" */
    if (name[0] == 'a' && name[1] == ':')
    {
	if (writing)
	{
	    EMSG2(e_readonlyvar, name);
	    return NULL;
	}
	name += 2;
	if (current_funccal == NULL)
	    return NULL;
	if (isdigit(*name))
	{
	    i = atol((char *)name);
	    if (i == 0)					/* a:0 */
		return &current_funccal->a0_var;
	    i += current_funccal->func->args.ga_len;
	    if (i > current_funccal->argcount)		/* a:999 */
		return NULL;
	    return &(current_funccal->argvars[i - 1]);	/* a:1, a:2, etc. */
	}
	if (STRCMP(name, "firstline") == 0)
	    return &(current_funccal->firstline);
	if (STRCMP(name, "lastline") == 0)
	    return &(current_funccal->lastline);
	for (i = 0; i < current_funccal->func->args.ga_len; ++i)
	    if (STRCMP(name, ((char_u **)
			      (current_funccal->func->args.ga_data))[i]) == 0)
	    return &(current_funccal->argvars[i]);	/* a:name */
	return NULL;
    }

    gap = find_var_ga(name, &varname);
    if (gap == NULL)
	return NULL;

    for (i = gap->ga_len; --i >= 0; )
	if (VAR_GAP_ENTRY(i, gap).var_name != NULL
		&& STRCMP(VAR_GAP_ENTRY(i, gap).var_name, varname) == 0)
	    break;
    if (i < 0)
	return NULL;
    return &VAR_GAP_ENTRY(i, gap);
}

/*
 * Find the growarray and start of name without ':' for a variable name.
 */
    static struct growarray *
find_var_ga(name, varname)
    char_u  *name;
    char_u  **varname;
{
    if (name[1] != ':')
    {
	*varname = name;
	if (current_funccal == NULL)
	    return &variables;			/* global variable */
	return &current_funccal->l_vars;	/* local function variable */
    }
    *varname = name + 2;
    if (*name == 'b')				/* buffer variable */
	return &curbuf->b_vars;
    if (*name == 'w')				/* window variable */
	return &curwin->w_vars;
    if (*name == 'g')				/* global variable */
	return &variables;
    if (*name == 'l' && current_funccal != NULL)/* local function variable */
	return &current_funccal->l_vars;
    return NULL;
}

/*
 * Initialize internal variables for use.
 */
    void
var_init(gap)
    struct growarray *gap;
{
    ga_init2(gap, (int)sizeof(var), 4);
}

/*
 * Clean up a list of internal variables.
 */
    void
var_clear(gap)
    struct growarray *gap;
{
    int	    i;

    for (i = gap->ga_len; --i >= 0; )
	var_free_one(&VAR_GAP_ENTRY(i, gap));
    ga_clear(gap);
}

    static void
var_free_one(v)
    VAR	    v;
{
    vim_free(v->var_name);
    v->var_name = NULL;
    if (v->var_type == VAR_STRING)
	vim_free(v->var_val.var_string);
    v->var_val.var_string = NULL;
}

/*
 * List the value of one internal variable.
 */
    static void
list_one_var(v, prefix)
    VAR		v;
    char_u	*prefix;
{
    list_one_var_a(prefix, v->var_name, v->var_type, get_var_string(v));
}

/*
 * List the value of one "v:" variable.
 */
    static void
list_vim_var(i)
    int		i;	/* index in vimvars[] */
{
    char_u	*p;
    char_u	numbuf[NUMBUFLEN];

    if (vimvars[i].type == VAR_NUMBER)
    {
	p = numbuf;
	sprintf((char *)p, "%ld", (long)vimvars[i].val);
    }
    else if (vimvars[i].val == NULL)
	p = (char_u *)"";
    else
	p = vimvars[i].val;
    list_one_var_a((char_u *)"v:", (char_u *)vimvars[i].name,
							  vimvars[i].type, p);
}

    static void
list_one_var_a(prefix, name, type, string)
    char_u	*prefix;
    char_u	*name;
    int		type;
    char_u	*string;
{
    msg_attr(prefix, 0);    /* don't use msg(), it overwrites "v:statusmsg" */
    if (name != NULL)	/* "a:" vars don't have a name stored */
	msg_puts(name);
    msg_putchar(' ');
    msg_advance(22);
    if (type == VAR_NUMBER)
	msg_putchar('#');
    else
	msg_putchar(' ');
    msg_outtrans(string);
}

/*
 * Set variable "name" to value in "varp".
 * If the variable already exists, the value is updated.
 * Otherwise the variable is created.
 */
    static void
set_var(name, varp)
    char_u	*name;
    VAR		varp;
{
    int			i;
    VAR			v;
    char_u		*varname;
    struct growarray	*gap;

    /*
     * Handle setting internal v: variables.
     */
    i = find_vim_var(name, (int)STRLEN(name));
    if (i >= 0)
    {
	if (vimvars[i].flags & VV_RO)
	    EMSG2(e_readonlyvar, name);
	else
	{
	    if (vimvars[i].type == VAR_STRING)
	    {
		vim_free(vimvars[i].val);
		vimvars[i].val = vim_strsave(get_var_string(varp));
	    }
	    else
		vimvars[i].val = (char_u *)varp->var_val.var_number;
	}
	return;
    }

    v = find_var(name, TRUE);
    if (v != NULL)	    /* existing variable, only need to free string */
    {
	if (v->var_type == VAR_STRING)
	    vim_free(v->var_val.var_string);
    }
    else		    /* add a new variable */
    {
	gap = find_var_ga(name, &varname);
	if (gap == NULL)    /* illegal name */
	    return;

	/* Try to use an empty entry */
	for (i = gap->ga_len; --i >= 0; )
	    if (VAR_GAP_ENTRY(i, gap).var_name == NULL)
		break;
	if (i < 0)	    /* need to allocate more room */
	{
	    if (ga_grow(gap, 1) == FAIL)
		return;
	    i = gap->ga_len;
	}
	v = &VAR_GAP_ENTRY(i, gap);
	if ((v->var_name = vim_strsave(varname)) == NULL)
	    return;
	if (i == gap->ga_len)
	{
	    ++gap->ga_len;
	    --gap->ga_room;
	}
    }

    v->var_type = varp->var_type;
    if (varp->var_type == VAR_STRING)
	v->var_val.var_string = vim_strsave(get_var_string(varp));
    else
	v->var_val.var_number = varp->var_val.var_number;
}

/*
 * Implementation of
 * ":echo expr1 ..."	print each argument separated with a space, add a
 *			newline at the end.
 * ":echon expr1 ..."	print each argument plain.
 */
    void
do_echo(eap, echo)
    EXARG	*eap;
    int		echo;	    /* TRUE for ":echo" command, FALSE for ":echon" */
{
    char_u	*arg = eap->arg;
    var		retvar;
    char_u	*p;
    int		needclr = TRUE;
    int		atstart = TRUE;

    if (eap->skip)
	++emsg_off;
    else if (echo)
	msg_start();
    while (*arg != NUL && *arg != '|' && *arg != '\n' && !got_int)
    {
	p = arg;
	if (eval1(&arg, &retvar, !eap->skip) == FAIL)
	{
	    EMSG2(e_invexpr2, p);
	    break;
	}
	if (!eap->skip)
	{
	    if (atstart)
		atstart = FALSE;
	    else if (echo)
		msg_puts_attr((char_u *)" ", echo_attr);
	    for (p = get_var_string(&retvar); *p != NUL && !got_int; ++p)
		if (*p == '\n' || *p == '\r' || *p == TAB)
		{
		    if (*p != TAB && needclr)
		    {
			/* remove any text still there from the command */
			msg_clr_eos();
			needclr = FALSE;
		    }
		    msg_putchar_attr(*p, echo_attr);
		}
		else
		{
#ifdef MULTI_BYTE
		    /* check multibyte */
		    if (is_dbcs && p[1] != NUL && IsLeadByte(*p))
		    {
			(void)msg_outtrans_len_attr(p, 2, echo_attr);
			++p;
		    }
		    else
#endif
			(void)msg_outtrans_len_attr(p, 1, echo_attr);
		}
	}
	clear_var(&retvar);
	arg = skipwhite(arg);
    }
    eap->nextcmd = check_nextcmd(arg);

    if (eap->skip)
	--emsg_off;
    else
    {
	/* remove text that may still be there from the command */
	if (needclr)
	    msg_clr_eos();
	if (echo)
	    msg_end();
    }
}

/*
 * Implementation of ":echohl {name}".
 */
    void
do_echohl(arg)
    char_u	*arg;
{
    int		id;

    id = syn_name2id(arg);
    if (id == 0)
	echo_attr = 0;
    else
	echo_attr = syn_id2attr(id);
}

/*
 * Implementation of
 * ":execute expr1 ..."	execute the result of an expression.
 */
    void
do_execute(eap, getline, cookie)
    EXARG	*eap;
    char_u	*(*getline) __ARGS((int, void *, int));
    void	*cookie;		/* argument for getline() */
{
    char_u	*arg = eap->arg;
    var		retvar;
    int		ret = OK;
    char_u	*p;
    struct growarray ga;
    int		len;

    ga_init2(&ga, 1, 80);

    if (eap->skip)
	++emsg_off;
    while (*arg != NUL && *arg != '|' && *arg != '\n')
    {
	p = arg;
	if (eval1(&arg, &retvar, !eap->skip) == FAIL)
	{
	    EMSG2(e_invexpr2, p);
	    ret = FAIL;
	    break;
	}

	if (!eap->skip)
	{
	    p = get_var_string(&retvar);
	    len = STRLEN(p);
	    if (ga_grow(&ga, len + 2) == FAIL)
	    {
		clear_var(&retvar);
		ret = FAIL;
		break;
	    }
	    if (ga.ga_len)
	    {
		((char_u *)(ga.ga_data))[ga.ga_len++] = ' ';
		--ga.ga_room;
	    }
	    STRCPY((char_u *)(ga.ga_data) + ga.ga_len, p);
	    ga.ga_room -= len;
	    ga.ga_len += len;
	}

	clear_var(&retvar);
	arg = skipwhite(arg);
    }

    if (ret != FAIL && ga.ga_data != NULL)
	do_cmdline((char_u *)ga.ga_data,
				 getline, cookie, DOCMD_NOWAIT|DOCMD_VERBOSE);

    ga_clear(&ga);

    if (eap->skip)
	--emsg_off;

    eap->nextcmd = check_nextcmd(arg);
}

    static char_u *
find_option_end(p)
    char_u  *p;
{
    while (isalnum(*p) || *p == '_')
	++p;
    return p;
}

/*
 * Handle the ":function" command.
 * "getline" can be NULL, in which case a line is obtained from the user.
 */
    void
do_function(eap, getline, cookie)
    EXARG	*eap;
    char_u	*(*getline) __ARGS((int, void *, int));
    void	*cookie;		/* argument for getline() */
{
    char_u	*theline;
    int		j;
    int		c;
    char_u	*name;
    char_u	*nameend;
    char_u	*p;
    char_u	*arg;
    struct growarray newargs;
    struct growarray newlines;
    int		varargs = FALSE;
    int		mustend = FALSE;
    int		flags = 0;
    struct ufunc *fp;
    int		indent;
    int		nesting;

    /*
     * ":function" without argument: list functions.
     */
    if (*eap->arg == NUL)
    {
	if (!eap->skip)
	    for (fp = firstfunc; fp != NULL; fp = fp->next)
		list_func_head(fp);
	return;
    }

    if (!isupper(*eap->arg))
    {
	EMSG2("Function name must start with a capital: %s", eap->arg);
	return;
    }

    /*
     * ":function func" with only function name: list function.
     */
    if (vim_strchr(eap->arg, '(') == NULL)
    {
	if (!eap->skip)
	{
	    fp = find_func(eap->arg);
	    if (fp != NULL)
	    {
		list_func_head(fp);
		for (j = 0; j < fp->lines.ga_len; ++j)
		{
		    msg_putchar('\n');
		    msg_prt_line(FUNCLINE(fp, j));
		}
		MSG("endfunction");
	    }
	    else
		EMSG2("Undefined function: %s", eap->arg);
	}
	return;
    }

    /*
     * ":function name(arg1, arg2)" Define function.
     */
    name = eap->arg;
    for (p = name; isalpha(*p) || isdigit(*p) || *p == '_'; ++p)
	;
    if (p == name)
    {
	EMSG("Function name required");
	return;
    }
    nameend = p;
    p = skipwhite(p);
    if (*p != '(')
    {
	EMSG2("Missing '(': %s", name);
	return;
    }
    p = skipwhite(p + 1);

    ga_init2(&newargs, (int)sizeof(char_u *), 3);
    ga_init2(&newlines, (int)sizeof(char_u *), 3);

    /*
     * Isolate the arguments: "arg1, arg2, ...)"
     */
    while (*p != ')')
    {
	if (p[0] == '.' && p[1] == '.' && p[2] == '.')
	{
	    varargs = TRUE;
	    p += 3;
	    mustend = TRUE;
	}
	else
	{
	    arg = p;
	    while (isalpha(*p) || isdigit(*p) || *p == '_')
		++p;
	    if (arg == p || isdigit(*arg))
	    {
		EMSG2("Illegal argument: %s", arg);
		goto erret;
	    }
	    if (ga_grow(&newargs, 1) == FAIL)
		goto erret;
	    c = *p;
	    *p = NUL;
	    arg = vim_strsave(arg);
	    if (arg == NULL)
		goto erret;
	    ((char_u **)(newargs.ga_data))[newargs.ga_len] = arg;
	    *p = c;
	    newargs.ga_len++;
	    newargs.ga_room--;
	    if (*p == ',')
		++p;
	    else
		mustend = TRUE;
	}
	p = skipwhite(p);
	if (mustend && *p != ')')
	{
	    EMSG2(e_invarg2, eap->arg);
	    goto erret;
	}
    }
    ++p;	/* skip the ')' */

    /* find extra arguments "range" and "abort" */
    for (;;)
    {
	p = skipwhite(p);
	if (STRNCMP(p, "range", 5) == 0)
	{
	    flags |= FC_RANGE;
	    p += 5;
	}
	else if (STRNCMP(p, "abort", 5) == 0)
	{
	    flags |= FC_ABORT;
	    p += 5;
	}
	else
	    break;
    }

    if (*p != NUL && *p != '"' && *p != '\n')
    {
	EMSG(e_trailing);
	goto erret;
    }

    /*
     * Read the body of the function, until ":endfunction" is found.
     */
    if (KeyTyped)
    {
	msg_putchar('\n');	    /* don't overwrite the function name */
	cmdline_row = msg_row;
    }
    indent = 2;
    nesting = 0;
    for (;;)
    {
	msg_scroll = TRUE;
	need_wait_return = FALSE;
	if (getline == NULL)
	    theline = getcmdline(':', 0L, indent);
	else
	    theline = getline(':', cookie, indent);
	lines_left = Rows - 1;
	if (theline == NULL)
	{
	    EMSG("Missing :endfunction");
	    goto erret;
	}

	/* Check for "endfunction" (should be more strict...). */
	for (p = theline; vim_iswhite(*p) || *p == ':'; ++p)
	    ;
	if (STRNCMP(p, "endf", 4) == 0 && nesting-- == 0)
	{
	    vim_free(theline);
	    break;
	}

	/* Increase indent inside "if" and "while", decrease at "end" */
	if (indent > 2 && STRNCMP(p, "end", 3) == 0)
	    indent -= 2;
	else if (STRNCMP(p, "if", 2) == 0 || STRNCMP(p, "wh", 2) == 0)
	    indent += 2;

	/* Check for defining a function inside this function. */
	if (STRNCMP(p, "fu", 2) == 0)
	{
	    p = skipwhite(skiptowhite(p));
	    if (isupper(*p))
	    {
		while (isalpha(*p) || isdigit(*p) || *p == '_')
		    ++p;
		if (*skipwhite(p) == '(')
		{
		    ++nesting;
		    indent += 2;
		}
	    }
	}
	if (ga_grow(&newlines, 1) == FAIL)
	    goto erret;
	((char_u **)(newlines.ga_data))[newlines.ga_len] = theline;
	newlines.ga_len++;
	newlines.ga_room--;
    }

    if (eap->skip)
	goto erret;

    /*
     * If there are no errors, add the function
     */
    *nameend = NUL;
    fp = find_func(name);
    if (fp != NULL)
    {
	if (!eap->forceit)
	{
	    EMSG2("Function %s already exists, use ! to replace", name);
	    goto erret;
	}
	if (fp->calls)
	{
	    EMSG2("Cannot redefine function %s: It is in use", name);
	    goto erret;
	}
	/* redefine existing function */
	ga_clear_strings(&(fp->args));
	ga_clear_strings(&(fp->lines));
    }
    else
    {
	fp = (struct ufunc *)alloc((unsigned)sizeof(struct ufunc));
	if (fp == NULL)
	    goto erret;
	name = vim_strsave(name);
	if (name == NULL)
	{
	    vim_free(fp);
	    goto erret;
	}
	/* insert the new function in the function list */
	fp->next = firstfunc;
	firstfunc = fp;
	fp->name = name;
    }
    fp->args = newargs;
    fp->lines = newlines;
    fp->varargs = varargs;
    fp->flags = flags;
    fp->calls = 0;
    return;

erret:
    ga_clear_strings(&newargs);
    ga_clear_strings(&newlines);
}

/*
 * List the head of the function: "name(arg1, arg2)".
 */
    static void
list_func_head(fp)
    struct ufunc *fp;
{
    int		j;

    MSG("function ");
    msg_puts(fp->name);
    msg_putchar('(');
    for (j = 0; j < fp->args.ga_len; ++j)
    {
	if (j)
	    MSG_PUTS(", ");
	msg_puts(FUNCARG(fp, j));
    }
    if (fp->varargs)
    {
	if (j)
	    MSG_PUTS(", ");
	MSG_PUTS("...");
    }
    msg_putchar(')');
}

/*
 * Find a function by name, return pointer to it in ufuncs.
 * Return NULL for unknown function.
 */
    static struct ufunc *
find_func(name)
    char_u	*name;
{
    struct ufunc *fp;

    for (fp = firstfunc; fp != NULL; fp = fp->next)
	if (STRCMP(name, fp->name) == 0)
	    break;
    return fp;
}

#ifdef CMDLINE_COMPL

/*
 * Function given to ExpandGeneric() to obtain the list of user defined
 * function names.
 */
    char_u *
get_user_func_name(idx)
    int	    idx;
{
    static struct ufunc *fp = NULL;
    char_u		*name;

    if (idx == 0)
	fp = firstfunc;
    if (fp != NULL)
    {
	name = fp->name;
	fp = fp->next;
	return name;
    }
    return NULL;
}

#endif /* CMDLINE_COMPL */

/*
 * Handle ":delfunction {name}".
 */
    void
do_delfunction(arg)
    char_u	*arg;
{
    struct ufunc *fp, *pfp;

    fp = find_func(arg);
    if (fp == NULL)
    {
	EMSG2("Undefined function: %s", arg);
	return;
    }
    if (fp->calls)
    {
	EMSG2("Cannot delete function %s: It is in use", arg);
	return;
    }

    /* clear this function */
    vim_free(fp->name);
    ga_clear_strings(&(fp->args));
    ga_clear_strings(&(fp->lines));

    /* remove the function from the function list */
    if (firstfunc == fp)
	firstfunc = fp->next;
    else
    {
	for (pfp = firstfunc; pfp != NULL; pfp = pfp->next)
	    if (pfp->next == fp)
	    {
		pfp->next = fp->next;
		break;
	    }
    }
    vim_free(fp);
}

/*
 * Call a user function.
 */
    static void
call_func(fp, argcount, argvars, retvar, firstline, lastline)
    struct ufunc *fp;		/* pointer to function */
    int		argcount;	/* nr of args */
    VAR		argvars;	/* arguments */
    VAR		retvar;		/* return value */
    linenr_t	firstline;	/* first line of range */
    linenr_t	lastline;	/* last line of range */
{
    char_u		*save_sourcing_name;
    linenr_t		save_sourcing_lnum;
    int			save_redrawing = RedrawingDisabled;
    struct funccall	fc;
    struct funccall	*save_fcp = current_funccal;
    int			save_did_emsg;
    static int		depth = 0;

    /* If depth of calling is getting too high, don't execute the function */
    if (depth >= p_mfd)
    {
	EMSG("Function call depth is higher than 'maxfuncdepth'");
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = -1;
	return;
    }
    ++depth;

    line_breakcheck();		/* check for CTRL-C hit */

    /* set local variables */
    var_init(&fc.l_vars);
    fc.func = fp;
    fc.argcount = argcount;
    fc.argvars = argvars;
    fc.retvar = retvar;
    retvar->var_val.var_number = 0;
    fc.linenr = 0;
    fc.a0_var.var_type = VAR_NUMBER;
    fc.a0_var.var_val.var_number = argcount - fp->args.ga_len;
    fc.a0_var.var_name = NULL;
    current_funccal = &fc;
    fc.firstline.var_type = VAR_NUMBER;
    fc.firstline.var_val.var_number = firstline;
    fc.firstline.var_name = NULL;
    fc.lastline.var_type = VAR_NUMBER;
    fc.lastline.var_val.var_number = lastline;
    fc.lastline.var_name = NULL;

    /* Don't redraw while executing the function. */
    RedrawingDisabled = TRUE;
    save_sourcing_name = sourcing_name;
    save_sourcing_lnum = sourcing_lnum;
    sourcing_lnum = 0;
    sourcing_name = alloc((unsigned)((save_sourcing_name == NULL ? 0
		: STRLEN(save_sourcing_name)) + STRLEN(fp->name) + 10));
    if (sourcing_name != NULL)
    {
	if (save_sourcing_name != NULL
			  && STRNCMP(save_sourcing_name, "function ", 9) == 0)
	    sprintf((char *)sourcing_name, "%s->", save_sourcing_name);
	else
	    STRCPY(sourcing_name, "function ");
	STRCAT(sourcing_name, fp->name);
    }
    save_did_emsg = did_emsg;
    did_emsg = FALSE;

    /* call do_cmdline() to execute the lines */
    do_cmdline(NULL, get_func_line, (void *)&fc,
				     DOCMD_NOWAIT|DOCMD_VERBOSE|DOCMD_REPEAT);

    RedrawingDisabled = save_redrawing;
    vim_free(sourcing_name);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;

    /* when the function was aborted because of an error, return -1 */
    if ((did_emsg && (fp->flags & FC_ABORT)) || retvar->var_type == VAR_UNKNOWN)
    {
	clear_var(retvar);
	retvar->var_type = VAR_NUMBER;
	retvar->var_val.var_number = -1;
    }
    did_emsg |= save_did_emsg;
    current_funccal = save_fcp;

    var_clear(&fc.l_vars);		/* free all local variables */
    --depth;
}

/*
 * Save the current function call pointer, and set it to NULL.
 * Used when executing autocommands and for ":source".
 */
    void *
save_funccal()
{
    struct funccall *fc;

    fc = current_funccal;
    current_funccal = NULL;
    return (void *)fc;
}

    void
restore_funccal(fc)
    void *fc;
{
    current_funccal = (struct funccall *)fc;
}

/*
 * Handle ":return [expr]".
 */
    void
do_return(eap)
    EXARG	*eap;
{
    char_u	*arg = eap->arg;
    var		retvar;
    char_u	*p;

    if (current_funccal == NULL)
    {
	EMSG(":return not inside a function");
	return;
    }

    if (eap->skip)
	++emsg_off;
    else
	current_funccal->linenr = -1;

    if (*arg != NUL && *arg != '|' && *arg != '\n')
    {
	p = arg;
	if (eval1(&arg, &retvar, !eap->skip) != FAIL)
	{
	    if (!eap->skip)
	    {
		clear_var(current_funccal->retvar);
		*current_funccal->retvar = retvar;
	    }
	    else
		clear_var(&retvar);
	}
	else
	    EMSG2(e_invexpr2, p);
    }

    /* when skipping, advance to the next command in this line.  When not
     * skipping, ignore the rest of the line.  Following lines will be ignored
     * by get_func_line(). */
    if (eap->skip)
    {
	--emsg_off;
	eap->nextcmd = check_nextcmd(arg);
    }
    else
	eap->nextcmd = NULL;
}

/*
 * Get next function line.
 * Called by do_cmdline() to get the next line.
 * Returns allocated string, or NULL for end of function.
 */
/* ARGSUSED */
    char_u *
get_func_line(c, cookie, indent)
    int	    c;		    /* not used */
    void    *cookie;
    int	    indent;	    /* not used */
{
    struct funccall  *fcp = (struct funccall *)cookie;
    char_u	    *retval;
    struct growarray *gap;  /* growarray with function lines */

    gap = &fcp->func->lines;
    if ((fcp->func->flags & FC_ABORT) && did_emsg)
	retval = NULL;
    else if (fcp->linenr < 0 || fcp->linenr >= gap->ga_len)
	retval = NULL;
    else
	retval = vim_strsave(((char_u **)(gap->ga_data))[fcp->linenr++]);

    if (p_verbose >= 15)
    {
	msg_scroll = TRUE;	    /* always scroll up, don't overwrite */
	if (retval == NULL)
	    msg((char_u *)"function returning");
	else
	    smsg((char_u *)"function line %s", retval);
	msg_puts((char_u *)"\n");   /* don't overwrite this either */
	cmdline_row = msg_row;
    }
    return retval;
}

/*
 * Return TRUE if the currently active function should be ended, because a
 * return was encountered or an error occured.  Used inside a ":while".
 */
    int
func_has_ended(cookie)
    void    *cookie;
{
    struct funccall  *fcp = (struct funccall *)cookie;

    return (((fcp->func->flags & FC_ABORT) && did_emsg) || fcp->linenr < 0);
}

/*
 * return TRUE if cookie indicates a function which "abort"s on errors.
 */
    int
func_has_abort(cookie)
    void    *cookie;
{
    return ((struct funccall *)cookie)->func->flags & FC_ABORT;
}

#if defined(VIMINFO) || defined(MKSESSION)
typedef enum
{
    VAR_FLAVOUR_DEFAULT,
    VAR_FLAVOUR_SESSION,
    VAR_FLAVOUR_VIMINFO
} VAR_FLAVOUR;

static VAR_FLAVOUR var_flavour __ARGS((char_u *varname));

    static VAR_FLAVOUR
var_flavour(varname)
    char_u *varname;
{
    char_u *p = varname;

    if (isupper(*p))
    {
	while (*(++p))
	    if (islower(*p))
		return VAR_FLAVOUR_SESSION;
	return VAR_FLAVOUR_VIMINFO;
    }
    else
	return VAR_FLAVOUR_DEFAULT;
}
#endif

#ifdef VIMINFO
/*
 * Restore global vars that start with a capital from the viminfo file
 */
    int
read_viminfo_varlist(line, fp, writing)
    char_u	*line;
    FILE	*fp;
    int		writing;
{
    char_u	*tab;
    int		is_string = FALSE;
    VAR		varp = NULL;
    char_u	*val;

    if (!writing && (find_viminfo_parameter('!') != NULL))
    {
	tab = vim_strchr(line + 1, '\t');
	if (tab != NULL)
	{
	    *tab++ = '\0';	/* isolate the variable name */
	    if (*tab == 'S')	/* string var */
		is_string = TRUE;

	    tab = vim_strchr(tab, '\t');
	    if (tab != NULL)
	    {
		/* create a nameless variable to hold the value */
		if (is_string)
		{
		    val = viminfo_readstring(tab + 1, fp);
		    if (val != NULL)
			varp = alloc_string_var(val);
		}
		else
		{
		    varp = alloc_var();
		    if (varp != NULL)
		    {
			varp->var_type = VAR_NUMBER;
			varp->var_val.var_number = atol((char *)tab + 1);
		    }
		}
		/* assign the value to the variable */
		if (varp != NULL)
		{
		    set_var(line + 1, varp);
		    free_var(varp);
		}
	    }
	}
    }

    return vim_fgets(line, LSIZE, fp);
}

/*
 * Write global vars that start with a capital to the viminfo file
 */
    void
write_viminfo_varlist(fp)
    FILE    *fp;
{
    struct growarray	*gap = &variables;		/* global variable */
    VAR			this_var;
    int			i;

    if (find_viminfo_parameter('!') == NULL)
	return;

    fprintf(fp, "\n# global variables:\n");
    for (i = gap->ga_len; --i >= 0; )
    {
	this_var = &VAR_GAP_ENTRY(i, gap);
	if (this_var->var_name != NULL
		&& var_flavour(this_var->var_name) == VAR_FLAVOUR_VIMINFO)
	{
	    fprintf(fp, "!%s\t%s\t", this_var->var_name,
			  (this_var->var_type == VAR_STRING) ? "STR" : "NUM");
	    viminfo_writestring(fp, get_var_string(this_var));
	}
    }
}
#endif

#ifdef MKSESSION
    int
store_session_globals(fd)
    FILE	*fd;
{
    struct growarray	*gap = &variables;		/* global variable */
    VAR			this_var;
    int			i;

    for (i = gap->ga_len; --i >= 0; )
    {
	this_var = &VAR_GAP_ENTRY(i, gap);
	if (this_var->var_name != NULL)
	{
	    if (var_flavour(this_var->var_name) == VAR_FLAVOUR_SESSION)
	    {
		if ((fprintf(fd, "let %s = %c%s%c",
			    this_var->var_name,
			    (this_var->var_type == VAR_STRING) ? '"' : ' ',
			    get_var_string(this_var),
			    (this_var->var_type == VAR_STRING) ? '"' : ' ') < 0)
			    || put_eol(fd) == FAIL)
		    return FAIL;
	    }

	}
    }
    return OK;
}
#endif

#endif /* WANT_EVAL */

#if defined(WANT_MODIFY_FNAME) || defined(WANT_EVAL) || defined(PROTO)

/*
 * Adjust a filename, according to a string of modifiers.
 * *fnamep must be NUL terminated when called.  When returning, the length is
 * determined by *fnamelen.
 * Returns valid flags.
 * When there is an error, *fnamep is set to NULL.
 */
    int
modify_fname(src, usedlen, fnamep, bufp, fnamelen)
    char_u	*src;		/* string with modifiers */
    int		*usedlen;	/* characters after src that are used */
    char_u	**fnamep;	/* file name so far */
    char_u	**bufp;		/* buffer for allocated file name or NULL */
    int		*fnamelen;	/* length of fnamep */
{
    int		valid = 0;
    char_u	*tail;
    char_u	*s, *p;
    char_u	dirname[MAXPATHL];
    int		c;

repeat:
    /* ":p" - full path/file_name */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 'p')
    {
	valid |= VALID_PATH;
	*usedlen += 2;
	/* FullName_save() is slow, don't use it when not needed. */
	if (!mch_isFullName(*fnamep))
	{
	    *fnamep = FullName_save(*fnamep, FALSE);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	}
    }

    /* ":." - path relative to the current directory */
    /* ":~" - path relative to the home directory */
    while (src[*usedlen] == ':' && ((c = src[*usedlen + 1]) == '.' || c == '~'))
    {
	*usedlen += 2;
	/* Need full path first (use expand_env() to remove a "~/") */
	if (c == '.' && **fnamep == '~')
	    p = expand_env_save(*fnamep);
	else
	    p = FullName_save(*fnamep, FALSE);
	if (p != NULL)
	{
	    if (c == '.')
	    {
		mch_dirname(dirname, MAXPATHL);
		s = shorten_fname(p, dirname);
		if (s != NULL)
		{
		    *fnamep = s;
		    vim_free(*bufp);	/* free any allocated file name */
		    *bufp = p;
		}
		else
		    vim_free(p);
	    }
	    else
	    {
		home_replace(NULL, p, dirname, MAXPATHL, TRUE);
		/* Only replace it when it starts with '~' */
		if (*dirname == '~')
		{
		    s = vim_strsave(dirname);
		    if (s != NULL)
		    {
			*fnamep = s;
			vim_free(*bufp);
			*bufp = s;
		    }
		}
		vim_free(p);
	    }
	}
    }

    tail = gettail(*fnamep);
    *fnamelen = STRLEN(*fnamep);

    /* ":h" - head, remove "/file_name", can be repeated  */
    /* Don't remove the first "/" or "c:\" */
    while (src[*usedlen] == ':' && src[*usedlen + 1] == 'h')
    {
	valid |= VALID_HEAD;
	*usedlen += 2;
	s = get_past_head(*fnamep);
	while (tail > s && vim_ispathsep(tail[-1]))
	    --tail;
	*fnamelen = tail - *fnamep;
#ifdef VMS
	if (*fnamelen > 0)
	    *fnamelen += 1; /* the path separator is part of the path */
#endif
	while (tail > s && !vim_ispathsep(tail[-1]))
	    --tail;
    }

    /* ":t" - tail, just the basename */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 't')
    {
	*usedlen += 2;
	*fnamelen -= tail - *fnamep;
	*fnamep = tail;
    }

    /* ":e" - extension, can be repeated */
    /* ":r" - root, without extension, can be repeated */
    while (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 'e' || src[*usedlen + 1] == 'r'))
    {
	/* find a '.' in the tail:
	 * - for second :e: before the current fname
	 * - otherwise: The last '.'
	 */
	if (src[*usedlen + 1] == 'e' && *fnamep > tail)
	    s = *fnamep - 2;
	else
	    s = *fnamep + *fnamelen - 1;
	for ( ; s > tail; --s)
	    if (s[0] == '.')
		break;
	if (src[*usedlen + 1] == 'e')		/* :e */
	{
	    if (s > tail)
	    {
		*fnamelen += *fnamep - (s + 1);
		*fnamep = s + 1;
#ifdef VMS
		/* cut version from the extension */
		s = *fnamep + *fnamelen - 1;
		for ( ; s > *fnamep; --s)
		    if (s[0] == ';')
			break;
		if (s > *fnamep)
		    *fnamelen = s - *fnamep;
#endif
	    }
	    else if (*fnamep <= tail)
		*fnamelen = 0;
	}
	else				/* :r */
	{
	    if (s > tail)	/* remove one extension */
		*fnamelen = s - *fnamep;
	}
	*usedlen += 2;
    }

    /* ":s?pat?foo?" - substitute */
    /* ":gs?pat?foo?" - global substitute */
    if (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 's'
		|| (src[*usedlen + 1] == 'g' && src[*usedlen + 2] == 's')))
    {
	char_u	    *str;
	char_u	    *pat;
	char_u	    *sub;
	int	    sep;
	char_u	    *flags;
	int	    didit = FALSE;

	flags = (char_u *)"";
	s = src + *usedlen + 2;
	if (src[*usedlen + 1] == 'g')
	{
	    flags = (char_u *)"g";
	    ++s;
	}

	sep = *s++;
	if (sep)
	{
	    /* find end of pattern */
	    p = vim_strchr(s, sep);
	    if (p != NULL)
	    {
		pat = vim_strnsave(s, (int)(p - s));
		if (pat != NULL)
		{
		    s = p + 1;
		    /* find end of substitution */
		    p = vim_strchr(s, sep);
		    if (p != NULL)
		    {
			sub = vim_strnsave(s, (int)(p - s));
			str = vim_strnsave(*fnamep, *fnamelen);
			if (sub != NULL && str != NULL)
			{
			    *usedlen = p + 1 - src;
			    s = do_string_sub(str, pat, sub, flags);
			    if (s != NULL)
			    {
				*fnamep = s;
				*fnamelen = STRLEN(s);
				vim_free(*bufp);
				*bufp = s;
				didit = TRUE;
			    }
			}
			vim_free(sub);
			vim_free(str);
		    }
		    vim_free(pat);
		}
	    }
	    /* after using ":s", repeat all the modifiers */
	    if (didit)
		goto repeat;
	}
    }

    return valid;
}

/*
 * Perform a substitution on "str" with pattern "pat" and substitute "sub".
 * "flags" can be "g" to do a global substitute.
 * Returns an allocated string, NULL for error.
 */
    char_u *
do_string_sub(str, pat, sub, flags)
    char_u	*str;
    char_u	*pat;
    char_u	*sub;
    char_u	*flags;
{
    int			sublen;
    vim_regexp		*prog;
    int			i;
    int			do_all;
    char_u		*tail;
    struct growarray	ga;
    char_u		*ret;
    char_u		*save_cpo;

    /* Make 'cpoptions' empty, so that the 'l' flag doesn't work here */
    save_cpo = p_cpo;
    p_cpo = (char_u *)"";

    ga_init2(&ga, 1, 200);

    do_all = (flags[0] == 'g');

    reg_ic = p_ic;
    prog = vim_regcomp(pat, TRUE);
    if (prog != NULL)
    {
	tail = str;
	while (vim_regexec(prog, tail, tail == str))
	{
	    /*
	     * Get some space for a temporary buffer to do the substitution
	     * into.  It will contain:
	     * - The text up to where the match is.
	     * - The substituted text.
	     * - The text after the match.
	     */
	    sublen = vim_regsub(prog, sub, tail, FALSE, TRUE);
	    if (ga_grow(&ga, (int)(STRLEN(tail) + sublen -
				  (prog->endp[0] - prog->startp[0]))) == FAIL)
	    {
		ga_clear(&ga);
		break;
	    }

	    /* copy the text up to where the match is */
	    i = prog->startp[0] - tail;
	    mch_memmove((char_u *)ga.ga_data + ga.ga_len, tail, (size_t)i);
	    /* add the substituted text */
	    (void)vim_regsub(prog, sub, (char_u *)ga.ga_data + ga.ga_len + i,
								  TRUE, TRUE);
	    ga.ga_len += i + sublen - 1;
	    ga.ga_room -= i + sublen - 1;
	    /* avoid getting stuck on a match with an empty string */
	    if (tail == prog->endp[0])
	    {
		if (*tail == NUL)
		    break;
		*((char_u *)ga.ga_data + ga.ga_len) = *tail++;
		++ga.ga_len;
		--ga.ga_room;
	    }
	    else
	    {
		tail = prog->endp[0];
		if (*tail == NUL)
		    break;
	    }
	    if (!do_all)
		break;
	}

	if (ga.ga_data != NULL)
	    STRCPY((char *)ga.ga_data + ga.ga_len, tail);

	vim_free(prog);
    }

    ret = vim_strsave(ga.ga_data == NULL ? str : (char_u *)ga.ga_data);
    ga_clear(&ga);
    p_cpo = save_cpo;

    return ret;
}

#endif /* defined(WANT_MODIFY_FNAME) || defined(WANT_EVAL) */
