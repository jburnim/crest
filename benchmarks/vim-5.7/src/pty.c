/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * The stuff in this file mostly comes from the "screen" program.
 * Copied from "pty.c" and "putenv.c".
 *
 * It has been modified to work better with Vim.
 * The parts that are not used in Vim have been deleted.
 * See the "screen" sources for the complete stuff.
 */

/* Copyright (c) 1993
 *	Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *	Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/* RCS_ID("$Id: pty.c,v 1.6 1994/05/31 12:32:44 mlschroe Exp $ FAU") */

#include "vim.h"

#define debug(x)
#define debug1(x, y)

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <signal.h>

#ifdef __CYGWIN32__
# include <sys/termios.h>
#endif

#if HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#if HAVE_STROPTS_H
#include <sys/types.h>
#include <stropts.h>
# ifdef sun
#  include <sys/conf.h>
# endif
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_TERMIO_H
# include <termio.h>
#endif

#if HAVE_SYS_STREAM_H
# include <sys/stream.h>
#endif

#if HAVE_SYS_PTEM_H
# include <sys/ptem.h>
#endif

#if !defined(sun) && !defined(VMS) && !defined(macintosh)
# include <sys/ioctl.h>
#endif

#if defined(sun) && defined(LOCKPTY) && !defined(TIOCEXCL)
# include <sys/ttold.h>
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#ifdef sgi
# include <sys/sysmacros.h>
#endif /* sgi */

#if defined(_INCLUDE_HPUX_SOURCE) && !defined(hpux)
# define hpux
#endif

/*
 * if no PTYRANGE[01] is in the config file, we pick a default
 */
#ifndef PTYRANGE0
# define PTYRANGE0 "qprs"
#endif
#ifndef PTYRANGE1
# define PTYRANGE1 "0123456789abcdef"
#endif

#if !(defined(sequent) || defined(_SEQUENT_) || defined(SVR4))
# ifdef hpux
static char PtyProto[] = "/dev/ptym/ptyXY";
static char TtyProto[] = "/dev/pty/ttyXY";
# else
#  ifdef __BEOS__
static char PtyProto[] = "/dev/pt/XY";
static char TtyProto[] = "/dev/tt/XY";
#  else
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#  endif
# endif
#endif

static void initpty __ARGS((int));

/*
 *  Open all ptys with O_NOCTTY, just to be on the safe side
 *  (RISCos mips breaks otherwise)
 */
#ifndef O_NOCTTY
# define O_NOCTTY 0
#endif

    static void
initpty(f)
    int f;
{
#ifndef VMS
# ifdef POSIX
    tcflush(f, TCIOFLUSH);
# else
#  ifdef TIOCFLUSH
    (void) ioctl(f, TIOCFLUSH, (char *) 0);
#  endif
# endif
# ifdef LOCKPTY
    (void) ioctl(f, TIOCEXCL, (char *) 0);
# endif
#endif
}

