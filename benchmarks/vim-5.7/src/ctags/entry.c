/*****************************************************************************
*   $Id: entry.c,v 8.19 2000/04/23 17:35:51 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for creating tag entries.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>
#include <ctype.h>	/* to define isspace() */
#include <errno.h>

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>	    /* to declare off_t on some hosts */
#endif
#if defined(HAVE_UNISTD_H)
# include <unistd.h>	/* to declare close(), ftruncate(), truncate() */
#endif

/*  These header files provide for the functions necessary to do file
 *  truncation.
 */
#include <fcntl.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "debug.h"
#include "entry.h"
#include "main.h"
#include "options.h"
#include "read.h"
#include "sort.h"
#include "strlist.h"

/*============================================================================
*=	Defines
============================================================================*/
#define PSEUDO_TAG_PREFIX	"!_"

#define includeExtensionFlags()		(Option.tagFileFormat > 1)

/*----------------------------------------------------------------------------
 *  Portability defines
 *--------------------------------------------------------------------------*/
#if !defined(HAVE_TRUNCATE) && !defined(HAVE_FTRUNCATE) && !defined(HAVE_CHSIZE)
# define USE_REPLACEMENT_TRUNCATE
#endif

/*  Hack for rediculous practice of Microsoft Visual C++.
 */
#if defined(WIN32) && defined(_MSC_VER)
# define chsize		_chsize
# define open		_open
# define close		_close
# define O_RDWR 	_O_RDWR
#endif

/*============================================================================
=   Data definitions
============================================================================*/

tagFile TagFile = {
    NULL,		/* file name */
    NULL,		/* file pointer */
    { 0, 0 },		/* numTags */
    { 0, 0, 0 },	/* max */
    { NULL, NULL, 0 },	/* etags */
    NULL		/* vLine */
};

static boolean TagsToStdout = FALSE;

/*============================================================================
=   Function prototypes
============================================================================*/
#ifdef NEED_PROTO_TRUNCATE
extern int truncate __ARGS((const char *path, off_t length));
#endif

#ifdef NEED_PROTO_FTRUNCATE
extern int ftruncate __ARGS((int fd, off_t length));
#endif

static void writePseudoTag __ARGS((const char *const tagName, const char *const fileName, const char *const pattern));
static void updateSortedFlag __ARGS((const char *const line, FILE *const fp, fpos_t startOfLine));
static void addPseudoTags __ARGS((void));
static long unsigned int updatePseudoTags __ARGS((FILE *const fp));
static boolean isValidTagAddress __ARGS((const char *const tag));
static boolean isCtagsLine __ARGS((const char *const tag));
static boolean isEtagsLine __ARGS((const char *const tag));
static boolean isTagFile __ARGS((const char *const filename));
#ifdef ENABLE_UPDATE
static boolean isNameInList __ARGS((Arguments* const args, const char *const name, const size_t length));
#endif
static void sortTagFile __ARGS((void));
static void resizeTagFile __ARGS((const long newSize));
static void writeEtagsIncludes __ARGS((FILE *const fp));
static size_t writeSourceLine __ARGS((FILE *const fp, const char *const line));
static size_t writeCompactSourceLine __ARGS((FILE *const fp, const char *const line));
static void rememberMaxLengths __ARGS((const size_t nameLength, const size_t lineLength));
static int writeXrefEntry __ARGS((const tagEntryInfo *const tag));
static void truncateTagLine __ARGS((char *const line, const char *const token, const boolean discardNewline));
static int writeEtagsEntry __ARGS((const tagEntryInfo *const tag));
static int addExtensionFlags __ARGS((const tagEntryInfo *const tag));
static int writePatternEntry __ARGS((const tagEntryInfo *const tag));
static int writeLineNumberEntry __ARGS((const tagEntryInfo *const tag));
static int writeCtagsEntry __ARGS((const tagEntryInfo *const tag));

/*============================================================================
=   Function definitions
============================================================================*/

extern void freeTagFileResources()
{
    vStringDelete(TagFile.vLine);
}

extern const char *tagFileName()
{
    return TagFile.name;
}

/*----------------------------------------------------------------------------
*-	Pseudo tag support
----------------------------------------------------------------------------*/

