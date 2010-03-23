/*****************************************************************************
*   $Id: options.c,v 8.27 2000/04/23 16:12:17 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions to process command line options.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>	/* to declare toupper() */

#include "ctags.h"
#define OPTION_WRITE
#include "options.h"

#include "main.h"
#include "debug.h"

/*============================================================================
=   Defines
============================================================================*/

#if defined(MSDOS) || defined(WIN32) || defined(OS2) || defined(__vms)
# define CASE_INSENSITIVE_OS
#endif

#define INVOCATION  "Usage: %s [options] [file(s)]\n"

#define CTAGS_ENVIRONMENT	"CTAGS"
#define ETAGS_ENVIRONMENT	"ETAGS"

#define CTAGS_FILE	"tags"
#define ETAGS_FILE	"TAGS"

#ifndef ETAGS
# define ETAGS	"etags"		/* name which causes default use of to -e */
#endif

/*  The following separators are permitted for list options.
 */
#define EXTENSION_SEPARATORS   "."
#define IGNORE_SEPARATORS   ", \t\n"

#ifndef DEFAULT_FILE_FORMAT
# define DEFAULT_FILE_FORMAT	2
#endif

#if defined(MSDOS) || defined(WIN32) || defined(OS2) || defined(AMIGA) || defined(HAVE_OPENDIR)
# define RECURSE_SUPPORTED
#endif

#define isCompoundOption(c)	(boolean)(strchr("fohiILpDb",(c)) != NULL)

#define shortOptionPending(c) \
	(boolean)((c)->shortOptions != NULL  &&  *(c)->shortOptions != '\0')

/*============================================================================
=   Data declarations
============================================================================*/

enum eOptionLimits {
    MaxHeaderExtensions	= 100,	/* maximum number of extensions in -h option */
    MaxSupportedTagFormat = 2
};

typedef struct sOptionDescription {
    int usedByEtags;
    const char *const description;
} optionDescription;

typedef void (*parametricOptionHandler) __ARGS((const char *const option, const char *const parameter));

typedef const struct {
    const char* name;
    parametricOptionHandler handler;
    boolean initOnly;
} parametricOption;

typedef const struct {
    const char* name;
    boolean* pValue;
    boolean defaultValue;
    boolean initOnly;
} booleanOption;

/*============================================================================
=   Data definitions
============================================================================*/

static boolean ReachedFileNames = FALSE;

static boolean StartedAsEtags = FALSE;

static const char *const CExtensions[] = {
    "c", NULL
};
static const char *const CppExtensions[] = {
    "c++", "cc", "cpp", "cxx", "h", "hh", "hpp", "hxx", "h++",
#ifndef CASE_INSENSITIVE_OS
    "C", "H",
#endif
    NULL
};
static const char *const EiffelExtensions[] = {
    "e", NULL
};
static const char *const FortranExtensions[] = {
    "f", "for", "ftn", "f77", "f90", "f95",
#ifndef CASE_INSENSITIVE_OS
    "F", "FOR", "FTN", "F77", "F90", "F95",
#endif
    NULL
};
static const char *const JavaExtensions[] = {
    "java", NULL
};
static const char *const HeaderExtensions[] = {
    "h", "hh", "hpp", "hxx", "h++", "inc", "def",
#ifndef CASE_INSENSITIVE_OS
    "H",
#endif
    NULL
};

static const char *const *const DefaultLanguageMap[LANG_COUNT] = {
    NULL,
    CExtensions,
    CppExtensions,
    EiffelExtensions,
    FortranExtensions,
    JavaExtensions
};

optionValues Option = {
    {	    /* include */
	{	/* c */
	    TRUE,		/* -ic */
	    TRUE,		/* -id */
	    TRUE,		/* -ie */
	    TRUE,		/* -if */
	    TRUE,		/* -ig */
	    TRUE,		/* -im */
	    TRUE,		/* -in */
	    FALSE,		/* -ip */
	    TRUE,		/* -is */
	    TRUE,		/* -it */
	    TRUE,		/* -iu */
	    TRUE,		/* -iv */
	    FALSE,		/* -ix */
	    FALSE,		/* -iA */
	    FALSE		/* -iC */
	},
	{	/* eiffel */
	    TRUE,		/* -c */
	    TRUE,		/* -f */
	    FALSE,		/* -l */
	    FALSE		/* -C */
	},
	{	/* fortran */
	    TRUE,		/* -b */
	    TRUE,		/* -c */
	    TRUE,		/* -e */
	    TRUE,		/* -f */
	    TRUE,		/* -i */
	    TRUE,		/* -l */
	    TRUE,		/* -m */
	    TRUE,		/* -n */
	    TRUE,		/* -p */
	    TRUE,		/* -s */
	    TRUE		/* -t */
	},
	{	/* java */
	    TRUE,		/* -c */
	    TRUE,		/* -f */
	    TRUE,		/* -i */
	    TRUE,		/* -m */
	    TRUE,		/* -p */
	    FALSE,		/* -iA */
	    FALSE		/* -iC */
	},
	FALSE,		/* -iF */
	TRUE,		/* -iS */
    },
    NULL,		/* -I */
    FALSE,		/* -a */
    FALSE,		/* -B */
    FALSE,		/* -e */
#ifdef MACROS_USE_PATTERNS
    EX_PATTERN,		/* -n, --excmd */
#else
    EX_MIX,		/* -n, --excmd */
#endif
    NULL,		/* -p */
    FALSE,		/* -R */
    TRUE,		/* -u, --sort */
    FALSE,		/* -V */
    FALSE,		/* -x */
    NULL,		/* -L */
    NULL,		/* -o */
    NULL,		/* -h */
    NULL,		/* --etags-include */
    DEFAULT_FILE_FORMAT,/* --format */
    FALSE,		/* --if0 */
    FALSE,		/* --kind-long */
    LANG_AUTO,		/* --lang */
    {			/* --langmap */
	NULL, NULL, NULL, NULL, NULL, NULL
    },
    TRUE,		/* --links */
    FALSE,		/* --filter */
    NULL,		/* --filter-terminator */
    FALSE,		/* --totals */
    FALSE,		/* --line-directives */
#ifdef DEBUG
    0, 0		/* -D, -b */
#endif
};

/*----------------------------------------------------------------------------
-   Locally used only
----------------------------------------------------------------------------*/