#if defined(OSX) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    static char TtyName[32];

    if ((f = open_controlling_pty(TtyName)) < 0)
	return -1;
    initpty(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if (defined(sequent) || defined(_SEQUENT_)) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    char	*m, *s;
    int		f;
    /* used for opening a new pty-pair: */
    static char PtyName[32];
    static char TtyName[32];

    if ((f = getpseudotty(&s, &m)) < 0)
	return -1;
#ifdef _SEQUENT_
    fvhangup(s);
#endif
    strncpy(PtyName, m, sizeof(PtyName));
    strncpy(TtyName, s, sizeof(TtyName));
    initpty(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(__sgi) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int f;
    char *name;
    RETSIGTYPE (*sigcld)__ARGS(SIGPROTOARG);

    /*
     * SIGCHLD set to SIG_DFL for _getpty() because it may fork() and
     * exec() /usr/adm/mkpts
     */
    sigcld = signal(SIGCHLD, SIG_DFL);
    name = _getpty(&f, O_RDWR | O_NONBLOCK | O_EXTRA, 0600, 0);
    signal(SIGCHLD, sigcld);

    if (name == 0)
	return -1;
    initpty(f);
    *ttyn = name;
    return f;
}
#endif

#if defined(MIPS) && defined(HAVE_DEV_PTC) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    struct stat buf;
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    if ((f = open("/dev/ptc", O_RDWR | O_NOCTTY | O_NONBLOCK | O_EXTRA, 0)) < 0)
	return -1;
    if (mch_fstat(f, &buf) < 0)
    {
	close(f);
	return -1;
    }
    sprintf(TtyName, "/dev/ttyq%d", minor(buf.st_rdev));
    initpty(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(SVR4) && !defined(PTY_DONE)

/* NOTE: Even though HPUX can have /dev/ptmx, the code below doesn't work! */
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    char	*m, *ptsname();
    int unlockpt __ARGS((int)), grantpt __ARGS((int));
    RETSIGTYPE (*sigcld)__ARGS(SIGPROTOARG);
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    if ((f = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_EXTRA, 0)) == -1)
	return -1;

    /*
     * SIGCHLD set to SIG_DFL for grantpt() because it fork()s and
     * exec()s pt_chmod
     */
    sigcld = signal(SIGCHLD, SIG_DFL);
    if ((m = ptsname(f)) == NULL || grantpt(f) || unlockpt(f))
    {
	signal(SIGCHLD, sigcld);
	close(f);
	return -1;
    }
    signal(SIGCHLD, sigcld);
    strncpy(TtyName, m, sizeof(TtyName));
    initpty(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(_AIX) && defined(HAVE_DEV_PTC) && !defined(PTY_DONE)
#define PTY_DONE

#ifdef _IBMR2
int aixhack = -1;
#endif

    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    /* a dumb looking loop replaced by mycrofts code: */
    if ((f = open ("/dev/ptc", O_RDWR | O_NOCTTY | O_EXTRA)) < 0)
	return -1;
    strncpy(TtyName, ttyname(f), sizeof(TtyName));
    if (geteuid() && mch_access(TtyName, R_OK | W_OK))
    {
	close(f);
	return -1;
    }
    initpty(f);
# ifdef _IBMR2
    if (aixhack >= 0)
	close(aixhack);
    if ((aixhack = open(TtyName, O_RDWR | O_NOCTTY | O_EXTRA, 0)) < 0)
    {
	close(f);
	return -1;
    }
# endif
    *ttyn = TtyName;
    return f;
}
#endif

#ifndef PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    char	*p, *q, *l, *d;
    int		f;
    /* used for opening a new pty-pair: */
    static char PtyName[32];
    static char TtyName[32];

    debug("OpenPTY: Using BSD style ptys.\n");
    strcpy(PtyName, PtyProto);
    strcpy(TtyName, TtyProto);
    for (p = PtyName; *p != 'X'; p++)
	;
    for (q = TtyName; *q != 'X'; q++)
	;
    for (l = PTYRANGE0; (*p = *l) != '\0'; l++)
    {
	for (d = PTYRANGE1; (p[1] = *d) != '\0'; d++)
	{
	    debug1("OpenPTY tries '%s'\n", PtyName);
#ifdef macintosh
	    if ((f = open(PtyName, O_RDWR | O_NOCTTY | O_EXTRA)) == -1)
#else
	    if ((f = open(PtyName, O_RDWR | O_NOCTTY | O_EXTRA, 0)) == -1)
#endif
		continue;
	    q[0] = *l;
	    q[1] = *d;
#ifndef macintosh
	    if (geteuid() && mch_access(TtyName, R_OK | W_OK))
	    {
		close(f);
		continue;
	    }
#endif
#if defined(sun) && defined(TIOCGPGRP) && !defined(SUNOS3)
	    /* Hack to ensure that the slave side of the pty is
	     * unused. May not work in anything other than SunOS4.1
	     */
	    {
		int pgrp;

		/* tcgetpgrp does not work (uses TIOCGETPGRP)! */
		if (ioctl(f, TIOCGPGRP, (char *)&pgrp) != -1 || errno != EIO)
		{
		    close(f);
		    continue;
		}
	    }
#endif
	    initpty(f);
	    *ttyn = TtyName;
	    return f;
	}
    }
    return -1;
}
#endif

/*
 * This causes a hang on some systems, but is required for a properly working
 * pty on others.  Needs to be tuned...
 */
#if defined(SVR4) && !defined(__sgi) && !defined(_AIX) && !defined(SCO)
# define USE_USG_PTYS
#endif

/*ARGSUSED*/
    int
SetupSlavePTY(fd)
    int fd;
{
#if defined(USE_USG_PTYS) && defined(I_PUSH)
# if HAVE_SYS_PTEM_H
    if (ioctl(fd, I_PUSH, "ptem") < 0)
	return -1;
# endif
    if (ioctl(fd, I_PUSH, "ldterm") < 0)
	return -1;
# ifdef SVR4
    if (ioctl(fd, I_PUSH, "ttcompat") < 0)
	return -1;
# endif
#endif
    return 0;
}