static void writePseudoTag( tagName, fileName, pattern )
    const char *const tagName;
    const char *const fileName;
    const char *const pattern;
{
    const int length = fprintf(TagFile.fp, "%s%s\t%s\t/%s/\n",
			       PSEUDO_TAG_PREFIX, tagName, fileName, pattern);
    ++TagFile.numTags.added;
    rememberMaxLengths(strlen(tagName), (size_t)length);
}

static void addPseudoTags()
{
    if (! Option.xref)
    {
	char format[11];
	const char *formatComment = "unknown format";

	sprintf(format, "%u", Option.tagFileFormat);

	if (Option.tagFileFormat == 1)
	    formatComment = "original ctags format";
	else if (Option.tagFileFormat == 2)
	    formatComment =
		    "extended format; --format=1 will not append ;\" to lines";

	writePseudoTag("TAG_FILE_FORMAT", format, formatComment);
	writePseudoTag("TAG_FILE_SORTED", Option.sorted ? "1":"0",
		       "0=unsorted, 1=sorted");
	writePseudoTag("TAG_PROGRAM_AUTHOR",	AUTHOR_NAME,  AUTHOR_EMAIL);
	writePseudoTag("TAG_PROGRAM_NAME",	PROGRAM_NAME, "");
	writePseudoTag("TAG_PROGRAM_URL",	PROGRAM_URL,  "official site");
	writePseudoTag("TAG_PROGRAM_VERSION",	PROGRAM_VERSION,
		       "with C, C++, Eiffel, Fortran, and Java  support");
    }
}

static void updateSortedFlag( line, fp, startOfLine )
    const char *const line;
    FILE *const fp;
    fpos_t startOfLine;
{
    const char *const tab = strchr(line, '\t');

    if (tab != NULL)
    {
	const long boolOffset = tab - line + 1;		/* where it should be */

	if (line[boolOffset] == '0'  ||  line[boolOffset] == '1')
	{
	    fpos_t nextLine;

	    if (fgetpos(fp, &nextLine) == -1 || fsetpos(fp, &startOfLine) == -1)
		error(WARNING, "Failed to update 'sorted' pseudo-tag");
	    else
	    {
		fpos_t flagLocation;
		int c, d;

		do
		    c = fgetc(fp);
		while (c != '\t'  &&  c != '\n');
		fgetpos(fp, &flagLocation);
		d = fgetc(fp);
		if (c == '\t'  &&  (d == '0'  ||  d == '1')  &&
		    d != (int)Option.sorted)
		{
		    fsetpos(fp, &flagLocation);
		    fputc(Option.sorted ? '1' : '0', fp);
		}
		fsetpos(fp, &nextLine);
	    }
	}
    }
}

/*  Look through all line beginning with "!_TAG_FILE", and update those which
 *  require it.
 */
static unsigned long updatePseudoTags( fp )
    FILE *const fp;
{
    enum { maxClassLength = 20 };
    char class[maxClassLength + 1];
    unsigned long linesRead = 0;
    fpos_t startOfLine;
    size_t classLength;
    const char *line;

    sprintf(class, "%sTAG_FILE", PSEUDO_TAG_PREFIX);
    classLength = strlen(class);
    Assert(classLength < maxClassLength);

    fgetpos(fp, &startOfLine);
    line = readLine(TagFile.vLine, fp);
    while (line != NULL  &&  line[0] == class[0])
    {
	++linesRead;
	if (strncmp(line, class, classLength) == 0)
	{
	    char tab, classType[16];

	    if (sscanf(line + classLength, "%15s%c", classType, &tab) == 2  &&
		tab == '\t')
	    {
		if (strcmp(classType, "_SORTED") == 0)
		    updateSortedFlag(line, fp, startOfLine);
	    }
	    fgetpos(fp, &startOfLine);
	}
	line = readLine(TagFile.vLine, fp);
    }
    while (line != NULL)			/* skip to end of file */
    {
	++linesRead;
	line = readLine(TagFile.vLine, fp);
    }
    return linesRead;
}

/*----------------------------------------------------------------------------
 *  Tag file management
 *--------------------------------------------------------------------------*/

static boolean isValidTagAddress( excmd )
    const char *const excmd;
{
    boolean isValid = FALSE;

    if (strchr("/?", excmd[0]) != NULL)
	isValid = TRUE;
    else
    {
	char *address = (char *)eMalloc(strlen(excmd) + 1);

	if (address != NULL)
	{
	    if (sscanf(excmd, "%[^;\n]", address) == 1  &&
		strspn(address,"0123456789") == strlen(address))
		    isValid = TRUE;
	    eFree(address);
	}
    }
    return isValid;
}

