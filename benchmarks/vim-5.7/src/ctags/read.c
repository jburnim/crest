/*****************************************************************************
*   $Id: read.c,v 8.2 2000/03/27 05:23:14 darren Exp $
*
*   Copyright (c) 1996-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains low level source and tag file read functions (newline
*   conversion for source files are performed at this level).
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>

#include "ctags.h"
#define FILE_WRITE
#include "read.h"

#include "entry.h"
#include "main.h"
#include "options.h"
#include "debug.h"

/*============================================================================
=   Data declarations
============================================================================*/

typedef struct sLangTab {
    const char *extension;
    langType language;
} langTab;

/*============================================================================
=   Data definitions
============================================================================*/

inputFile File;			/* globally read through macros */

static fpos_t StartOfLine;	/* holds deferred position of start of line */

/*============================================================================
=   Function prototypes
============================================================================*/
static void setInputFileName __ARGS((const char *const fileName));
static void setSourceFileParameters __ARGS((vString *const fileName));
static void fileNewline __ARGS((void));

/*============================================================================
=   Function definitions
============================================================================*/

extern void freeSourceFileResources()
{
    vStringDelete(File.name);
    vStringDelete(File.path);
    vStringDelete(File.source.name);
}

/*----------------------------------------------------------------------------
 *	Generic line reading with automatic buffer sizing
 *--------------------------------------------------------------------------*/

extern char *readLine( vLine, fp )
    vString *const vLine;
    FILE *const fp;
{
    char *line = NULL;

    vStringClear(vLine);
    if (fp == NULL)		/* to free memory allocated to buffer */
	error(FATAL, "NULL file pointer");
    else
    {
	boolean reReadLine;

	/*  If reading the line places any character other than a null or a
	 *  newline at the last character position in the buffer (one less
	 *  than the buffer size), then we must resize the buffer and
	 *  reattempt to read the line.
	 */
	do
	{
	    char *const pLastChar = vStringValue(vLine) + vStringSize(vLine) -2;
	    fpos_t startOfLine;

	    fgetpos(fp, &startOfLine);
	    reReadLine = FALSE;
	    *pLastChar = '\0';
	    line = fgets(vStringValue(vLine), (int)vStringSize(vLine), fp);
	    if (line == NULL)
	    {
		if (! feof(fp))
		    error(FATAL | PERROR, "Failure on attempt to read file");
	    }
	    else if (*pLastChar != '\0'  &&  *pLastChar != '\n')
	    {
		/*  buffer overflow */
		reReadLine = vStringAutoResize(vLine);
		if (reReadLine)
		    fsetpos(fp, &startOfLine);
		else
		    error(FATAL | PERROR, "input line too big; out of memory");
	    }
	    else
		vStringSetLength(vLine);
	} while (reReadLine);
    }
    return line;
}

/*----------------------------------------------------------------------------
 *	Source file access functions
 *--------------------------------------------------------------------------*/

static void setInputFileName( fileName )
    const char *const fileName;
{
    const char *const head = fileName;
    const char *const tail = baseFilename(head);

    if (File.name != NULL)
	vStringDelete(File.name);
    File.name = vStringNewInit(fileName);

    if (File.path != NULL)
	vStringDelete(File.path);

    if (tail == head)
	File.path = NULL;
    else
    {
	const size_t length = tail - head - 1;
	File.path = vStringNew();
	vStringNCopyS(File.path, fileName, length);
    }
}

static void setSourceFileParameters( fileName )
    vString *const fileName;
{
    if (File.source.name != NULL)
	vStringDelete(File.source.name);
    File.source.name = fileName;

    if (vStringLength(fileName) > TagFile.max.file)
	TagFile.max.file = vStringLength(fileName);

    File.source.isHeader = isFileHeader(vStringValue(fileName));
    File.source.language = getFileLanguage(vStringValue(fileName));
}

extern void setSourceFileName( fileName )
    vString *const fileName;
{
    vString *pathName;

    if (isAbsolutePath(vStringValue(fileName)) || File.path == NULL)
	pathName = fileName;
    else
	pathName = combinePathAndFile(vStringValue(File.path),
				      vStringValue(fileName));

    setSourceFileParameters(pathName);
}

extern void setSourceFileLine( lineNumber )
    const unsigned long lineNumber;
{
    File.source.lineNumber = lineNumber;
}