/* putenv.c */

/*
 *  putenv  --	put value into environment
 *
 *  Usage:  i = putenv (string)
 *    int i;
 *    char  *string;
 *
 *  where string is of the form <name>=<value>.
 *  Putenv returns 0 normally, -1 on error (not enough core for malloc).
 *
 *  Putenv may need to add a new name into the environment, or to
 *  associate a value longer than the current value with a particular
 *  name.  So, to make life simpler, putenv() copies your entire
 *  environment into the heap (i.e. malloc()) from the stack
 *  (i.e. where it resides when your process is initiated) the first
 *  time you call it.
 *
 *  (history removed, not very interesting.  See the "screen" sources.)
 */
/* RCS_ID("$Id: putenv.c,v 1.1.1.1 1993/06/16 23:51:15 jnweiger Exp $ FAU") */

#if !defined(HAVE_SETENV) && !defined(HAVE_PUTENV)

#define EXTRASIZE 5		/* increment to add to env. size */

static int  envsize = -1;	/* current size of environment */
#ifndef macintosh
extern
#endif
       char **environ;		/* the global which is your env. */

static int  findenv __ARGS((char *name)); /* look for a name in the env. */
static int  newenv __ARGS((void));	/* copy env. from stack to heap */
static int  moreenv __ARGS((void));	/* incr. size of env. */

    int
putenv(string)
    const char *string;
{
    int	    i;
    char    *p;

    if (envsize < 0)
    {				/* first time putenv called */
	if (newenv() < 0)	/* copy env. to heap */
	    return -1;
    }

    i = findenv((char *)string); /* look for name in environment */

    if (i < 0)
    {				/* name must be added */
	for (i = 0; environ[i]; i++);
	if (i >= (envsize - 1))
	{			/* need new slot */
	    if (moreenv() < 0)
		return -1;
	}
	p = (char *)alloc((unsigned)(strlen(string) + 1));
	if (p == NULL)		/* not enough core */
	    return -1;
	environ[i + 1] = 0;	/* new end of env. */
    }
    else
    {				/* name already in env. */
	p = vim_realloc(environ[i], strlen(string) + 1);
	if (p == NULL)
	    return -1;
    }
    sprintf(p, "%s", string);	/* copy into env. */
    environ[i] = p;

    return 0;
}

    static int
findenv(name)
    char *name;
{
    char    *namechar, *envchar;
    int	    i, found;

    found = 0;
    for (i = 0; environ[i] && !found; i++)
    {
	envchar = environ[i];
	namechar = name;
	while (*namechar && *namechar != '=' && (*namechar == *envchar))
	{
	    namechar++;
	    envchar++;
	}
	found = ((*namechar == '\0' || *namechar == '=') && *envchar == '=');
    }
    return found ? i - 1 : -1;
}

    static int
newenv()
{
    char    **env, *elem;
    int	    i, esize;

#ifdef macintosh
    /* for Mac a new, empty environment is created */
    i = 0;
#else
    for (i = 0; environ[i]; i++)
	;
#endif
    esize = i + EXTRASIZE + 1;
    env = (char **)alloc((unsigned)(esize * sizeof (elem)));
    if (env == NULL)
	return -1;

#ifndef macintosh
    for (i = 0; environ[i]; i++)
    {
	elem = (char *)alloc((unsigned)(strlen(environ[i]) + 1));
	if (elem == NULL)
	    return -1;
	env[i] = elem;
	strcpy(elem, environ[i]);
    }
#endif

    env[i] = 0;
    environ = env;
    envsize = esize;
    return 0;
}

    static int
moreenv()
{
    int	    esize;
    char    **env;

    esize = envsize + EXTRASIZE;
    env = (char **)vim_realloc((char *)environ, esize * sizeof (*env));
    if (env == 0)
	return -1;
    environ = env;
    envsize = esize;
    return 0;
}

# ifdef USE_VIMPTY_GETENV
    char_u *
vimpty_getenv(string)
    const char *string;
{
    int i;
    char_u *p;

    if (envsize < 0)
	return NULL;

    i = findenv((char *)string);

    if (i < 0)
	return NULL;

    p = vim_strchr((char_u *)environ[i], '=');
    return (p + 1);
}
# endif

#endif /* !defined(HAVE_SETENV) && !defined(HAVE_PUTENV) */