static optionDescription LongOptionDescription[] = {
 {1,"  -a   Append the tags to an existing tag file."},
#ifdef DEBUG
 {1,"  -b <line>"},
 {1,"       Set break line."},
#endif
 {0,"  -B   Use backward searching patterns (?...?)."},
#ifdef DEBUG
 {1,"  -D <level>"},
 {1,"       Set debug level."},
#endif
 {0,"  -e   Output tag file for use with Emacs."},
 {1,"  -f <name>"},
 {1,"       Output tags to the specified file (default is \"tags\"; or \"TAGS\""},
 {1,"       if -e is specified). If specified as \"-\", tags are written to"},
 {1,"       standard output."},
 {0,"  -F   Use forward searching patterns (/.../) (default)."},
 {1,"  -h <list>"},
 {1,"       Specifies a list of file extensions used for headers."},
 {1,"       The default list is \".h.H.hh.hpp.hxx.h++\"."},
 {1,"  -i <types>"},
 {1,"       Nearly equivalent to --c-types=<types>."},
 {1,"  -I <list | file>"},
 {1,"       A list of tokens to be specially handled is read from either the"},
 {1,"       command line or the specified file."},
 {1,"  -L <file>"},
 {1,"       A list of source file names are read from the specified file."},
 {1,"       If specified as \"-\", then standard input is read."},
 {0,"  -n   Equivalent to --excmd=number."},
 {0,"  -N   Equivalent to --excmd=pattern."},
 {1,"  -o   Alternative for -f."},
 {1,"  -p <path>"},
 {1,"       Default path to use for all (relative path) filenames."},
#ifdef RECURSE_SUPPORTED
 {1,"  -R   Equivalent to --recurse."},
#else
 {1,"  -R   Not supported on this platform."},
#endif
 {0,"  -u   Equivalent to --sort=no."},
 {1,"  -V   Equivalent to --verbose."},
 {1,"  -x   Print a tabular cross reference file to standard output."},
 {1,"  --append=[yes|no]"},
 {1,"       Indicates whether tags should be appended to existing tag file"},
 {1,"       (default=no)."},
 {1,"  --c-types=types"},
 {1,"       Specifies a list of C/C++ language tag types to include in the"},
 {1,"       output file. \"Types\" is a group of one-letter flags designating"},
 {1,"       types of tags to either include or exclude from the output. Each"},
 {1,"       letter or group of letters may be preceded by either '+' (default,"},
 {1,"       if omitted) to add it to those already included, or '-' to exclude"},
 {1,"       it from the output. In the absence of any preceding '+' or '-'"},
 {1,"       sign, only those types listed in \"types\" will be included in the"},
 {1,"       output. Tags for the following language contructs are supported"},
 {1,"       (types are enabled by default except as noted):"},
 {1,"          c   classes"},
 {1,"          d   macro definitions"},
 {1,"          e   enumerators (values inside an enumeration)"},
 {1,"          f   function definitions"},
 {1,"          g   enumeration names"},
 {1,"          m   class, struct, and union members"},
 {1,"          n   namespaces"},
 {1,"          p   function prototypes [off]"},
 {1,"          s   structure names"},
 {1,"          t   typedefs"},
 {1,"          u   union names"},
 {1,"          v   variable definitions"},
 {1,"          x   external variable declarations [off]"},
 {1,"       In addition, the following modifiers are accepted:"},
 {1,"          A   record the access of members into the tag file [off]"},
 {1,"          C   include extra, class-qualified tag entries for members [off]"},
 {1,"  --etags-include=file"},
 {1,"      Include reference to 'file' in Emacs-style tag file (requires -e)."},
 {0,"  --excmd=number|pattern|mix"},
#ifdef MACROS_USE_PATTERNS
 {0,"       Uses the specified type of EX command to locate tags (default=pattern)."},
#else
 {0,"       Uses the specified type of EX command to locate tags (default=mix)."},
#endif
 {1,"  --eiffel-types=types"},
 {1,"       Specifies a list of Eiffel language tag types to be included in the"},
 {1,"       output. See --c-types for the definition of the format of \"types\"."},
 {1,"       Tags for the following Eiffel language contructs are supported"},
 {1,"       (types are enabled by default except as noted):"},
 {1,"          c   classes"},
 {1,"          f   features"},
 {1,"          l   local entities [off]"},
 {1,"       In addition, the following modifiers are accepted:"},
 {1,"          C   include extra, class-qualified tag entries for members [off]"},
 {1,"  --file-scope=[yes|no]"},
 {1,"       Indicates whether tags scoped only for a single file (e.g. \"static\""},
 {1,"       tags) should be included in the output (default=yes)."},
 {1,"  --file-tags=[yes|no]"},
 {1,"       Indicates whether tags should be generated for source file names"},
 {1,"       (default=no)."},
 {0,"  --format=level"},
#if DEFAULT_FILE_FORMAT==1
 {0,"       Forces output of specified tag file format (default=1)."},
#else
 {0,"       Forces output of specified tag file format (default=2)."},
#endif
 {1,"  --fortran-types=types"},
 {1,"       Specifies a list of Fortran language tag types to be included in the"},
 {1,"       output. See --c-types for the definition of the format of \"types\"."},
 {1,"       Tags for the following Fortran language contructs are supported"},
 {1,"       (all are enabled by default):"},
 {1,"          b   block data"},
 {1,"          c   common blocks"},
 {1,"          e   entry points"},
 {1,"          f   functions"},
 {1,"          i   interfaces"},
 {1,"          l   labels"},
 {1,"          m   modules"},
 {1,"          m   namelists"},
 {1,"          p   programs"},
 {1,"          s   subroutines"},
 {1,"          t   derived types"},
 {1,"  --help"},
 {1,"       Prints this option summary."},
 {1,"  --if0=[yes|no]"},
 {1,"       Indicates whether code within #if 0 conditional branches should"},
 {1,"       be examined for tags (default=no)."},
 {1,"  --java-types=types"},
 {1,"       Specifies a list of Java language tag types to be included in the"},
 {1,"       output. See --c-types for the definition of the format of \"types\"."},
 {1,"       Tags for the following Java language contructs are supported (all"},
 {1,"       are enabled by default):"},
 {1,"          c   classes"},
 {1,"          f   fields"},
 {1,"          i   interfaces"},
 {1,"          m   methods"},
 {1,"          p   packages"},
 {1,"       In addition, the following modifiers are accepted:"},
 {1,"          A   record the access of fields into the tag file [off]"},
 {1,"          C   include extra, class-qualified tag entries for fields [off]"},
 {1,"  --kind-long=[yes|no]"},
 {1,"       Indicates whether verbose tag descriptions are placed into tag file"},
 {1,"       (default=no)."},
 {1,"  --lang=[auto|c|c++|eiffel|fortran|java]"},
 {1,"       Interpret files as being of specified language [default=auto]."},
 {1,"  --langmap=map(s)"},
 {1,"       Overrides the default mapping of language to source file extension."},
 {0,"  --line-directives=[yes|no]"},
 {0,"       Indicates whether #line directives should be processed (default=no)."},
 {1,"  --links=[yes|no]"},
 {1,"       Indicates whether symbolic links should be followed (default=yes)."},
 {1,"  --filter=yes|no"},
 {1,"       Behave as a filter, reading file names from standard input and"},
 {1,"       writing tags to standard output [default=no]."},
 {1,"  --filter-terminator=string"},
 {1,"       Specifies a string to print to standard output following the tags"},
 {1,"       for each file parsed when --filter is enabled."},
 {1,"  --recurse=[yes|no]"},
#ifdef RECURSE_SUPPORTED
 {1,"       Recurse into directories supplied on command line (default=no)."},
#else
 {1,"       Not supported on this platform."},
#endif
 {0,"  --sort=[yes|no]"},
 {0,"       Indicates whether tags should be sorted (default=yes)."},
 {1,"  --totals=[yes|no]"},
 {1,"       Prints statistics about source and tag files (default=no)."},
 {1,"  --verbose=[yes|no]"},
 {1,"       Enable verbose messages describing actions on each source file."},
 {1,"  --version"},
 {1,"       Prints a version identifier to standard output."},
 {1, NULL}
};

