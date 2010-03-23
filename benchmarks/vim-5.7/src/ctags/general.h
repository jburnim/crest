/*****************************************************************************
*   $Id: general.h,v 8.7 2000/01/06 05:48:13 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Provides the general (non-ctags-specific) environment assumed by all.
*****************************************************************************/
#ifndef _GENERAL_H
#define _GENERAL_H

/*============================================================================
=   Include files
============================================================================*/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/*============================================================================
=   Macros
============================================================================*/

#if defined(__STDC__) || defined(MSDOS) || defined(WIN32) || defined(OS2)
# define ENABLE_PROTOTYPES
#endif

/*  Determine whether to use prototypes or simple declarations.
 */
#ifndef __ARGS
# ifdef ENABLE_PROTOTYPES
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

/*  This is a helpful internal feature of later versions (> 2.7) of GCC
 *  to prevent warnings about unused variables.
 */
#if __GNUC__ > 2  ||  (__GNUC__ == 2  &&  __GNUC_MINOR__ >= 7)
# define __unused__	__attribute__((unused))
#else
# define __unused__
#endif

/*  MS-DOS doesn't allow manipulation of standard error, so we send it to
 *  stdout instead.
 */
#if defined(MSDOS) || defined(WIN32)
# define errout	stdout
#else
# define errout	stderr
#endif

#if defined(__STDC__) || defined(MSDOS) || defined(WIN32) || defined(OS2)
# define ENABLE_STDARG
#endif

#if defined(MSDOS) || defined(WIN32)
# define HAVE_DOS_H
# define HAVE_IO_H
# define HAVE_STDLIB_H
# define HAVE_TIME_H
# define HAVE_CLOCK
# define HAVE_CHSIZE
# define HAVE_FGETPOS
# define HAVE_STRERROR
# define HAVE_FINDNEXT
# ifdef __BORLANDC__
#  define HAVE_DIR_H
#  define HAVE_DIRENT_H
#  define HAVE_FINDFIRST
# else
#  ifdef _MSC_VER
#   define HAVE__FINDFIRST
#   define fortran avoid_fortran_clash	/* obsolete keyword in MSVC++ 5.0 */
#  else
#   ifdef __MINGW32__
#    define HAVE_DIR_H
#    define HAVE_DIRENT_H
#    define HAVE__FINDFIRST
#    define NEED_PROTO_FGETPOS
#    define ffblk _finddata_t
#    define FA_DIREC _A_SUBDIR
#    define ff_name name
#   endif
#  endif
# endif
#endif

#ifdef DJGPP
# define HAVE_DIR_H
# define HAVE_UNISTD_H
# define HAVE_FGETPOS
# define HAVE_FINDFIRST
# define HAVE_TRUNCATE
#endif

#if defined(OS2)
# define HAVE_DIRENT_H
# define HAVE_IO_H
# define HAVE_TIME_H
# define HAVE_STDLIB_H
# define HAVE_SYS_TYPES_H
# define HAVE_SYS_STAT_H
# define HAVE_UNISTD_H
# define HAVE_CLOCK
# define HAVE_CHSIZE
# define HAVE_FGETPOS
# define HAVE_OPENDIR
# define HAVE_STRERROR
# define HAVE_TRUNCATE
#endif

#ifdef AMIGA
# define HAVE_STDLIB_H
# define HAVE_SYS_STAT_H
# define HAVE_TIME_H
# define HAVE_CLOCK
# define HAVE_FGETPOS
# define HAVE_STRERROR
#endif

#ifdef __vms
# define HAVE_STDLIB_H
# define HAVE_TIME_H
# define HAVE_CLOCK
# define HAVE_FGETPOS
# define HAVE_STRERROR
#endif


#ifdef QDOS
# define HAVE_DIRENT_H
# define HAVE_STDLIB_H
# define HAVE_SYS_STAT_H
# define HAVE_SYS_TIMES_H
# define HAVE_SYS_TYPES_H
# define HAVE_TIME_H
# define HAVE_UNISTD_H
# define STDC_HEADERS
# define HAVE_CLOCK
# define HAVE_FGETPOS
# define HAVE_FTRUNCATE
# define HAVE_OPENDIR
# define HAVE_PUTENV
# define HAVE_REMOVE
# define HAVE_STRERROR
# define HAVE_STRSTR
# define HAVE_TIMES
# define HAVE_TRUNCATE
# define NON_CONST_PUTENV_PROTOTYPE
#endif

/*============================================================================
=   Data declarations
============================================================================*/

#undef FALSE
#undef TRUE
typedef enum { FALSE, TRUE } boolean;

#if !defined(HAVE_FGETPOS) && !defined(fpos_t)
# define fpos_t long
#endif

/*----------------------------------------------------------------------------
-	Possibly missing system prototypes.
----------------------------------------------------------------------------*/

#if defined(NEED_PROTO_REMOVE) && defined(HAVE_REMOVE)
extern int remove __ARGS((const char *));
#endif

#if defined(NEED_PROTO_UNLINK) && !defined(HAVE_REMOVE)
extern void *unlink __ARGS((const char *));
#endif

#ifdef NEED_PROTO_GETENV
extern char *getenv __ARGS((const char *));
#endif

#ifdef NEED_PROTO_STRSTR
extern char *strstr __ARGS((const char *str, const char *substr));
#endif

#endif	/* _GENERAL_H */

/* vi:set tabstop=8 shiftwidth=4: */