static boolean isCtagsLine( line )
    const char *const line;
{
    enum fieldList { TAG, TAB1, SRC_FILE, TAB2, EXCMD, NUM_FIELDS };
    boolean ok = FALSE;			/* we assume not unless confirmed */
    const size_t fieldLength = strlen(line) + 1;
    char *const fields = (char *)eMalloc((size_t)NUM_FIELDS * fieldLength);

    if (fields == NULL)
	error(FATAL, "Cannot analyze tag file");
    else
    {
#define field(x)	(fields + ((size_t)(x) * fieldLength))

	const int numFields = sscanf(line, "%[^\t]%[\t]%[^\t]%[\t]%[^\r\n]",
				     field(TAG), field(TAB1), field(SRC_FILE),
				     field(TAB2), field(EXCMD));

	/*  There must be exactly five fields: two tab fields containing
	 *  exactly one tab each, the tag must not begin with "#", and the
	 *  file name should not end with ";", and the excmd must be
	 *  accceptable.
	 *
	 *  These conditions will reject tag-looking lines like:
	 *      int	a;	<C-comment>
	 *      #define	LABEL	<C-comment>
	 */
	if (numFields == NUM_FIELDS   &&
	    strlen(field(TAB1)) == 1  &&
	    strlen(field(TAB2)) == 1  &&
	    field(TAG)[0] != '#'      &&
	    field(SRC_FILE)[strlen(field(SRC_FILE)) - 1] != ';'  &&
	    isValidTagAddress(field(EXCMD)))
		ok = TRUE;

	eFree(fields);
    }
    return ok;
}

static boolean isEtagsLine( line )
    const char *const line;
{
    const char *const magic = "\f\n";

    return (boolean)(strncmp(line, magic, strlen(magic)) == 0);
}

static boolean isTagFile( filename )
    const char *const filename;
{
    boolean ok = FALSE;			/* we assume not unless confirmed */
    FILE *const fp = fopen(filename, "rb");

    if (fp == NULL  &&  errno == ENOENT)
	ok = TRUE;
    else if (fp != NULL)
    {
	const char *line = readLine(TagFile.vLine, fp);

	if (line == NULL)
	    ok = TRUE;
	else if (Option.etags)		/* check for etags magic number */
	    ok = isEtagsLine(line);
	else
	    ok = isCtagsLine(line);
	fclose(fp);
    }
    return ok;
}

extern void copyBytes( fromFp, toFp, size )
    FILE* const fromFp;
    FILE* const toFp;
    const long size;
{
    enum { BufferSize = 1000 };
    long toRead, numRead;
    char* buffer = (char*)eMalloc((size_t)BufferSize);
    long remaining = size;
    do
    {
	toRead = (0 < remaining && remaining < BufferSize) ?
		    remaining : BufferSize;
	numRead = fread(buffer, (size_t)1, (size_t)toRead, fromFp);
	if (fwrite(buffer, (size_t)1, (size_t)numRead, toFp) < (size_t)numRead)
	    error (FATAL | PERROR, "cannot complete write");
	if (remaining > 0)
	    remaining -= numRead;
    } while (numRead == toRead  &&  remaining != 0);
    eFree(buffer);
}

extern void copyFile( from, to, size )
    const char *const from;
    const char *const to;
    const long size;
{
    FILE* const fromFp = fopen(from, "rb");
    if (fromFp == NULL)
	error (FATAL | PERROR, "cannot open file to copy");
    else
    {
	FILE* const toFp = fopen(to, "wb");
	if (toFp == NULL)
	    error (FATAL | PERROR, "cannot open copy destination");
	else
	{
	    copyBytes (fromFp, toFp, size);
	    fclose(toFp);
	}
	fclose(fromFp);
    }
}