/*  Contains a set of strings describing the set of "features" compiled into
 *  the code.
 */
static const char *const Features[] = {
#ifdef DEBUG
    "debug",
#endif
#ifdef WIN32
    "win32",
#endif
#ifdef DJGPP
    "msdos_32",
#else
# ifdef MSDOS
    "msdos_16",
# endif
#endif
#ifdef OS2
    "os2",
#endif
#ifdef AMIGA
    "amiga",
#endif
#ifdef __vms
    "vms",
#endif
#ifndef EXTERNAL_SORT
    "internal_sort",
#endif
#ifdef CUSTOM_CONFIGURATION_FILE
    "custom-conf",
#endif
    NULL
};

/*============================================================================
=   Function prototypes
============================================================================*/
static char *stringCopy __ARGS((const char *const string));
static void freeString __ARGS((char **const pString));
static void freeList __ARGS((stringList** const pString));

static void addExtensionList __ARGS((stringList *const slist, const char *const elist, const boolean clear));
static const char *findExtension __ARGS((const char *const fileName));
static langType getExtensionLanguage __ARGS((const char *const extension));
static langType getLangType __ARGS((const char *const name));
static void processLangOption __ARGS((const char *const option, const char *const parameter));
static boolean installLangMap __ARGS((char *const map));
static void installLangMapDefault __ARGS((const langType language));
static void installLangMapDefaults __ARGS((void));
static void freeLanguageMaps __ARGS((void));
static void processLangMapOption __ARGS((const char *const option, const char *const parameter));
static void installHeaderListDefaults __ARGS((void));
static void processHeaderListOption __ARGS((const int option, const char *parameter));

static void processCTypesOption __ARGS((const char *const option, const char *const parameter));
static void processEiffelTypesOption __ARGS((const char *const option, const char *const parameter));
static void processFortranTypesOption __ARGS((const char *const option, const char *const parameter));
static void processJavaTypesOption __ARGS((const char *const option, const char *const parameter));

static void saveIgnoreToken __ARGS((vString *const ignoreToken));
static void readIgnoreList __ARGS((const char *const list));
static void readIgnoreListFromFile __ARGS((const char *const fileName));
static void processIgnoreOption __ARGS((const char *const list));

static void printfFeatureList __ARGS((FILE *const where));
static void printProgramIdentification __ARGS((FILE *const where));
static void printInvocationDescription __ARGS((FILE *const where));
static void printOptionDescriptions __ARGS((const optionDescription *const optDesc, FILE *const where));
static void printHelp __ARGS((const optionDescription *const optDesc));
static void processExcmdOption __ARGS((const char *const option, const char *const parameter));
static void processFormatOption __ARGS((const char *const option, const char *const parameter));
static void processEtagsInclude __ARGS((const char *const option, const char *const parameter));
static void processFilterTerminatorOption __ARGS((const char *const option, const char *const parameter));
static void checkOptionOrder __ARGS((const char* const option));

static boolean getBooleanOption __ARGS((const char *const option, const char *const parameter));
static boolean processBooleanOption __ARGS((const char *const option, const char *const parameter));
static boolean processParametricOption __ARGS((const char *const option, const char *const parameter));
static void processLongOption __ARGS((const char *const option, const char *const parameter));
static void processShortOption __ARGS((const char *const option, const char *const parameter));
static void parseFileOptions __ARGS((const char *const fileName));
static void parseConfigurationFileOptions __ARGS((void));
static void parseEnvironmentOptions __ARGS((void));
static void parseShortOption __ARGS((cookedArgs *const args));
static void parseLongOption __ARGS((cookedArgs *const args, const char *item));

static void cArgRead __ARGS((cookedArgs *const current));

/*============================================================================
=   Function definitions
============================================================================*/

static char* stringCopy( string )
    const char* const string;
{
    char* result = NULL;
    if (string != NULL)
    {
	result = (char*)eMalloc(strlen(string) + 1);
	strcpy(result, string);
    }
    return result;
}

static void freeString( pString )
    char** const pString;
{
    if (*pString != NULL)
    {
	eFree(*pString);
	*pString = NULL;
    }
}

static void freeList( pList )
    stringList** const pList;
{
    if (*pList != NULL)
    {
	stringListDelete(*pList);
	*pList = NULL;
    }
}

extern void setDefaultTagFileName()
{
    if (Option.tagFileName != NULL)
	;		/* accept given name */
    else if (Option.etags)
	Option.tagFileName = stringCopy(ETAGS_FILE);
    else
	Option.tagFileName = stringCopy(CTAGS_FILE);
}

extern void checkOptions()
{
    const char* notice;
    if (Option.xref)
    {
	notice = "xref output";
	if (Option.include.fileNames)
	{
	    error(WARNING, "%s disables file name tags", notice);
	    Option.include.fileNames = FALSE;
	}
    }
    if (Option.append)
    {
	notice = "append mode is not compatible with";
	if (isDestinationStdout())
	    error(FATAL, "%s tags to stdout", notice);
    }
    if (Option.filter)
    {
	notice = "filter mode";
	if (Option.printTotals)
	{
	    error(WARNING, "%s disables totals", notice);
	    Option.printTotals = FALSE;
	}
	if (Option.tagFileName != NULL)
	    error(WARNING, "%s ignores output tag file name", notice);
    }
#ifdef UPDATE_ENABLED
    if (Option.update)
    {
	notice = "update option is not compatible with";
	if (Option.etags)
	    error(FATAL, "%s emacs-style tags", notice);
	if (Option.filter)
	    error(FATAL, "%s filter option", notice);
	if (isDestinationStdout())
	    error(FATAL, "%s tags to stdout", notice);
	Option.append = TRUE;
    }
#endif
}

extern void testEtagsInvocation()
{
    if (strncmp(getExecutableName(), ETAGS, strlen(ETAGS)) == 0)
    {
	StartedAsEtags = TRUE;

	Option.etags		= TRUE;
	Option.sorted		= FALSE;
	Option.lineDirectives	= FALSE;
    }
}

/*----------------------------------------------------------------------------
 *  File extension and language mapping
 *--------------------------------------------------------------------------*/

