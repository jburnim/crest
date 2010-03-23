/*****************************************************************************
*   $Id: main.c,v 8.30 2000/04/23 17:23:00 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   Author: Darren Hiebert <darren@hiebert.com>, <darren@hiwaay.net>
*           http://darren.hiebert.com
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License. It is provided on an as-is basis and no
*   responsibility is accepted for its failure to perform as expected.
*
*   This is a reimplementation of the ctags(1) program. It is an attempt to
*   provide a fully featured ctags program which is free of the limitations
*   which most (all?) others are subject to.
*
*   This module contains top level start-up and portability functions.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>		/* to declare malloc(), realloc() */
#endif
#include <string.h>
#include <errno.h>

#include <stdio.h>		/* to declare SEEK_SET (hopefully) */
#if defined(HAVE_UNISTD_H)
# include <unistd.h>	/* to declare mkstemp(), and SEEK_SET on SunOS 4.1.x */
#endif

#ifdef AMIGA
# include <dos/dosasl.h>	/* for struct AnchorPath */
# include <clib/dos_protos.h>	/* function prototypes */
# define ANCHOR_BUF_SIZE (512)
# define ANCHOR_SIZE (sizeof(struct AnchorPath) + ANCHOR_BUF_SIZE)
#endif

#ifdef ENABLE_STDARG
# include <stdarg.h>
#else
# include <varargs.h>
#endif

/*  To declare "struct stat" and stat().
 */
#if defined(__MWERKS__) && defined(__MACINTOSH__)
# include <stat.h>		/* there is no sys directory on the Mac */
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif

/*  To provide directory searching for recursion feature.
 */
#ifdef HAVE_DIRENT_H
# ifdef __BORLANDC__
#  define boolean BORLAND_boolean
# endif
# include <dirent.h>
# undef boolean
#endif
#ifdef HAVE_DOS_H
# include <dos.h>	/* to declare FA_DIREC */
#endif
#ifdef HAVE_DIR_H
# include <dir.h>	/* to declare findfirst() and findnext() */
#endif
#ifdef HAVE_IO_H
# include <io.h>	/* to declare _finddata_t in MSVC++ 4.x */
#endif

/*  To provide timings features if available.
 */
#ifdef HAVE_CLOCK
# ifdef HAVE_TIME_H
#  include <time.h>
# endif
#else
# ifdef HAVE_TIMES
#  ifdef HAVE_SYS_TIMES_H
#   include <sys/times.h>
#  endif
# endif
#endif

#include "ctags.h"
#include "debug.h"
#include "entry.h"
#include "keyword.h"
#include "main.h"
#include "options.h"
#include "parse.h"
#include "read.h"
#include "vstring.h"

#ifdef TRAP_MEMORY_CALLS
# include "safe_malloc.h"
#endif

/*============================================================================
=   Defines
============================================================================*/

/*----------------------------------------------------------------------------
 *  Miscellaneous defines
 *--------------------------------------------------------------------------*/
#define selected(var,feature)	(((int)(var) & (int)(feature)) == (int)feature)

#define plural(value)		(((unsigned long)(value) == 1L) ? "" : "s")

/*----------------------------------------------------------------------------
 *  Portability defines
 *--------------------------------------------------------------------------*/
#if defined(MSDOS) || defined(WIN32) || defined(OS2)
# define PATH_SEPARATOR '\\'
#else
# ifdef QDOS
#  define PATH_SEPARATOR '_'
# else
#  define PATH_SEPARATOR '/'
# endif
#endif

/*  File type tests.
 */
#ifndef S_ISREG
# if defined(S_IFREG) && !defined(AMIGA)
#  define S_ISREG(mode)	    ((mode) & S_IFREG)
# else
#  define S_ISREG(mode)	    TRUE	/* assume regular file */
# endif
#endif

#ifndef S_ISLNK
# ifdef S_IFLNK
#  define S_ISLNK(mode)	    (((mode) & S_IFMT) == S_IFLNK)
# else
#  define S_ISLNK(mode)	    FALSE	/* assume no soft links */
# endif
#endif

