/*****************************************************************************
*   $Id: options.h,v 8.13 1999/12/14 05:28:12 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Defines external interface to option processing.
*****************************************************************************/
#ifndef _OPTIONS_H
#define _OPTIONS_H

#ifdef OPTION_WRITE
# define CONST_OPTION
#else
# define CONST_OPTION const
#endif

/*============================================================================
=   Include files
============================================================================*/
#include "args.h"
#include "general.h"
#include "ctags.h"
#include "strlist.h"
#include "vstring.h"

/*============================================================================
=   Data declarations
============================================================================*/

typedef enum { OPTION_NONE, OPTION_SHORT, OPTION_LONG } optionType;

typedef struct sCookedArgs {
/* private */
    Arguments* args;
    char *shortOptions;
    char simple[2];
    boolean isOption;
    boolean longOption;
    const char* parameter;
/* public */
    char* item;
} cookedArgs;

/*  This stores the command line options.
 */
typedef struct sOptionValues {
    struct sInclude {		/* include tags for: */
	struct sCInclude {
	    boolean	classNames;
	    boolean	defines;
	    boolean	enumerators;
	    boolean	functions;
	    boolean	enumNames;
	    boolean	members;
	    boolean	namespaceNames;
	    boolean	prototypes;
	    boolean	structNames;
	    boolean	typedefs;
	    boolean	unionNames;
	    boolean	variables;
	    boolean	externVars;
	    boolean	access;
	    boolean	classPrefix;
	} cTypes;
	struct sEiffelInclude {
	    boolean	classNames;
	    boolean	features;
	    boolean	localEntities;
	    boolean	classPrefix;
	} eiffelTypes;
	struct sFortranInclude {
	    boolean	blockData;
	    boolean	commonBlocks;
	    boolean	entryPoints;
	    boolean	functions;
	    boolean	interfaces;
	    boolean	labels;
	    boolean	modules;
	    boolean	namelists;
	    boolean	programs;
	    boolean	subroutines;
	    boolean	types;
	} fortranTypes;
	struct sJavaInclude {
	    boolean	classNames;
	    boolean	fields;
	    boolean	interfaceNames;
	    boolean	methods;
	    boolean	packageNames;
	    boolean	access;
	    boolean	classPrefix;
	} javaTypes;
	boolean fileNames;	/* include tags for source file names */
	boolean	fileScope;	/* include tags of file scope only */
    } include;
    stringList* ignore;	    /* -I  name of file containing tokens to ignore */
    boolean append;	    /* -a  append to "tags" file */
    boolean backward;	    /* -B  regexp patterns search backwards */
    boolean etags;	    /* -e  output Emacs style tags file */
    enum eLocate {
	EX_MIX,		    /* line numbers for defines, patterns otherwise */
	EX_LINENUM,	    /* -n  only line numbers in tag file */
	EX_PATTERN	    /* -N  only patterns in tag file */
    } locate;		    /* --excmd  EX command used to locate tag */
    char *path;		    /* -p  default path for source files */
    boolean recurse;	    /* -R  recurse into directories */
    boolean sorted;	    /* -u,--sort  sort tags */
    boolean verbose;	    /* -V  verbose */
    boolean xref;	    /* -x  generate xref output instead */
    char *fileList;	    /* -L  name of file containing names of files */
    char *tagFileName;	    /* -o  name of tags file */
    stringList* headerExt;  /* -h  header extensions */
    stringList* etagsInclude;/* --etags-include  list of TAGS files to include*/
    unsigned int tagFileFormat;/* --format  tag file format (level) */
    boolean if0;	    /* --if0  examine code within "#if 0" branch */
    boolean kindLong;	    /* --kind-long */
    langType language;	    /* --lang specified language override */
    stringList* langMap[(int)LANG_COUNT];
			    /* --langmap  language-extension map */
    boolean followLinks;    /* --link  follow symbolic links? */
    boolean filter;	    /* --filter  behave as filter: files in, tags out */
    char* filterTerminator; /* --filter-terminator  string to output */
    boolean printTotals;    /* --totals  print cumulative statistics */
    boolean lineDirectives; /* --linedirectives  process #line directives */
#ifdef DEBUG
    int debugLevel;	    /* -D  debugging output */
    unsigned long breakLine;/* -b  source line at which to call lineBreak() */
#endif
} optionValues;

/*============================================================================
=   Global variables
============================================================================*/
extern CONST_OPTION optionValues	Option;

/*============================================================================
=   Function prototypes
============================================================================*/
extern cookedArgs* cArgNewFromString __ARGS((const char* string));
extern cookedArgs* cArgNewFromArgv __ARGS((char* const* const argv));
extern cookedArgs* cArgNewFromFile __ARGS((FILE* const fp));
extern cookedArgs* cArgNewFromLineFile __ARGS((FILE* const fp));
extern void cArgDelete __ARGS((cookedArgs* const current));
extern boolean cArgOff __ARGS((cookedArgs* const current));
extern boolean cArgIsOption __ARGS((cookedArgs* const current));
extern const char* cArgItem __ARGS((cookedArgs* const current));
extern void cArgForth __ARGS((cookedArgs* const current));

extern void setDefaultTagFileName __ARGS((void));
extern void checkOptions __ARGS((void));
extern void testEtagsInvocation __ARGS((void));
extern boolean isFileHeader __ARGS((const char *const fileName));
extern langType getFileLanguage __ARGS((const char *const fileName));
extern boolean isIgnoreToken __ARGS((const char *const name, boolean *const pIgnoreParens, const char **const replacement));
extern const char *getLanguageName __ARGS((const langType language));
extern void parseOption __ARGS((cookedArgs* const cargs));
extern void parseOptions __ARGS((cookedArgs* const cargs));
extern void previewFirstOption __ARGS((cookedArgs* const cargs));
extern void readOptionConfiguration __ARGS((void));
extern void initOptions __ARGS((void));
extern void freeOptionResources __ARGS((void));

#endif	/* _OPTIONS_H */

/* vi:set tabstop=8 shiftwidth=4: */