static void addExtensionList( slist, elist, clear )
    stringList *const slist;
    const char *const elist;
    const boolean clear;
{
    char *const extensionList = (char *)eMalloc(strlen(elist) + 1);
    const char *extension;
    boolean first = TRUE;

    if (clear)
    {
	if (Option.verbose)
	    printf("      clearing\n");
	stringListClear(slist);
    }
    strcpy(extensionList, elist);
    extension = strtok(extensionList, EXTENSION_SEPARATORS);
    if (Option.verbose)
	printf("      adding: ");
    while (extension != NULL)
    {
	if (Option.verbose)
	    printf("%s%s", first ? "" : ", ", extension);
	stringListAdd(slist, vStringNewInit(extension));
	extension = strtok(NULL, EXTENSION_SEPARATORS);
	first = FALSE;
    }
    if (Option.verbose)
    {
	putchar('\n');
	printf("      now: ");
	stringListPrint(slist);
	putchar ('\n');
    }
    eFree(extensionList);
}

static const char *findExtension( fileName )
    const char *const fileName;
{
    const char *extension;
    const char *pDelimiter = NULL;
#ifdef QDOS
    pDelimiter = strrchr(fileName, '_');
#endif
    if (pDelimiter == NULL)
        pDelimiter = strrchr(fileName, '.');

    if (pDelimiter == NULL)
	extension = "";
    else
	extension = pDelimiter + 1;	/* skip to first char of extension */

    return extension;
}

/*  Determines whether the specified file name is considered to be a header
 *  file for the purposes of determining whether enclosed tags are global or
 *  static.
 */
extern boolean isFileHeader( fileName )
    const char *const fileName;
{
    boolean result = FALSE;
    const char *const extension = findExtension(fileName);
    if (Option.headerExt != NULL)
    {
#ifdef CASE_INSENSITIVE_OS
	result = stringListHasInsensitive(Option.headerExt, extension);
#else
	result = stringListHas(Option.headerExt, extension);
#endif
    }
    return result;
}

static langType getExtensionLanguage( extension )
    const char *const extension;
{
    langType language = LANG_IGNORE;
    unsigned int i;
    for (i = 0  ;  i < (int)LANG_COUNT  ;  ++i)
    {
	if (Option.langMap[i] != NULL)
	{
#ifdef CASE_INSENSITIVE_OS
	    if (stringListHasInsensitive(Option.langMap[i], extension))
#else
	    if (stringListHas(Option.langMap[i], extension))
#endif
	    {
		language = (langType)i;
		break;
	    }
	}
    }
    return language;
}

extern langType getFileLanguage( fileName )
    const char *const fileName;
{
    const char *const extension = findExtension(fileName);
    langType language;

    if (Option.language != LANG_AUTO)
	language = Option.language;
    else if (extension[0] == '\0')
	language = LANG_IGNORE;		/* ignore files with no extension */
    else
	language = getExtensionLanguage(extension);

    return language;
}

extern const char *getLanguageName( language )
    const langType language;
{
    static const char *const names[] = {
	"auto", "C", "C++", "Eiffel", "Fortran", "Java"
    };

    Assert(sizeof(names)/sizeof(names[0]) == LANG_COUNT);
    return names[(int)language];
}

static langType getLangType( name )
    const char *const name;
{
    unsigned int i;
    langType language = LANG_IGNORE;

    for (i = 0  ;  i < LANG_COUNT  ;  ++i)
    {
	if (strequiv(name, getLanguageName((langType)i)))
	{
	    language = (langType)i;
	    break;
	}
    }
    return language;
}

static void processLangOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    const langType language = getLangType(parameter);

    if (language == LANG_IGNORE)
	error(FATAL, "Uknown language specified in \"%s\" option", option);
    else
	Option.language = language;
}

static void freeLanguageMaps()
{
    unsigned int i;
    for (i = 0  ;  i < (int)LANG_COUNT  ;  ++i)
	freeList(&Option.langMap[i]);
}

static void installLangMapDefault( language )
    const langType language;
{
    const int i = (int)language;
    freeList(&Option.langMap[i]);
    if (DefaultLanguageMap[i] != NULL)
    {
	Option.langMap[i] = stringListNewFromArgv(DefaultLanguageMap[i]);
	if (Option.verbose)
	{
	    printf("    Setting default %s map: ", getLanguageName(language));
	    stringListPrint(Option.langMap[i]);
	    putchar ('\n');
	}
    }
}

static void installLangMapDefaults()
{
    unsigned int i;
    for (i = 0  ;  i < (int)LANG_COUNT  ;  ++i)
	installLangMapDefault((langType)i);
}

static boolean installLangMap( map )
    char *const map;
{
    char *const separator = strchr(map, ':');
    boolean ok = TRUE;

    if (separator != NULL)
    {
	langType language;
	const char *list = separator + 1;
	boolean clear = TRUE;

	*separator = '\0';
	if (list[0] == '+')
	{
	    clear = FALSE;
	    ++list;
	}
	language = getLangType(map);
	if (language == LANG_IGNORE)
	    ok = FALSE;
	else if (language == LANG_AUTO)
	    ok = FALSE;
	else if (strcmp(list, "default") == 0)
	    installLangMapDefault(language);
	else
	{
	    if (Option.verbose)
		printf("    %s language map:\n", getLanguageName(language));
	    if (Option.langMap[(int)language] == NULL)
		Option.langMap[(int)language] = stringListNew();
	    addExtensionList(Option.langMap[(int)language], list, clear);
	}
    }
    return ok;
}

static void processLangMapOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    char *const maps = (char *)eMalloc(strlen(parameter) + 1);
    char *map = maps;

    strcpy(maps, parameter);

    if (Option.verbose)
	printf("    Language-extension maps:\n");
    if (strcmp(parameter, "default") == 0)
    {
	if (Option.verbose)
	    printf("    restoring defaults\n");
	installLangMapDefaults();
    }
    else while (map != NULL)
    {
	char *end = strchr(map, ',');

	if (end != NULL)
	    *end = '\0';
	if (! installLangMap(map))
	    error(FATAL, "Unknown language specified in \"%s\" option", option);
	if (end != NULL)
	    map = end + 1;
	else
	    map = NULL;
    }
    eFree(maps);
}

static void installHeaderListDefaults()
{
    Option.headerExt = stringListNewFromArgv(HeaderExtensions);
    if (Option.verbose)
    {
	printf("    Setting default header extensions: ");
	stringListPrint(Option.headerExt);
	putchar ('\n');
    }
}

static void processHeaderListOption( option, parameter )
    const int option;
    const char* parameter;
{
    /*  Check to make sure that the user did not enter "ctags -h *.c"
     *  by testing to see if the list is a filename that exists.
     */
    if (doesFileExist(parameter))
	error(FATAL, "-%c: Invalid list", option);
    if (strcmp(parameter, "default") == 0)
	installHeaderListDefaults();
    else
    {
	boolean clear = TRUE;

	if (parameter[0] == '+')
	{
	    ++parameter;
	    clear = FALSE;
	}
	if (Option.headerExt == NULL)
	    Option.headerExt = stringListNew();
	if (Option.verbose)
	    printf("    Header Extensions:\n");
	addExtensionList(Option.headerExt, parameter, clear);
    }
}