#ifndef S_ISDIR
# ifdef S_IFDIR
#  define S_ISDIR(mode)	    (((mode) & S_IFMT) == S_IFDIR)
# else
#  define S_ISDIR(mode)	    FALSE	/* assume no soft links */
# endif
#endif

/*  Hack for rediculous practice of Microsoft Visual C++.
 */
#if defined(WIN32) && defined(_MSC_VER)
# define stat	_stat
#endif

/*============================================================================
=   Data definitions
============================================================================*/
#if defined(MSDOS) || defined(WIN32) || defined(OS2)
static const char PathDelimiters[] = ":/\\";
#endif
#ifdef __vms
static const char PathDelimiters[] = ":]>";
#endif

#ifndef TMPDIR
# define TMPDIR "/tmp"
#endif

static const char *ExecutableProgram = NULL;
static const char *ExecutableName = NULL;

static struct { long files, lines, bytes; } Totals = { 0, 0, 0 };

/*============================================================================
=   Function prototypes
============================================================================*/
#ifdef NEED_PROTO_STAT
extern int stat __ARGS((const char *, struct stat *));
#endif
#ifdef NEED_PROTO_LSTAT
extern int lstat __ARGS((const char *, struct stat *));
#endif

# if defined(MSDOS) || defined(WIN32) || defined(AMIGA)
static boolean createTagsForMatchingEntries __ARGS((char *const pattern));
#endif

static boolean isSymbolicLink __ARGS((const char *const name));
static boolean isNormalFile __ARGS((const char *const name));
static boolean isDirectory __ARGS((const char *const name));
static boolean createTagsForFile __ARGS((const char *const filePath, const langType language, const unsigned int passCount));
static vString *sourceFilePath __ARGS((const char *const file));
static boolean createTagsWithFallback __ARGS((const char *const fileName, const langType language));
static boolean createTagsForDirectory __ARGS((const char *const dirName));
static boolean createTagsForEntry __ARGS((const char *const entryName));
static boolean createTagsFromFileInput __ARGS((FILE* const fp, const boolean filter));
static boolean createTagsFromListFile __ARGS((const char* const fileName));
static boolean createTagsForArgs __ARGS((cookedArgs* const args));
static void printTotals __ARGS((const clock_t *const timeStamps));
static void makeTags __ARGS((cookedArgs* arg));
static void setExecutableName __ARGS((const char *const path));

/*============================================================================
=   Function definitions
============================================================================*/

#ifdef ENABLE_STDARG
extern void error( const errorSelection selection,
		   const char *const format, ... )
#else
extern void error( va_alist )
    va_dcl
#endif
{
    va_list ap;

#ifdef ENABLE_STDARG
    va_start(ap, format);
#else
    const char *format;
    errorSelection selection;

    va_start(ap);
    selection = va_arg(ap, errorSelection);
    format = va_arg(ap, char *);
#endif
    fprintf(errout, "%s: %s", getExecutableName(),
	    selected(selection, WARNING) ? "warning: " : "");
    vfprintf(errout, format, ap);
    if (selected(selection, PERROR))
#ifdef HAVE_STRERROR
	fprintf(errout, " : %s", strerror(errno));
#else
	perror(" ");
#endif
    fputs("\n", errout);
    va_end(ap);
    if (selected(selection, FATAL))
	exit(1);
}

extern boolean strequiv( s1, s2 )
    const char *const s1;
    const char *const s2;
{
    boolean equivalent = TRUE;
    const char *p1 = s1;
    const char *p2 = s2;

    do
    {
	if (toupper((int)*p1) != toupper((int)*p2))
	{
	    equivalent = FALSE;
	    break;
	}
    } while (*p1++ != '\0'  &&  *p2++ != '\0');

    return equivalent;
}