/*  This function opens a source file, and resets the line counter.  If it
 *  fails, it will display an error message and leave the File.fp set to NULL.
 */
extern boolean fileOpen( fileName, language )
    const char *const fileName;
    const langType language;
{
#ifdef __vms
    const char *const openMode = "r";
#else
    const char *const openMode = "rb";
#endif
    boolean opened = FALSE;

    /*	If another file was already open, then close it.
     */
    if (File.fp != NULL)
    {
	fclose(File.fp);		/* close any open source file */
	File.fp = NULL;
    }

    File.fp = fopen(fileName, openMode);
    if (File.fp == NULL)
	error(WARNING | PERROR, "cannot open \"%s\"", fileName);
    else
    {
	opened = TRUE;

	setInputFileName(fileName);
	fgetpos(File.fp, &StartOfLine);
	fgetpos(File.fp, &File.filePosition);
	File.lineNumber   = 0L;
	File.newLine      = TRUE;
	File.language     = language;

	setSourceFileParameters(vStringNewInit(fileName));
	File.source.lineNumber = 0L;

	if (Option.verbose)
	{
	    const char *name = getLanguageName(language);

	    printf("OPENING %s as %c%s language %sfile\n", fileName,
		   toupper((int)name[0]), name + 1,
		   File.source.isHeader ? "include " : "");
	}
    }
    return opened;
}

extern void fileClose()
{
    if (File.fp != NULL)
    {
	/*  The line count of the file is 1 too big, since it is one-based
	 *  and is incremented upon each newline.
	 */
	if (Option.printTotals)
	    addTotals(0, File.lineNumber - 1L,
		      getFileSize(vStringValue(File.name)));

	fclose(File.fp);
	File.fp = NULL;
    }
}

/*  Action to take for each encountered source newline.
 */
static void fileNewline()
{
    File.filePosition = StartOfLine;
    File.newLine = FALSE;
    File.lineNumber++;
    File.source.lineNumber++;
    DebugStatement( if (Option.breakLine == File.lineNumber) lineBreak(); )
    DebugStatement( debugPrintf(DEBUG_RAW, "%6d: ", File.lineNumber); )
}

/*  This function reads a single character from the stream, performing newline
 *  canonicalization.
 */
extern int fileGetc()
{
    int	c;

    /*	If there is an ungotten character, then return it.  Don't do any
     *	other processing on it, though, because we already did that the
     *	first time it was read through fileGetc().
     */
    if (File.ungetch != '\0')
    {
	c = File.ungetch;
	File.ungetch = '\0';
	return c;	    /* return here to avoid re-calling debugPutc() */
    }

    c = getc(File.fp);

    /*	If previous character was a newline, then we're starting a line.
     */
    if (File.newLine  &&  c != EOF)
	fileNewline();

    if (c == NEWLINE)
    {
	File.newLine = TRUE;
	fgetpos(File.fp, &StartOfLine);
    }
    else if (c == CRETURN)
    {
	/*  Turn line breaks into a canonical form. The three commonly
	 *  used forms if line breaks: LF (UNIX), CR (MacIntosh), and
	 *  CR-LF (MS-DOS) are converted into a generic newline.
	 */
	const int next = getc(File.fp);		/* is CR followed by LF? */

	if (next != NEWLINE)
	    ungetc(next, File.fp);

	c = NEWLINE;				/* convert CR into newline */
	File.newLine = TRUE;
	fgetpos(File.fp, &StartOfLine);
    }
    DebugStatement( debugPutc(c, DEBUG_RAW); )
    return c;
}

extern void fileUngetc( c )
    int c;
{
    File.ungetch = c;
}

/*  Places into the line buffer the contents of the line referenced by
 *  "location".
 */
extern char *readSourceLine( vLine, location, pSeekValue )
    vString *const vLine;
    fpos_t location;
    long *const pSeekValue;
{
    fpos_t orignalPosition;
    char *line;

    fgetpos(File.fp, &orignalPosition);
    fsetpos(File.fp, &location);
    if (pSeekValue != NULL)
	*pSeekValue = ftell(File.fp);
    line = readLine(vLine, File.fp);
    if (line == NULL)
	error(FATAL, "Unexpected end of file: %s", File.name);
    fsetpos(File.fp, &orignalPosition);

    return line;
}

/* vi:set tabstop=8 shiftwidth=4: */