/*----------------------------------------------------------------------------
 *  Language tag type processing
 *--------------------------------------------------------------------------*/

static void processCTypesOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    const boolean longOption = (boolean)(strcmp(option, "i") != 0);
    struct sCInclude* const inc = &Option.include.cTypes;
    boolean defaultClear = longOption;
    const char* p = parameter;
    boolean mode = TRUE;
    int c;

    if (*p == '=')
    {
	defaultClear = TRUE;
	++p;
    }
    if (defaultClear  &&  strchr("+-", *p) == NULL)
    {
	inc->classNames		= FALSE;
	inc->defines		= FALSE;
	inc->enumerators	= FALSE;
	inc->functions		= FALSE;
	inc->enumNames		= FALSE;
	inc->members		= FALSE;
	inc->namespaceNames	= FALSE;
	inc->prototypes		= FALSE;
	inc->structNames	= FALSE;
	inc->typedefs		= FALSE;
	inc->unionNames		= FALSE;
	inc->variables		= FALSE;
	inc->access		= FALSE;
	inc->classPrefix	= FALSE;

	if (! longOption)
	{
	    Option.include.fileNames	= FALSE;
	    Option.include.fileScope	= FALSE;
	}
    }
    while ((c = *p++) != '\0') switch (c)
    {
	case '+': mode = TRUE;			break;
	case '-': mode = FALSE;			break;

	case 'c': inc->classNames	= mode;	break;
	case 'd': inc->defines		= mode;	break;
	case 'e': inc->enumerators	= mode;	break;
	case 'f': inc->functions	= mode;	break;
	case 'g': inc->enumNames	= mode;	break;
	case 'm': inc->members		= mode;	break;
	case 'n': inc->namespaceNames	= mode;	break;
	case 'p': inc->prototypes	= mode;	break;
	case 's': inc->structNames	= mode;	break;
	case 't': inc->typedefs		= mode;	break;
	case 'u': inc->unionNames	= mode;	break;
	case 'v': inc->variables	= mode;	break;
	case 'x': inc->externVars	= mode;	break;
	case 'A': inc->access		= mode;	break;
	case 'C': inc->classPrefix	= mode;	break;

	case 'F':
	    if (longOption)
		error(WARNING, "Unsupported parameter '%c' for --%s option",
		      c, option);
	    else
		Option.include.fileNames = mode;
	    break;

	case 'S':
	    if (longOption)
		error(WARNING, "Unsupported parameter '%c' for --%s option",
		      c, option);
	    else
		Option.include.fileScope = mode;
	    break;

	default:
	    error(WARNING, "Unsupported parameter '%c' for %s%s option",
		  c, longOption ? "--" : "-", option);
	    break;
    }
}

static void processEiffelTypesOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    struct sEiffelInclude *const inc = &Option.include.eiffelTypes;
    const char *p = parameter;
    boolean mode = TRUE;
    int c;

    if (strchr("+-", *p) == NULL)
    {
	inc->classNames		= FALSE;
	inc->features		= FALSE;
	inc->localEntities	= FALSE;
    }
    while ((c = *p++) != '\0') switch (c)
    {
	case '+': mode = TRUE;			break;
	case '-': mode = FALSE;			break;

	case 'c': inc->classNames	= mode;	break;
	case 'f': inc->features		= mode;	break;
	case 'l': inc->localEntities	= mode;	break;
	case 'C': inc->classPrefix	= mode;	break;

	default:
	    error(WARNING, "Unsupported parameter '%c' for \"%s\" option",
		    c, option);
	    break;
    }
}

static void processFortranTypesOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    struct sFortranInclude *const inc = &Option.include.fortranTypes;
    const char *p = parameter;
    boolean mode = TRUE;
    int c;

    if (strchr("+-", *p) == NULL)
    {
	inc->blockData		= FALSE;
	inc->commonBlocks	= FALSE;
	inc->entryPoints	= FALSE;
	inc->functions		= FALSE;
	inc->interfaces		= FALSE;
	inc->labels		= FALSE;
	inc->modules		= FALSE;
	inc->namelists		= FALSE;
	inc->programs		= FALSE;
	inc->subroutines	= FALSE;
	inc->types		= FALSE;
    }
    while ((c = *p++) != '\0') switch (c)
    {
	case '+': mode = TRUE;			break;
	case '-': mode = FALSE;			break;

	case 'b': inc->blockData	= mode;	break;
	case 'c': inc->commonBlocks	= mode;	break;
	case 'e': inc->entryPoints	= mode;	break;
	case 'f': inc->functions	= mode;	break;
	case 'i': inc->interfaces	= mode;	break;
	case 'l': inc->labels		= mode;	break;
	case 'm': inc->modules		= mode;	break;
	case 'n': inc->namelists	= mode;	break;
	case 'p': inc->programs		= mode;	break;
	case 's': inc->subroutines	= mode;	break;
	case 't': inc->types		= mode;	break;

	default:
	    error(WARNING, "Unsupported parameter '%c' for \"%s\" option",
		    c, option);
	    break;
    }
}

static void processJavaTypesOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    struct sJavaInclude *const inc = &Option.include.javaTypes;
    const char *p = parameter;
    boolean mode = TRUE;
    int c;

    if (strchr("+-", *p) == NULL)
    {
	inc->classNames		= FALSE;
	inc->fields		= FALSE;
	inc->interfaceNames	= FALSE;
	inc->methods		= FALSE;
	inc->packageNames	= FALSE;
	inc->access		= FALSE;
	inc->classPrefix	= FALSE;
    }
    while ((c = *p++) != '\0') switch (c)
    {
	case '+': mode = TRUE;			break;
	case '-': mode = FALSE;			break;

	case 'c': inc->classNames	= mode;	break;
	case 'f': inc->fields		= mode;	break;
	case 'i': inc->interfaceNames	= mode;	break;
	case 'm': inc->methods		= mode;	break;
	case 'p': inc->packageNames	= mode;	break;
	case 'A': inc->access		= mode;	break;
	case 'C': inc->classPrefix	= mode;	break;

	default:
	    error(WARNING, "Unsupported parameter '%c' for \"%s\" option",
		  c, option);
	    break;
    }
}

/*----------------------------------------------------------------------------
 *  Token ignore processing
 *--------------------------------------------------------------------------*/

/*  Determines whether or not "name" should be ignored, per the ignore list.
 */