extern void *eMalloc( size )
    const size_t size;
{
    void *buffer = malloc(size);

    if (buffer == NULL)
	error(FATAL, "out of memory");

    return buffer;
}

extern void *eRealloc( ptr, size )
    void *const ptr;
    const size_t size;
{
    void *buffer = realloc(ptr, size);

    if (buffer == NULL)
	error(FATAL, "out of memory");

    return buffer;
}

extern void eFree( ptr )
    void *const ptr;
{
    Assert(ptr != NULL);
    free(ptr);
}

extern unsigned long getFileSize( name )
    const char *const __unused__ name;
{
    struct stat fileStatus;
    unsigned long size = 0;

    if (stat(name, &fileStatus) == 0)
	size = fileStatus.st_size;

    return size;
}

static boolean isSymbolicLink( name )
    const char *const name;
{
#if defined(__vms) || defined(MSDOS) || defined(WIN32) || defined(__EMX__) || defined(AMIGA)
    return FALSE;
#else
    struct stat fileStatus;
    boolean isLink = FALSE;

    if (lstat(name, &fileStatus) == 0)
	isLink = (boolean)(S_ISLNK(fileStatus.st_mode));

    return isLink;
#endif
}

static boolean isNormalFile( name )
    const char *const name;
{
    struct stat fileStatus;
    boolean isNormal = FALSE;

    if (stat(name, &fileStatus) == 0)
	isNormal = (boolean)(S_ISREG(fileStatus.st_mode));

    return isNormal;
}

#ifdef HAVE_MKSTEMP

static boolean isSetUID __ARGS((const char *const name));

static boolean isSetUID( name )
    const char *const name;
{
#if defined(__vms) || defined(MSDOS) || defined(WIN32) || defined(__EMX__) || defined(AMIGA)
    return FALSE;
#else
    struct stat fileStatus;
    boolean result = FALSE;

    if (stat(name, &fileStatus) == 0)
	result = (boolean)((fileStatus.st_mode & S_ISUID) != 0);

    return result;
#endif
}

#endif

static boolean isDirectory( name )
    const char *const name;
{
    boolean isDir = FALSE;
#ifdef AMIGA
    struct FileInfoBlock *const fib = (struct FileInfoBlock *)
					eMalloc(sizeof(struct FileInfoBlock));

    if (fib != NULL)
    {
	const BPTR flock = Lock((UBYTE *)name, (long)ACCESS_READ);

	if (flock != (BPTR)NULL)
	{
	    if (Examine(flock, fib))
		isDir = ((fib->fib_DirEntryType >= 0) ? TRUE : FALSE);
	    UnLock(flock);
	}
	eFree(fib);
    }
#else
    struct stat fileStatus;

    if (stat(name, &fileStatus) == 0)
	isDir = (boolean)S_ISDIR(fileStatus.st_mode);
#endif
    return isDir;
}

extern boolean doesFileExist( fileName )
    const char *const fileName;
{
    struct stat fileStatus;

    return (boolean)(stat(fileName, &fileStatus) == 0);
}

#ifndef HAVE_FGETPOS

extern int fgetpos( stream, pos )
    FILE *const stream;
    fpos_t *const pos;
{
    int result = 0;

    *pos = ftell(stream);
    if (*pos == -1L)
	result = -1;

    return result;
}

extern int fsetpos( stream, pos )
    FILE *const stream;
    fpos_t *const pos;
{
    return fseek(stream, *pos, SEEK_SET);
}

#endif

extern void addTotals( files, lines, bytes )
    const unsigned int files;
    const unsigned long lines;
    const unsigned long bytes;
{
    Totals.files += files;
    Totals.lines += lines;
    Totals.bytes += bytes;
}

extern boolean isDestinationStdout()
{
    boolean toStdout = FALSE;

    if (Option.xref  ||  Option.filter  ||
	(Option.tagFileName != NULL  &&  (strcmp(Option.tagFileName, "-") == 0
#if defined(MSDOS) || defined(WIN32) || defined(OS2)
	/* nothing else special needed here */
#else
# ifdef __vms
    || strncasecmp(Option.tagFileName, "sys$output", sizeof("sys$output")) == 0
# else
    || strcmp(Option.tagFileName, "/dev/stdout") == 0
# endif
#endif
	)))
	toStdout = TRUE;

    return toStdout;
}