extern void openTagFile()
{
    setDefaultTagFileName();
    TagsToStdout = isDestinationStdout();

    if (TagFile.vLine == NULL)
	TagFile.vLine = vStringNew();

    /*  Open the tags file.
     */
    if (TagsToStdout)
	TagFile.fp = tempFile("w", &TagFile.name);
    else
    {
	char* fname;
	boolean fileExists;

	setDefaultTagFileName();
	fname = (char*)eMalloc(strlen(Option.tagFileName) + 1);
	strcpy(fname, Option.tagFileName);
	TagFile.name = fname;
	fileExists = doesFileExist(fname);
	if (fileExists  &&  ! isTagFile(fname))
	    error(FATAL,
	      "\"%s\" doesn't look like a tag file; I refuse to overwrite it.",
		  fname);

	if (Option.etags)
	{
	    if (Option.append  &&  fileExists)
		TagFile.fp = fopen(fname, "a+b");
	    else
		TagFile.fp = fopen(fname, "w+b");
	}
	else
	{
	    if (Option.append  &&  fileExists)
	    {
		TagFile.fp = fopen(fname, "r+");
		if (TagFile.fp != NULL)
		{
		    TagFile.numTags.prev = updatePseudoTags(TagFile.fp);
		    fclose(TagFile.fp);
		    TagFile.fp = fopen(fname, "a+");
		}
	    }
	    else
	    {
		TagFile.fp = fopen(fname, "w");
		if (TagFile.fp != NULL)
		    addPseudoTags();
	    }
	}
	if (TagFile.fp == NULL)
	{
	    error(FATAL | PERROR, "cannot open tag file");
	    exit(1);
	}
    }
}

#ifdef USE_REPLACEMENT_TRUNCATE

static int replacementTruncate __ARGS((const char *const name, const long size));

/*  Replacement for missing library function.
 */
static int replacementTruncate( name, size )
    const char *const name;
    const long size;
{
    char *tempName = NULL;
    FILE *fp = tempFile("w", &tempName);
    fclose(fp);
    copyFile(name, tempName, size);
    copyFile(tempName, name, WHOLE_FILE);
    remove(tempName);
    eFree(tempName);

    return 0;
}

#endif

static void sortTagFile()
{
    if (TagFile.numTags.added > 0L)
    {
	if (Option.sorted)
	{
	    if (Option.verbose)
		printf("sorting tag file\n");
#ifdef EXTERNAL_SORT
	    externalSortTags(TagsToStdout);
#else
	    internalSortTags(TagsToStdout);
#endif
	}
	else if (TagsToStdout)
	    catFile(tagFileName());
    }
    if (TagsToStdout)
	remove(tagFileName());		/* remove temporary file */
}

static void resizeTagFile( newSize )
    const long newSize;
{
    int result = 0;

#ifdef USE_REPLACEMENT_TRUNCATE
    result = replacementTruncate(TagFile.name, newSize);
#else
# ifdef HAVE_TRUNCATE
    result = truncate(TagFile.name, (off_t)newSize);
# else
    const int fd = open(TagFile.name, O_RDWR);

    if (fd != -1)
    {
#  ifdef HAVE_FTRUNCATE
	result = ftruncate(fd, (off_t)newSize);
#  else
#   ifdef HAVE_CHSIZE
	result = chsize(fd, newSize);
#   endif
#  endif
	close(fd);
    }
# endif
#endif
    if (result == -1)
	fprintf(errout, "Cannot shorten tag file: errno = %d\n", errno);
}

static void writeEtagsIncludes( fp )
    FILE *const fp;
{
    if (Option.etagsInclude)
    {
	unsigned int i;
	for (i = 0  ;  i < stringListCount(Option.etagsInclude)  ;  ++i)
	{
	    vString *item = stringListItem(Option.etagsInclude, i);
	    fprintf(fp, "\f\n%s,include\n", vStringValue(item));
	}
    }
}

extern void closeTagFile( resize )
    const boolean resize;
{
    long desiredSize, size;

    if (Option.etags)
	writeEtagsIncludes(TagFile.fp);
    desiredSize = ftell(TagFile.fp);
    fseek(TagFile.fp, 0L, SEEK_END);
    size = ftell(TagFile.fp);
    fclose(TagFile.fp);
    if (resize  &&  desiredSize < size)
    {
	DebugStatement(
	    debugPrintf(DEBUG_STATUS, "shrinking %s from %ld to %ld bytes\n",
			TagFile.name, size, desiredSize); )
	resizeTagFile(desiredSize);
    }
    if (! Option.etags)
	sortTagFile();
    eFree(TagFile.name);
    TagFile.name = NULL;
}

extern void beginEtagsFile()
{
    TagFile.etags.fp = tempFile("w+b", &TagFile.etags.name);
    TagFile.etags.byteCount = 0;
}