extern boolean isIgnoreToken( name, pIgnoreParens, replacement )
    const char *const name;
    boolean *const pIgnoreParens;
    const char **const replacement;
{
    boolean result = FALSE;

    if (Option.ignore != NULL)
    {
	const size_t nameLen = strlen(name);
	unsigned int i;

	if (pIgnoreParens != NULL)
	    *pIgnoreParens = FALSE;

	for (i = 0  ;  i < stringListCount(Option.ignore)  ;  ++i)
	{
	    vString *token = stringListItem(Option.ignore, i);

	    if (strncmp(vStringValue(token), name, nameLen) == 0)
	    {
		const size_t tokenLen = vStringLength(token);

		if (nameLen == tokenLen)
		{
		    result = TRUE;
		    break;
		}
		else if (tokenLen == nameLen + 1  &&
			vStringChar(token, tokenLen - 1) == '+')
		{
		    result = TRUE;
		    if (pIgnoreParens != NULL)
			*pIgnoreParens = TRUE;
		    break;
		}
		else if (vStringChar(token, nameLen) == '=')
		{
		    if (replacement != NULL)
			*replacement = vStringValue(token) + nameLen + 1;
		    break;
		}
	    }
	}
    }
    return result;
}

static void saveIgnoreToken( ignoreToken )
    vString *const ignoreToken;
{
    if (Option.ignore == NULL)
	Option.ignore = stringListNew();
    stringListAdd(Option.ignore, ignoreToken);
    if (Option.verbose)
	printf("    ignore token: %s\n", vStringValue(ignoreToken));
}

static void readIgnoreList( list )
    const char *const list;
{
    char* newList = stringCopy(list);
    const char *token = strtok(newList, IGNORE_SEPARATORS);

    while (token != NULL)
    {
	vString *const entry = vStringNewInit(token);

	saveIgnoreToken(entry);
	token = strtok(NULL, IGNORE_SEPARATORS);
    }
    eFree(newList);
}

static void readIgnoreListFromFile( fileName )
    const char *const fileName;
{
    FILE *const fp = fopen(fileName, "r");

    if (fp == NULL)
	error(FATAL | PERROR, "cannot open \"%s\"", fileName);
    else
    {
	vString *entry = NULL;
	int c;

	do
	{
	    c = fgetc(fp);

	    if (isspace(c)  ||  c == EOF)
	    {
		if (entry != NULL  &&  vStringLength(entry) > 0)
		{
		    vStringTerminate(entry);
		    saveIgnoreToken(entry);
		    entry = NULL;
		}
	    }
	    else
	    {
		if (entry == NULL)
		    entry = vStringNew();
		vStringPut(entry, c);
	    }
	} while (c != EOF);
	Assert(entry == NULL);
    }
}

static void processIgnoreOption( list )
    const char* const list;
{
    if (strchr("./\\", list[0]) != NULL)
	readIgnoreListFromFile(list);
    else if (strcmp(list, "-") == 0)
    {
	freeList(&Option.ignore);
	if (Option.verbose)
	    printf("    clearing list\n");
    }
    else
	readIgnoreList(list);
}

/*----------------------------------------------------------------------------
 *  Specific option processing
 *--------------------------------------------------------------------------*/

static void printfFeatureList( where )
    FILE *const where;
{
    int i;

    for (i = 0 ; Features[i] != NULL ; ++i)
    {
	if (i == 0)
	    fprintf(where, "Optional features: ");
	fprintf(where, "%s+%s", (i>0 ? ", " : ""), Features[i]);
#ifdef CUSTOM_CONFIGURATION_FILE
	if (strcmp(Features[i], "custom-conf") == 0)
	    fprintf(where, "=%s", CUSTOM_CONFIGURATION_FILE);
#endif
    }
    if (i > 0)
	fputc('\n', where);
}

static void printProgramIdentification( where )
    FILE *const where;
{
    fprintf(where, "%s %s, by %s <%s>\n",
	    PROGRAM_NAME, PROGRAM_VERSION, AUTHOR_NAME, AUTHOR_EMAIL);
    printfFeatureList(where);
}

static void printInvocationDescription( where )
    FILE *const where;
{
    fprintf(where, INVOCATION, getExecutableName());
}

static void printOptionDescriptions( optDesc, where )
    const optionDescription *const optDesc;
    FILE *const where;
{
    int i;

    for (i = 0 ; optDesc[i].description != NULL ; ++i)
    {
	if (! StartedAsEtags || optDesc[i].usedByEtags)
	{
	    fputs(optDesc[i].description, where);
	    fputc('\n', where);
	}
    }
}

static void printHelp( optDesc )
    const optionDescription *const optDesc;
{

    printProgramIdentification(stdout);
    putchar('\n');
    printInvocationDescription(stdout);
    putchar('\n');
    printOptionDescriptions(optDesc, stdout);
}

static void processExcmdOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    switch (*parameter)
    {
	case 'm':	Option.locate = EX_MIX;		break;
	case 'n':	Option.locate = EX_LINENUM;	break;
	case 'p':	Option.locate = EX_PATTERN;	break;
	default:
	    error(FATAL, "Invalid value for \"%s\" option", option);
	    break;
    }
}

static void processFormatOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    unsigned int format;

    if (sscanf(parameter, "%u", &format) < 1)
	error(FATAL, "Invalid value for \"%s\" option",option);
    else if (format <= (unsigned int)MaxSupportedTagFormat)
	Option.tagFileFormat = format;
    else
	error(FATAL, "Unsupported value for \"%s\" option", option);
}

static void processEtagsInclude( option, parameter )
    const char *const __unused__ option;
    const char *const parameter;
{
    vString *const file = vStringNewInit(parameter);
    if (Option.etagsInclude == NULL)
	Option.etagsInclude = stringListNew();
    stringListAdd(Option.etagsInclude, file);
}

static void processFilterTerminatorOption( option, parameter )
    const char *const __unused__ option;
    const char *const parameter;
{
    freeString(&Option.filterTerminator);
    Option.filterTerminator = stringCopy(parameter);
}

/*----------------------------------------------------------------------------
 *  Option tables
 *--------------------------------------------------------------------------*/

static parametricOption ParametricOptions[] = {
    { "c-types",		processCTypesOption,		FALSE	},
    { "c++-types",		processCTypesOption,		FALSE	},
    { "eiffel-types",		processEiffelTypesOption,	FALSE	},
    { "etags-include",		processEtagsInclude,		FALSE	},
    { "excmd",			processExcmdOption,		FALSE	},
    { "filter-terminator",	processFilterTerminatorOption,	TRUE	},
    { "format",			processFormatOption,		TRUE	},
    { "fortran-types",		processFortranTypesOption,	FALSE	},
    { "java-types",		processJavaTypesOption,		FALSE	},
    { "lang",			processLangOption,		FALSE	},
    { "language",		processLangOption,		FALSE	},
    { "langmap",		processLangMapOption,		FALSE	}
};