extern FILE *tempFile( mode, pName )
    const char *const mode;
    char **const pName;
{
    char *name;
    FILE *fp;
#ifdef HAVE_MKSTEMP
    int fd;
    const char *const template = "tags.XXXXXX";
    const char *tmpdir = NULL;
    if (! isSetUID(ExecutableProgram))
	tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
	tmpdir = TMPDIR;
    name = (char*)eMalloc(strlen(tmpdir) + 1 + strlen(template) + 1);
    sprintf(name, "%s%c%s", tmpdir, PATH_SEPARATOR, template);
    fd = mkstemp(name);
    if (fd == -1)
	error(FATAL | PERROR, "cannot open temporary file");
    fp = fdopen(fd, mode);
#else
    name = (char*)eMalloc((size_t)L_tmpnam);
    if (tmpnam(name) != name)
	error(FATAL | PERROR, "cannot assign temporary file name");
    fp = fopen(name, mode);
#endif
    if (fp == NULL)
	error(FATAL | PERROR, "cannot open temporary file");
    DebugStatement(
	debugPrintf(DEBUG_STATUS, "opened temporary file %s\n", name); )
    Assert(*pName == NULL);
    *pName = name;
    return fp;
}

/*----------------------------------------------------------------------------
 *  Pathname manipulation (O/S dependent!!!)
 *--------------------------------------------------------------------------*/

extern const char *baseFilename( filePath )
    const char *const filePath;
{
#if defined(MSDOS) || defined(WIN32) || defined(OS2) || defined(__vms)
    const char *tail = NULL;
    unsigned int i;

    /*  Find whichever of the path delimiters is last.
     */
    for (i = 0  ;  i < strlen(PathDelimiters)  ;  ++i)
    {
	const char *sep = strrchr(filePath, PathDelimiters[i]);

	if (sep > tail)
	    tail = sep;
    }
#else
    const char *tail = strrchr(filePath, PATH_SEPARATOR);
#endif
    if (tail == NULL)
	tail = filePath;
    else
	++tail;			/* step past last delimiter */

    return tail;
}

extern boolean isAbsolutePath( path )
    const char *const path;
{
#if defined(MSDOS) || defined(WIN32) || defined(OS2)
    return (strchr(PathDelimiters, path[0]) != NULL  ||
	    (isalpha(path[0])  &&  path[1] == ':'));
#else
# ifdef __vms
    return (boolean)(strchr(path, ':') != NULL);
# else
    return (boolean)(path[0] == PATH_SEPARATOR);
# endif
#endif
}

extern vString *combinePathAndFile( path, file )
    const char *const path;
    const char *const file;
{
    vString *const filePath = vStringNew();
#ifdef __vms
    const char *const directoryId = strstr(file, ".DIR;1");

    if (directoryId == NULL)
    {
	const char *const versionId = strchr(file, ';');

	vStringCopyS(filePath, path);
	if (versionId == NULL)
	    vStringCatS(filePath, file);
	else
	    vStringNCatS(filePath, file, versionId - file);
	vStringCopyToLower(filePath, filePath);
    }
    else
    {
	/*  File really is a directory; append it to the path.
	 *  Gotcha: doesn't work with logical names.
	 */
	vStringNCopyS(filePath, path, strlen(path) - 1);
	vStringPut(filePath, '.');
	vStringNCatS(filePath, file, directoryId - file);
	if (strchr(path, '[') != NULL)
	    vStringPut(filePath, ']');
	else
	    vStringPut(filePath, '>');
	vStringTerminate(filePath);
    }
#else
    const int lastChar = path[strlen(path) - 1];
# if defined(MSDOS) || defined(WIN32) || defined(OS2)
    boolean terminated = (boolean)(strchr(PathDelimiters, lastChar) != NULL);
# else
    boolean terminated = (boolean)(lastChar == PATH_SEPARATOR);
# endif

    vStringCopyS(filePath, path);
    if (! terminated)
    {
	vStringPut(filePath, PATH_SEPARATOR);
	vStringTerminate(filePath);
    }
    vStringCatS(filePath, file);
#endif

    return filePath;
}