extern void endEtagsFile( name )
    const char *const name;
{
    const char *line;

    fprintf(TagFile.fp, "\f\n%s,%ld\n", name, (long)TagFile.etags.byteCount);
    if (TagFile.etags.fp != NULL)
    {
	rewind(TagFile.etags.fp);
	while ((line = readLine(TagFile.vLine, TagFile.etags.fp)) != NULL)
	    fputs(line, TagFile.fp);
	fclose(TagFile.etags.fp);
	remove(TagFile.etags.name);
	eFree(TagFile.etags.name);
	TagFile.etags.fp = NULL;
	TagFile.etags.name = NULL;
    }
}

/*----------------------------------------------------------------------------
 *  Tag entry management
 *--------------------------------------------------------------------------*/

/*  This function copies the current line out to a specified file. It has no
 *  effect on the fileGetc() function.  During copying, any '\' characters
 *  are doubled and a leading '^' or trailing '$' is also quoted. End of line
 *  characters (line feed or carriage return) are dropped.
 */
static size_t writeSourceLine( fp, line )
    FILE *const fp;
    const char *const line;
{
    size_t length = 0;
    const char *p;

    /*	Write everything up to, but not including, a line end character.
     */
    for (p = line  ;  *p != '\0'  ;  ++p)
    {
	const int next = *(p + 1);
	const int c = *p;

	if (c == CRETURN  ||  c == NEWLINE)
	    break;

	/*  If character is '\', or a terminal '$', then quote it.
	 */
	if (c == BACKSLASH  ||  c == (Option.backward ? '?' : '/')  ||
	    (c == '$'  &&  (next == NEWLINE  ||  next == CRETURN)))
	{
	    putc(BACKSLASH, fp);
	    ++length;
	}
	putc(c, fp);
	++length;
    }
    return length;
}

/*  Writes "line", stripping leading and duplicate white space.
 */
static size_t writeCompactSourceLine( fp, line )
    FILE *const fp;
    const char *const line;
{
    boolean lineStarted = FALSE;
    size_t  length = 0;
    const char *p;
    int c;

    /*	Write everything up to, but not including, the newline.
     */
    for (p = line, c = *p  ;  c != NEWLINE  &&  c != '\0'  ;  c = *++p)
    {
	if (lineStarted  || ! isspace(c))	/* ignore leading spaces */
	{
	    lineStarted = TRUE;
	    if (isspace(c))
	    {
		int next;

		/*  Consume repeating white space.
		 */
		while (next = *(p+1) , isspace(next)  &&  next != NEWLINE)
		    ++p;
		c = ' ';	/* force space character for any white space */
	    }
	    if (c != CRETURN  ||  *(p + 1) != NEWLINE)
	    {
		putc(c, fp);
		++length;
	    }
	}
    }
    return length;
}

static void rememberMaxLengths( nameLength, lineLength )
    const size_t nameLength;
    const size_t lineLength;
{
    if (nameLength > TagFile.max.tag)
	TagFile.max.tag = nameLength;

    if (lineLength > TagFile.max.line)
	TagFile.max.line = lineLength;
}

static int writeXrefEntry( tag )
    const tagEntryInfo *const tag;
{
    const char *const line =
			readSourceLine(TagFile.vLine, tag->filePosition, NULL);
    int length = fprintf(TagFile.fp, "%-20s %-10s %4lu  %-14s ", tag->name,
			 tag->kindName, tag->lineNumber, tag->sourceFileName);

    length += writeCompactSourceLine(TagFile.fp, line);
    putc(NEWLINE, TagFile.fp);
    ++length;

    return length;
}

/*  Truncates the text line containing the tag at the character following the
 *  tag, providing a character which designates the end of the tag.
 */
static void truncateTagLine( line, token, discardNewline )
    char *const line;
    const char *const token;
    const boolean discardNewline;
{
    char *p = strstr(line, token);

    if (p != NULL)
    {
	p += strlen(token);
	if (*p != '\0'  &&  !(*p == '\n'  &&  discardNewline))
	    ++p;		/* skip past character terminating character */
	*p = '\0';
    }
}

