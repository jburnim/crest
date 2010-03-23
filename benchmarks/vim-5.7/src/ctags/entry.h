/*****************************************************************************
*   $Id: entry.h,v 8.8 2000/04/13 05:38:19 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to entry.c
*****************************************************************************/
#ifndef _ENTRY_H
#define _ENTRY_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <stdio.h>

#include "ctags.h"
#include "vstring.h"

/*============================================================================
=   Macro definitions
============================================================================*/
#define WHOLE_FILE  -1L

/*============================================================================
=   Data declarations
============================================================================*/

/*  Maintains the state of the tag file.
 */
typedef struct eTagFile {
    char *name;
    FILE *fp;
    struct sNumTags { unsigned long added, prev; } numTags;
    struct sMax { size_t line, tag, file; } max;
    struct sEtags {
	char *name;
	FILE *fp;
	size_t byteCount;
    } etags;
    vString *vLine;
} tagFile;

typedef struct sTagFields {
    unsigned int count;		/* number of additional extension flags */
    const char *const *label;	/* list of labels for extension flags */
    const char *const *value;	/* list of values for extension flags */
} tagFields;

/*  Information about the current tag candidate.
 */
typedef struct sTagEntryInfo {
    boolean	lineNumberEntry;/* pattern or line number entry */
    unsigned long lineNumber;	/* line number of tag */
    fpos_t	filePosition;	/* file position of line containing tag */
    boolean	isFileScope;	/* is tag visibile only within source file? */
    boolean	isFileEntry;	/* is this just an entry for a file name? */
    boolean	truncateLine;	/* truncate tag line at end of tag name? */
    const char *sourceFileName;	/* name of source file */
    const char *name;		/* name of the tag */
    const char *kindName;	/* kind of tag */
    char	kind;		/* single character representation of kind */
    tagFields	otherFields;	/* list of other entries as "label:value" */
} tagEntryInfo;

/*============================================================================
=   Global variables
============================================================================*/
extern tagFile TagFile;

/*============================================================================
=   Function prototypes
============================================================================*/
extern void freeTagFileResources __ARGS((void));
extern const char *tagFileName __ARGS((void));
extern void copyBytes __ARGS((FILE* const fromFp, FILE* const toFp, const long size));
extern void copyFile __ARGS((const char *const from, const char *const to, const long size));
extern void openTagFile __ARGS((void));
extern void closeTagFile __ARGS((const boolean resize));
extern void beginEtagsFile __ARGS((void));
extern void endEtagsFile __ARGS((const char *const name));
extern void makeTagEntry __ARGS((const tagEntryInfo *const tag));
extern void initTagEntry __ARGS((tagEntryInfo *const e, const char *const name));

#endif	/* _ENTRY_H */

/* vi:set tabstop=8 shiftwidth=4: */