/*----------------------------------------------------------------------------
 *	Create tags
 *--------------------------------------------------------------------------*/

static boolean createTagsForFile( filePath, language, passCount )
    const char *const filePath;
    const langType language;
    const unsigned int passCount;
{
    boolean retry = FALSE;

    if (Option.etags)
	beginEtagsFile();

    if (fileOpen(filePath, language))
    {
	if (Option.include.fileNames)
	{
	    tagEntryInfo tag;

	    initTagEntry(&tag, baseFilename(filePath));

	    tag.isFileEntry	= TRUE;
	    tag.lineNumberEntry = TRUE;
	    tag.lineNumber	= 1;
	    tag.kindName	= "file";
	    tag.kind		= 'F';

	    makeTagEntry(&tag);
	}

	if (language == LANG_EIFFEL)
	    retry = createEiffelTags();
	else if (language == LANG_FORTRAN)
	    retry = createFortranTags(passCount);
	else
	    retry = createCTags(passCount);
	fileClose();
    }

    if (Option.etags)
	endEtagsFile(filePath);

    return retry;
}

static vString *sourceFilePath( file )
    const char *const file;
{
    vString *filePath;

    if (Option.path == NULL  ||  isAbsolutePath(file))
    {
	filePath = vStringNew();
	vStringCopyS(filePath, file);
    }
    else
	filePath = combinePathAndFile(Option.path, file);

    return filePath;
}

static boolean createTagsWithFallback( fileName, language )
    const char *const fileName;
    const langType language;
{
    const unsigned long numTags	= TagFile.numTags.added;
    fpos_t tagFilePosition;
    boolean resize = FALSE;
    unsigned int passCount = 0;

    fgetpos(TagFile.fp, &tagFilePosition);
    while (createTagsForFile(fileName, language, ++passCount))
    {
	/*  Restore prior state of tag file.
	 */
	fsetpos(TagFile.fp, &tagFilePosition);
	TagFile.numTags.added = numTags;
	resize = TRUE;
    }
    return resize;
}

# if defined(MSDOS) || defined(WIN32)

static boolean createTagsForMatchingEntries( pattern )
    char *const pattern;
{
    boolean resize = FALSE;
    const size_t dirLength = baseFilename(pattern) - (char *)pattern;
    vString *const filePath = vStringNew();
#ifdef HAVE_FINDFIRST
    struct ffblk fileInfo;
    int result = findfirst(pattern, &fileInfo, FA_DIREC);

    while (result == 0)
    {
	const char *const entryName = fileInfo.ff_name;

	/*  We must not recurse into the directories "." or "..".
	 */
	if (strcmp(entryName, ".") != 0  &&  strcmp(entryName, "..") != 0)
	{
	    vStringNCopyS(filePath, pattern, dirLength);
	    vStringCatS(filePath, entryName);
	    resize |= createTagsForEntry(vStringValue(filePath));
	}
	result = findnext(&fileInfo);
    }
#else
# ifdef HAVE__FINDFIRST
    struct _finddata_t fileInfo;
    long hFile = _findfirst(pattern, &fileInfo);

    if (hFile != -1L)
    {
	do
	{
	    const char *const entryName = fileInfo.name;

	    /*  We must not recurse into the directories "." or "..".
	     */
	    if (strcmp(entryName, ".") != 0  &&  strcmp(entryName, "..") != 0)
	    {
		vStringNCopyS(filePath, pattern, dirLength);
		vStringCatS(filePath, entryName);
		resize |= createTagsForEntry(vStringValue(filePath));
	    }
	} while (_findnext(hFile, &fileInfo) == 0);
	_findclose(hFile);
    }
# endif	/* HAVE__FINDFIRST */
#endif	/* HAVE_FINDFIRST */

    vStringDelete(filePath);
    return resize;
}