static int writeEtagsEntry( tag )
    const tagEntryInfo *const tag;
{
    int length;

    if (tag->isFileEntry)
	length = fprintf(TagFile.etags.fp, "\177%s\001%lu,0\n",
			 tag->name, tag->lineNumber);
    else
    {
	long seekValue;
	char *const line =
		readSourceLine(TagFile.vLine, tag->filePosition, &seekValue);

	if (tag->truncateLine)
	    truncateTagLine(line, tag->name, TRUE);
	else
	{
	    char *p;

	    for (p = line  ;  *p != '\0'  ;  ++p)
		if (strchr("\n\r", *p) != NULL)
		{
		    *p = '\0';
		    break;
		}
	}
	length = fprintf(TagFile.etags.fp, "%s\177%s\001%lu,%ld\n", line,
			 tag->name, tag->lineNumber, seekValue);
    }
    TagFile.etags.byteCount += length;

    return length;
}

static int addExtensionFlags( tag )
    const tagEntryInfo *const tag;
{
    const char *const prefix = ";\"\t";
    const char *const separator = "\t";
    unsigned int i;
    int length = 0;

    /*  Add an extension flag designating that the type of the tag.
     */
    if (Option.kindLong)
	length += fprintf(TagFile.fp, "%s%s", prefix, tag->kindName);
    else
	length += fprintf(TagFile.fp, "%s%c", prefix, tag->kind);

    /*  If this is a static tag, add the appropriate extension flag.
     */
    if (tag->isFileScope)
	length += fprintf(TagFile.fp, "%sfile:", separator);

    /*  Add any other extension flags.
     */
    for (i = 0  ;  i < tag->otherFields.count  ;  ++i)
    {
	const char *const label = tag->otherFields.label[i];
	const char *const value = tag->otherFields.value[i];

	if (label != NULL)
	    length += fprintf(TagFile.fp, "%s%s:%s", separator,
			      label, value == NULL ? "" : value);
    }

    return length;
}

static int writePatternEntry( tag )
    const tagEntryInfo *const tag;
{
    char *const line = readSourceLine(TagFile.vLine, tag->filePosition, NULL);
    const int searchChar = Option.backward ? '?' : '/';
    boolean newlineTerminated;
    int length = 0;

    if (tag->truncateLine)
	truncateTagLine(line, tag->name, FALSE);
    newlineTerminated = (boolean)(strchr("\n\r", line[strlen(line)-1]) != NULL);

    length += fprintf(TagFile.fp, "%c^", searchChar);
    length += writeSourceLine(TagFile.fp, line);
    length += fprintf(TagFile.fp, "%s%c", newlineTerminated ? "$":"",
		      searchChar);

    return length;
}

static int writeLineNumberEntry( tag )
    const tagEntryInfo *const tag;
{
    return fprintf(TagFile.fp, "%lu", tag->lineNumber);
}

static int writeCtagsEntry( tag )
    const tagEntryInfo *const tag;
{
    int length = fprintf(TagFile.fp, "%s\t%s\t", tag->name,tag->sourceFileName);

    if (tag->lineNumberEntry)
	length += writeLineNumberEntry(tag);
    else
	length += writePatternEntry(tag);

    if (includeExtensionFlags())
	length += addExtensionFlags(tag);

    length += fprintf(TagFile.fp, "\n");

    return length;
}

extern void makeTagEntry( tag )
    const tagEntryInfo *const tag;
{
    int length = 0;

    DebugStatement( debugEntry(tag); )

    if (Option.xref)
    {
	if (! tag->isFileEntry)
	    length = writeXrefEntry(tag);
    }
    else if (Option.etags)
	length = writeEtagsEntry(tag);
    else
	length = writeCtagsEntry(tag);

    ++TagFile.numTags.added;
    rememberMaxLengths(strlen(tag->name), (size_t)length);
}

extern void initTagEntry( e, name )
    tagEntryInfo *const e;
    const char *const name;
{
    e->lineNumberEntry	= (boolean)(Option.locate == EX_LINENUM);
    e->lineNumber	= getFileLine();
    e->filePosition	= getFilePosition();
    e->isFileScope	= FALSE;
    e->isFileEntry	= FALSE;
    e->truncateLine	= FALSE;
    e->sourceFileName	= getFileName();
    e->name		= name;
    e->kindName		= "";
    e->kind		= ' ';
    e->otherFields.count = 0;
    e->otherFields.label = NULL;
    e->otherFields.value = NULL;
}

/* vi:set tabstop=8 shiftwidth=4: */