static booleanOption BooleanOptions[] = {
    { "append",		&Option.append,			TRUE,	TRUE	},
    { "file-scope",	&Option.include.fileScope,	TRUE,	FALSE	},
    { "file-tags",	&Option.include.fileNames,	TRUE,	FALSE	},
    { "if0",		&Option.if0,			TRUE,	FALSE	},
    { "kind-long",	&Option.kindLong,		TRUE,	TRUE	},
    { "line-directives",&Option.lineDirectives,		TRUE,	FALSE	},
    { "links",		&Option.followLinks,		TRUE,	FALSE	},
    { "filter",		&Option.filter,			TRUE,	TRUE	},
#ifdef RECURSE_SUPPORTED
    { "recurse",	&Option.recurse,		TRUE,	FALSE	},
#endif
    { "sort",		&Option.sorted,			TRUE,	TRUE	},
    { "totals",		&Option.printTotals,		TRUE,	TRUE	},
    { "verbose",	&Option.verbose,		TRUE,	FALSE	},
};

/*----------------------------------------------------------------------------
 *  Generic option parsing
 *--------------------------------------------------------------------------*/

static void checkOptionOrder( option )
    const char* const option;
{
    if (ReachedFileNames)
	error(FATAL, "-%s option may not follow a file name", option);
}

static boolean processParametricOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    const int count = sizeof(ParametricOptions)/sizeof(parametricOption);
    boolean found = FALSE;
    int i;

    for (i = 0  ;  i < count  &&  ! found  ;  ++i)
    {
	parametricOption* const entry = &ParametricOptions[i];
	if (strcmp(option, entry->name) == 0)
	{
	    found = TRUE;
	    if (entry->initOnly)
		checkOptionOrder(option);
	    (entry->handler)(option, parameter);
	}
    }
    return found;
}

static boolean getBooleanOption( optionName, parameter )
    const char *const optionName;
    const char *const parameter;
{
    boolean selection = TRUE;

    if (parameter[0] == '\0')
	selection = TRUE;
    else if (strcmp(parameter, "0"  ) == 0  ||
	     strcmp(parameter, "no" ) == 0  ||
	     strcmp(parameter, "off") == 0)
	selection = FALSE;
    else if (strcmp(parameter, "1"  ) == 0  ||
	     strcmp(parameter, "yes") == 0  ||
	     strcmp(parameter, "on" ) == 0)
	selection = TRUE;
    else
	error(FATAL, "Invalid value for \"%s\" option", optionName);

    return selection;
}

static boolean processBooleanOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    const int count = sizeof(BooleanOptions) / sizeof(booleanOption);
    boolean found = FALSE;
    int i;

    for (i = 0  ;  i < count  &&  ! found  ;  ++i)
    {
	booleanOption* const entry = &BooleanOptions[i];
	if (strcmp(option, entry->name) == 0)
	{
	    found = TRUE;
	    if (entry->initOnly)
		checkOptionOrder(option);
	    *entry->pValue = getBooleanOption(option, parameter);
	}
    }
    return found;
}

static void processLongOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    if (Option.verbose)
    {
	if (parameter == NULL  ||  *parameter == '\0')
	    printf("  Option: --%s\n", option);
	else
	    printf("  Option: --%s=%s\n", option, parameter);
    }
    if (processBooleanOption(option, parameter))
	;
    else if (processParametricOption(option, parameter))
	;
#define isOption(item)	(boolean)(strcmp(item,option) == 0)
    else if (isOption("help"))
	{ printHelp(LongOptionDescription); exit(0); }
    else if (isOption("version"))
    {
	printProgramIdentification(stdout);
	exit(0);
    }
#ifndef RECURSE_SUPPORTED
    else if (isOption("recurse"))
	error(WARNING, "%s option not supported on this host", option);
#endif
    else
	error(FATAL, "Unknown option: --%s", option);
#undef isOption
}

static void processShortOption( option, parameter )
    const char *const option;
    const char *const parameter;
{
    if (Option.verbose)
    {
	if (parameter == NULL  ||  *parameter == '\0')
	    printf("  Option: -%s\n", option);
	else
	    printf("  Option: -%s %s\n", option, parameter);
    }
    if (isCompoundOption(*option) && *parameter == '\0')
	error(FATAL, "Missing parameter for \"%s\" option", option);
    else switch (*option)
    {
	case '?':
	    printHelp(LongOptionDescription);
	    exit(0);
	    break;
	case 'a':
	    checkOptionOrder(option);
	    Option.append = TRUE;
	    break;
#ifdef DEBUG
	case 'b':
	    if (atol(parameter) < 0)
		error(FATAL, "-%c: Invalid line number", option);
	    Option.breakLine = atol(parameter);
	    break;
	case 'D':
	    Option.debugLevel = atoi(parameter);
	    if (debug(DEBUG_STATUS))
		Option.verbose = TRUE;
	    break;
#endif
	case 'B':
	    Option.backward = TRUE;
	    break;
	case 'e':
	    checkOptionOrder(option);
	    Option.etags = TRUE;
	    Option.sorted = FALSE;
	    break;
	case 'f':
	case 'o':
	    checkOptionOrder(option);
	    if (Option.tagFileName != NULL)
	    {
		error(WARNING,
		    "-%c option specified more than once, last value used",
		    option);
		freeString(&Option.tagFileName);
	    }
	    Option.tagFileName = stringCopy(parameter);
	    break;
	case 'F':
	    Option.backward = FALSE;
	    break;
	case 'h':
	    processHeaderListOption(*option, parameter);
	    break;
	case 'i':
	    processCTypesOption("i", parameter);
	    break;
	case 'I':
	    processIgnoreOption(parameter);
	    break;
	case 'L':
	    if (Option.fileList != NULL)
	    {
		error(WARNING,
		    "-%c option specified more than once, last value used",
		    option);
		freeString(&Option.fileList);
	    }
	    Option.fileList = stringCopy(parameter);
	    break;
	case 'n':
	    Option.locate = EX_LINENUM;
	    break;
	case 'N':
	    Option.locate = EX_PATTERN;
	    break;
	case 'p':
	    if (Option.path != NULL)
		freeString(&Option.path);
	    Option.path = stringCopy(parameter);
	    break;
	case 'R':
#ifdef RECURSE_SUPPORTED
	    Option.recurse = TRUE;
#else
	    error(WARNING, "-%c option not supported on this host", option);
#endif
	    break;
	case 'u':
	    checkOptionOrder(option);
	    Option.sorted = FALSE;
	    break;
	case 'V':
	    Option.verbose = TRUE;
	    break;
	case 'x':
	    checkOptionOrder(option);
	    Option.xref = TRUE;
	    break;
	default:
	    error(FATAL, "Unknown option: -%s", option);
	    break;
    }
}

extern void parseOption( args )
    cookedArgs* const args;
{
    Assert(! cArgOff(args));
    if (args->isOption)
    {
	if (args->longOption)
	    processLongOption(args->item, args->parameter);
	else
	    processShortOption(args->item, args->parameter);
	cArgForth(args);
    }
}

extern void parseOptions( args )
    cookedArgs* const args;
{
    while (! cArgOff(args)  &&  cArgIsOption(args))
	parseOption(args);
}