#else
# ifdef AMIGA

static boolean createTagsForMatchingEntries( pattern )
    char *const pattern;
{
    boolean resize = FALSE;
    struct AnchorPath *const anchor =
			(struct AnchorPath *)eMalloc((size_t)ANCHOR_SIZE);

    if (anchor != NULL)
    {
	LONG result;

	memset(anchor, 0, (size_t)ANCHOR_SIZE);
	anchor->ap_Strlen = ANCHOR_BUF_SIZE; /* ap_Length no longer supported */

	/*  Allow '.' for current directory.
	 */
#ifdef APF_DODOT
	anchor->ap_Flags = APF_DODOT | APF_DOWILD;
#else
	anchor->ap_Flags = APF_DoDot | APF_DoWild;
#endif

	result = MatchFirst((UBYTE *)pattern, anchor);
	while (result == 0)
	{
	    resize |= createTagsForEntry((char *)anchor->ap_Buf);
	    result = MatchNext(anchor);
	}
	MatchEnd(anchor);
	eFree(anchor);
    }
    return resize;
}

# endif	/* AMIGA */
#endif	/* MSDOS || WIN32 */

static boolean createTagsForDirectory( dirName )
    const char *const dirName;
{
#ifdef HAVE_OPENDIR
    boolean resize = FALSE;
    DIR *const dir = opendir(dirName);

    if (dir == NULL)
	error(WARNING | PERROR, "Cannot recurse into directory %s", dirName);
    else
    {
	struct dirent *entry;

	if (Option.verbose)
	    printf("RECURSING into directory %s\n", dirName);
	while ((entry = readdir(dir)) != NULL)
	{
	    const char *const entryName = entry->d_name;

	    if (strcmp(entryName, ".")      != 0  &&
		strcmp(entryName, "..")     != 0  &&
		strcmp(entryName, "SCCS")   != 0  &&
		strcmp(entryName, "EIFGEN") != 0)
	    {
		vString *const filePath = combinePathAndFile(dirName,entryName);
		resize |= createTagsForEntry(vStringValue(filePath));
		vStringDelete(filePath);
	    }
	}
	closedir(dir);
    }
    return resize;
#else
# if defined(AMIGA) || defined(MSDOS) || defined(WIN32)
    vString *const pattern = vStringNew();
    boolean resize;

    if (Option.verbose)
	printf("RECURSING into directory %s\n", dirName);

    vStringCopyS(pattern, dirName);
#  ifdef AMIGA
    vStringCatS(pattern, "/#?");
#  else
    vStringCatS(pattern, "\\*.*");
#  endif
    resize = createTagsForMatchingEntries(vStringValue(pattern));
    vStringDelete(pattern);

    return resize;
# else
    return FALSE;
# endif
#endif	/* HAVE_OPENDIR */
}

static boolean createTagsForEntry( entryName )
    const char *const entryName;
{
    boolean resize = FALSE;
    langType language;

    if (isSymbolicLink(entryName)  &&  ! Option.followLinks)
    {
	if (Option.verbose)
	    printf("ignoring %s (symbolic link)\n", entryName);
    }
    else if (! doesFileExist(entryName))
	error(WARNING | PERROR, "cannot access \"%s\"", entryName);
    else if (isDirectory(entryName))
    {
	if (Option.recurse)
	    resize = createTagsForDirectory(entryName);
	else
	{
	    if (Option.verbose)
		printf("ignoring %s (directory)\n", entryName);
	    resize = FALSE;
	}
    }
    else if (! isNormalFile(entryName))
    {
	if (Option.verbose)
	    printf("ignoring %s (special file)\n", entryName);
    }
    else if ((language = getFileLanguage(entryName)) == LANG_IGNORE)
    {
	if (Option.verbose)
	    printf("ignoring %s (unknown extension)\n", entryName);
    }
    else
    {
	if (Option.filter) openTagFile();
	resize = createTagsWithFallback(entryName, language);
	if (Option.filter) closeTagFile(resize);
	addTotals(1, 0L, 0L);
    }
    return resize;
}

