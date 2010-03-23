/*****************************************************************************
*   $Id: vstring.h,v 8.1 1999/03/04 04:16:38 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Provides the external interface for resizeable strings.
*****************************************************************************/
#ifndef _VSTRING_H
#define _VSTRING_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#if defined(HAVE_STDLIB_H)
# include <stdlib.h>	/* to define size_t */
#endif

/*============================================================================
=   Macros
============================================================================*/
#define vStringValue(vs)	((vs)->buffer)
#define vStringLength(vs)	((vs)->length)
#define vStringSize(vs)		((vs)->size)
#define vStringCat(vs,s)	vStringCatS((vs), vStringValue((s)))
#define vStringNCat(vs,s,l)	vStringNCatS((vs), vStringValue((s)), (l))
#define vStringCopy(vs,s)	vStringCopyS((vs), vStringValue((s)))
#define vStringNCopy(vs,s,l)	vStringNCopyS((vs), vStringValue((s)), (l))
#define vStringChar(vs,i)	((vs)->buffer[i])
#define vStringTerminate(vs)	vStringPut(vs, '\0')

/*============================================================================
=   Data declarations
============================================================================*/

typedef struct sVString {
    size_t	length;		/* size of buffer used */
    size_t	size;		/* allocated size of buffer */
    char *	buffer;		/* location of buffer */
} vString;

/*============================================================================
=   Function prototypes
============================================================================*/
extern boolean vStringAutoResize __ARGS((vString *const string));
extern void vStringClear __ARGS((vString *const string));
extern vString *vStringNew __ARGS((void));
extern void vStringDelete __ARGS((vString *const string));
extern void vStringPut __ARGS((vString *const string, const int c));
extern void vStringStrip __ARGS((vString *const string));
extern void vStringCatS __ARGS((vString *const string, const char *const s));
extern void vStringNCatS __ARGS((vString *const string, const char *const s, const size_t length));
extern vString *vStringNewInit __ARGS((const char *const s));
extern void vStringCopyS __ARGS((vString *const string, const char *const s));
extern void vStringNCopyS __ARGS((vString *const string, const char *const s, const size_t length));
extern void vStringCopyToLower __ARGS((vString *const dest, vString *const src));
extern void vStringSetLength __ARGS((vString *const string));

#endif	/* _VSTRING_H */