static void parseFileOptions( fileName )
    const char* const fileName;
{
    FILE* const fp = fopen(fileName, "r");
    if (fp != NULL)
    {
	cookedArgs* const args = cArgNewFromFile(fp);
	if (Option.verbose)
	    printf("Reading options from %s\n", fileName);
	parseOptions(args);
	cArgDelete(args);
	fclose(fp);
    }
}

extern void previewFirstOption( args )
    cookedArgs* const args;
{
    if (cArgIsOption(args) &&
	(strcmp(args->item, "V") == 0 || strcmp(args->item, "verbose") == 0))
    {
	parseOption(args);
    }
}

static void parseConfigurationFileOptions()
{
    const char* const home = getenv("HOME");
    parseFileOptions("/etc/ctags.conf");
    parseFileOptions("/usr/local/etc/ctags.conf");
#ifdef CUSTOM_CONFIGURATION_FILE
    parseFileOptions(CUSTOM_CONFIGURATION_FILE);
#endif
    if (home != NULL)
    {
	vString* const dotFile = combinePathAndFile(home, ".ctags");
	parseFileOptions(vStringValue(dotFile));
	vStringDelete(dotFile);
    }
    parseFileOptions(".ctags");
}

static void parseEnvironmentOptions()
{
    const char *envOptions = NULL;

    if (StartedAsEtags)
	envOptions = getenv(ETAGS_ENVIRONMENT);
    if (envOptions == NULL)
	envOptions = getenv(CTAGS_ENVIRONMENT);
    if (envOptions != NULL  &&  envOptions[0] != '\0')
    {
	cookedArgs* const args = cArgNewFromString(envOptions);
	if (Option.verbose)
	    printf("Reading options from $CTAGS\n");
	parseOptions(args);
	cArgDelete(args);
    }
}

extern void readOptionConfiguration()
{
    parseConfigurationFileOptions();
    parseEnvironmentOptions();
}

/*----------------------------------------------------------------------------
 *  Cooked argument parsing
 *--------------------------------------------------------------------------*/

static void parseShortOption( args )
    cookedArgs* const args;
{
    args->simple[0] = *args->shortOptions++;
    args->simple[1] = '\0';
    args->item = args->simple;
    if (! isCompoundOption(*args->simple))
	args->parameter = "";
    else if (*args->shortOptions == '\0')
    {
	argForth(args->args);
	if (argOff(args->args))
	    args->parameter = NULL;
	else
	    args->parameter = argItem(args->args);
	args->shortOptions = NULL;
    }
    else
    {
	args->parameter = args->shortOptions;
	args->shortOptions = NULL;
    }
}

static void parseLongOption( args, item )
    cookedArgs* const args;
    const char* item;
{
    const char* const equal = strchr(item, '=');
    if (equal == NULL)
    {
	args->item = (char*)eMalloc(strlen(item) + 1);
	strcpy(args->item, item);
	args->parameter = "";
    }
    else
    {
	const size_t length = equal - item;
	args->item = (char*)eMalloc(length + 1);
	strncpy(args->item, item, length);
	args->item[length] = '\0';
	args->parameter = equal + 1;
    }
}

static void cArgRead( current )
    cookedArgs* const current;
{
    char* item;

    Assert(current != NULL);
    if (! argOff(current->args))
    {
	item = argItem(current->args);
	Assert(item != NULL);
	if (strncmp(item, "--", (size_t)2) == 0)
	{
	    current->isOption = TRUE;
	    current->longOption = TRUE;
	    parseLongOption(current, item + 2);
	}
	else if (*item == '-')
	{
	    current->isOption = TRUE;
	    current->longOption = FALSE;
	    current->shortOptions = item + 1;
	    parseShortOption(current);
	}
	else
	{
	    current->isOption = FALSE;
	    current->longOption = FALSE;
	    current->item = item;
	    current->parameter = "";
	}
    }
}

extern cookedArgs* cArgNewFromString( string )
    const char* string;
{
    cookedArgs* const result = (cookedArgs*)eMalloc(sizeof(cookedArgs));
    memset(result, 0, sizeof(cookedArgs));
    result->args = argNewFromString(string);
    cArgRead(result);
    return result;
}

extern cookedArgs* cArgNewFromArgv( argv )
    char* const* const argv;
{
    cookedArgs* const result = (cookedArgs*)eMalloc(sizeof(cookedArgs));
    memset(result, 0, sizeof(cookedArgs));
    result->args = argNewFromArgv(argv);
    cArgRead(result);
    return result;
}

extern cookedArgs* cArgNewFromFile( fp )
    FILE* const fp;
{
    cookedArgs* const result = (cookedArgs*)eMalloc(sizeof(cookedArgs));
    memset(result, 0, sizeof(cookedArgs));
    result->args = argNewFromFile(fp);
    cArgRead(result);
    return result;
}

extern cookedArgs* cArgNewFromLineFile( fp )
    FILE* const fp;
{
    cookedArgs* const result = (cookedArgs*)eMalloc(sizeof(cookedArgs));
    memset(result, 0, sizeof(cookedArgs));
    result->args = argNewFromLineFile(fp);
    cArgRead(result);
    return result;
}

extern void cArgDelete( current )
    cookedArgs* const current;
{
    Assert(current != NULL);
    argDelete(current->args);
    memset(current, 0, sizeof(cookedArgs));
    eFree(current);
}

extern boolean cArgOff( current )
    cookedArgs* const current;
{
    Assert(current != NULL);
    return (boolean)(argOff(current->args) && ! shortOptionPending(current));
}

extern boolean cArgIsOption( current )
    cookedArgs* const current;
{
    Assert(current != NULL);
    return current->isOption;
}

extern const char* cArgItem( current )
    cookedArgs* const current;
{
    Assert(current != NULL);
    return current->item;
}

extern void cArgForth( current )
    cookedArgs* const current;
{
    Assert(current != NULL);
    Assert(! cArgOff(current));
    if (shortOptionPending(current))
	parseShortOption(current);
    else
    {
	Assert(! argOff(current->args));
	argForth(current->args);
	if (! argOff(current->args))
	    cArgRead(current);
	else
	{
	    current->isOption = FALSE;
	    current->longOption = FALSE;
	    current->item = NULL;
	    current->parameter = NULL;
	}
    }
}

/*----------------------------------------------------------------------------
*-	Option initialization
----------------------------------------------------------------------------*/

extern void initOptions()
{
    if (Option.verbose)
	printf("Installing option defaults\n");
    installLangMapDefaults();
    installHeaderListDefaults();
}

extern void freeOptionResources()
{
    freeString(&Option.tagFileName);
    freeString(&Option.fileList);
    freeString(&Option.path);
    freeString(&Option.filterTerminator);

    freeList(&Option.ignore);
    freeList(&Option.headerExt);
    freeList(&Option.etagsInclude);

    freeLanguageMaps();
}

/* vi:set tabstop=8 shiftwidth=4: */