static boolean createTagsForArgs( args )
    cookedArgs* const args;
{
    boolean resize = FALSE;

    /*  Generate tags for each argument on the command line.
     */
    while (! cArgOff(args))
    {
	const char *arg = cArgItem(args);

#if defined(MSDOS) || defined(WIN32)
	vString *const pattern = sourceFilePath(arg);
	char *patternS = vStringValue(pattern);

	vStringCopyS(pattern, arg);

	/*  We must transform the "." and ".." forms into something that can
	 *  be expanded by the MSDOS/Windows functions.
	 */
	if (Option.recurse  &&
	    (strcmp(patternS, ".") == 0  ||  strcmp(patternS, "..") == 0))
	{
	    vStringCatS(pattern, "\\*.*");
	}
	resize |= createTagsForMatchingEntries(patternS);
	vStringDelete(pattern);
#else
	vString *const argPath = sourceFilePath(arg);
	resize |= createTagsForEntry(vStringValue(argPath));
	vStringDelete(argPath);
#endif
	cArgForth(args);
	parseOptions(args);
    }
    return resize;
}

/*  Read from an opened file a list of file names for which to generate tags.
 */
static boolean createTagsFromFileInput( fp, filter )
    FILE* const fp;
    const boolean filter;
{
    boolean resize = FALSE;
    if (fp != NULL)
    {
	cookedArgs* args = cArgNewFromLineFile(fp);
	parseOptions(args);
	while (! cArgOff(args))
	{
	    const char* const fileName = cArgItem(args);
	    vString *const filePath = sourceFilePath(fileName);
	    resize |= createTagsForEntry(vStringValue(filePath));
	    if (filter)
	    {
		if (Option.filterTerminator != NULL)
		    fputs(Option.filterTerminator, stdout);
		fflush(stdout);
	    }
	    vStringDelete(filePath);
	    cArgForth(args);
	    parseOptions(args);
	}
	cArgDelete(args);
    }
    return resize;
}

/*  Read from a named file a list of file names for which to generate tags.
 */
static boolean createTagsFromListFile( fileName )
    const char* const fileName;
{
    boolean resize;
    Assert (fileName != NULL);
    if (strcmp(fileName, "-") == 0)
	resize = createTagsFromFileInput(stdin, FALSE);
    else
    {
	FILE* const fp = fopen(fileName, "r");
	if (fp == NULL)
	    error(FATAL | PERROR, "cannot open \"%s\"", fileName);
	resize = createTagsFromFileInput(fp, FALSE);
	fclose(fp);
    }
    return resize;
}

#ifdef HAVE_CLOCK
# define CLOCK_AVAILABLE
# ifndef CLOCKS_PER_SEC
#  define CLOCKS_PER_SEC	1000000
# endif
#else
# ifdef HAVE_TIMES
#  define CLOCK_AVAILABLE
#  define CLOCKS_PER_SEC	60
static clock_t clock __ARGS((void));
static clock_t clock()
{
    struct tms buf;

    times(&buf);
    return (buf.tms_utime + buf.tms_stime);
}
# else
#  define clock()  (clock_t)0
# endif
#endif

static void printTotals( timeStamps )
    const clock_t *const timeStamps;
{
    const unsigned long totalTags = TagFile.numTags.added +
				    TagFile.numTags.prev;

    fprintf(errout, "%ld file%s, %ld line%s (%ld kB) scanned",
	    Totals.files, plural(Totals.files),
	    Totals.lines, plural(Totals.lines),
	    Totals.bytes/1024L);
#ifdef CLOCK_AVAILABLE
    {
	const double interval = ((double)(timeStamps[1] - timeStamps[0])) /
				CLOCKS_PER_SEC;

	fprintf(errout, " in %.01f seconds", interval);
	if (interval != (double)0.0)
	    fprintf(errout, " (%lu kB/s)",
		    (unsigned long)(Totals.bytes / interval) / 1024L);
    }
#endif
    fputc('\n', errout);

    fprintf(errout, "%lu tag%s added to tag file",
	    TagFile.numTags.added, plural(TagFile.numTags.added));
    if (Option.append)
	fprintf(errout, " (now %lu tags)", totalTags);
    fputc('\n', errout);

    if (totalTags > 0  &&  Option.sorted)
    {
	fprintf(errout, "%lu tag%s sorted", totalTags, plural(totalTags));
#ifdef CLOCK_AVAILABLE
	fprintf(errout, " in %.02f seconds",
		((double)(timeStamps[2] - timeStamps[1])) / CLOCKS_PER_SEC);
#endif
	fputc('\n', errout);
    }

#ifdef DEBUG
    fprintf(errout, "longest tag line = %lu\n",
	    (unsigned long)TagFile.max.line);
#endif
}

static void makeTags( args )
    cookedArgs* args;
{
    boolean resize = FALSE;
    boolean files = FALSE;
    clock_t timeStamps[3];

#define timeStamp(n) timeStamps[(n)] = (Option.printTotals ? clock():(clock_t)0)
    if (! Option.filter)
	openTagFile();

    /******/  timeStamp(0);

    if (! cArgOff(args))
    {
	files = TRUE;
	if (Option.verbose)
	    printf("Reading command line arguments\n");
	resize = createTagsForArgs(args);
    }
    if (Option.fileList != NULL)
    {
	files = TRUE;
	if (Option.verbose)
	    printf("Reading list file\n");
	resize = (boolean)(createTagsFromListFile(Option.fileList) || resize);
    }
    if (Option.filter)
    {
	files = TRUE;
	if (Option.verbose)
	    printf("Reading filter input\n");
	resize = (boolean)(createTagsFromFileInput(stdin, TRUE) || resize);
    }
    if (! files)
    {
	if (Option.recurse)
	{
	    cookedArgs* const dot = cArgNewFromString(".");
	    resize = createTagsForArgs(dot);
	    cArgDelete(dot);
	}
	else
	    error(FATAL, "No files specified. Try \"%s --help\".",
		getExecutableName());
    }

    /******/  timeStamp(1);

    if (! Option.filter)
	closeTagFile(resize);

    /******/  timeStamp(2);

    if (Option.printTotals)
	printTotals(timeStamps);
#undef timeStamp
}

/*----------------------------------------------------------------------------
 *	Start up code
 *--------------------------------------------------------------------------*/

static void setExecutableName( path )
    const char *const path;
{
    ExecutableProgram = path;
    ExecutableName = baseFilename(path);
}

extern const char *getExecutableName()
{
    return ExecutableName;
}

extern int main( argc, argv )
    int __unused__ argc;
    char **argv;
{
    cookedArgs *args;

#ifdef AMIGA
    /* This program doesn't work when started from the Workbench */
    if (argc == 0)
	exit(1);
#endif

#ifdef __EMX__
    _wildcard(&argc, &argv);	/* expand wildcards in argument list */
#endif

    setExecutableName(*argv++);
    testEtagsInvocation();

    args = cArgNewFromArgv(argv);
    previewFirstOption(args);
    initOptions();
    readOptionConfiguration();
    if (Option.verbose)
	printf("Reading options from command line\n");
    parseOptions(args);
    checkOptions();
    makeTags(args);

    /*  Clean up.
     */
    cArgDelete(args);
    freeKeywordTable();
    freeSourceFileResources();
    freeTagFileResources();
    freeOptionResources();

    exit(0);
    return 0;
}

/* vi:set tabstop=8 shiftwidth=4: */
