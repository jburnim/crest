/*****************************************************************************
*   $Id: fortran.c,v 8.12 1999/11/27 22:50:27 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for Fortran language
*   files.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>
#include <limits.h>
#include <ctype.h>	/* to define tolower() */
#include <setjmp.h>

#include "debug.h"
#include "entry.h"
#include "keyword.h"
#include "main.h"
#include "options.h"
#include "parse.h"
#include "read.h"
#include "vstring.h"

/*============================================================================
=   Macros
============================================================================*/
#define isBlank(c)		(boolean)(c == ' ' || c == '\t')
#define isType(token,t)		(boolean)((token)->type == (t))
#define isKeyword(token,k)	(boolean)((token)->keyword == (k))

/*============================================================================
=   Data declarations
============================================================================*/

typedef enum eException {
    ExceptionNone, ExceptionEOF, ExceptionFixedFormat
} exception_t;

/*  Used to designate type of line read in fixed source form.
 */
typedef enum eFortranLineType {
    LTYPE_UNDETERMINED,
    LTYPE_INVALID,
    LTYPE_COMMENT,
    LTYPE_CONTINUATION,
    LTYPE_EOF,
    LTYPE_INITIAL,
    LTYPE_SHORT
} lineType;

/*  Used to specify type of keyword.
 */
typedef enum eKeywordId {
    KEYWORD_NONE,
    KEYWORD_assignment,
    KEYWORD_block,
    KEYWORD_common,
    KEYWORD_data,
    KEYWORD_end,
    KEYWORD_entry,
    KEYWORD_function,
    KEYWORD_interface,
    KEYWORD_module,
    KEYWORD_namelist,
    KEYWORD_operator,
    KEYWORD_program,
    KEYWORD_subroutine,
    KEYWORD_type
} keywordId;

/*  Used to determine whether keyword is valid for the token language and
 *  what its ID is.
 */
typedef struct sKeywordDesc {
    const char *name;
    keywordId id;
} keywordDesc;

typedef enum eTokenType {
    TOKEN_UNDEFINED,
    TOKEN_COMMA,
    TOKEN_DECLARATION,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_LABEL,
    TOKEN_NEWLINE,
    TOKEN_NUMERIC,
    TOKEN_OPERATOR,
    TOKEN_PAREN_CLOSE,
    TOKEN_PAREN_OPEN,
    TOKEN_STATEMENT_END,
    TOKEN_STRING
} tokenType;

typedef enum eTagType {
    TAG_UNDEFINED,
    TAG_BLOCK_DATA,
    TAG_COMMON_BLOCK,
    TAG_ENTRY_POINT,
    TAG_FUNCTION,
    TAG_INTERFACE,
    TAG_LABEL,
    TAG_MODULE,
    TAG_NAMELIST,
    TAG_PROGRAM,
    TAG_SUBROUTINE,
    TAG_DERIVED_TYPE,
    TAG_VARIABLE,
    TAG_COUNT		/* must be last */
} tagType;

typedef struct sTokenInfo {
    tokenType		type;
    keywordId		keyword;
    tagType		tag;
    vString *		string;
    unsigned long	lineNumber;
    fpos_t		filePosition;
} tokenInfo;

/*============================================================================
=   Data definitions
============================================================================*/

static jmp_buf Exception;
static int Ungetc = '\0';
static unsigned int Column = 0;
static boolean FreeSourceForm = FALSE;
static tokenInfo *Parent = NULL;

static const keywordDesc FortranKeywordTable[] = {
    /* keyword		keyword ID */
    { "assignment",	KEYWORD_assignment	},
    { "block",		KEYWORD_block		},
    { "common",		KEYWORD_common		},
    { "data",		KEYWORD_data		},
    { "end",		KEYWORD_end		},
    { "entry",		KEYWORD_entry		},
    { "function",	KEYWORD_function	},
    { "interface",	KEYWORD_interface	},
    { "module",		KEYWORD_module		},
    { "namelist",	KEYWORD_namelist	},
    { "operator",	KEYWORD_operator	},
    { "program",	KEYWORD_program		},
    { "subroutine",	KEYWORD_subroutine	},
    { "type",		KEYWORD_type		}
};

/*============================================================================
=   Function prototypes
============================================================================*/
static void buildFortranKeywordHash __ARGS((void));
static const char *tagName __ARGS((const tagType type));
static int tagLetter __ARGS((const tagType type));
static boolean includeTag __ARGS((const tagType type));
static void copyToken __ARGS((tokenInfo *const dest, const tokenInfo *const src));
static void makeFortranTag __ARGS((const tokenInfo *const token));
static int skipLine __ARGS((void));
static void makeLabelTag __ARGS((vString *const label));
static lineType getLineType __ARGS((void));
static int getFixedFormChar __ARGS((void));
static int getFreeFormChar __ARGS((void));
static int getChar __ARGS((void));
static void ungetChar __ARGS((const int c));
static vString *parseInteger __ARGS((int c));
static vString *parseNumeric __ARGS((int c));
static void parseString __ARGS((vString *const string, const int delimeter));
static void parseIdentifier __ARGS((vString *const string, const int firstChar));
static keywordId analyzeToken __ARGS((vString *const name));
static void checkForLabel __ARGS((void));
static void readToken __ARGS((tokenInfo *const token));
static void setTagType __ARGS((tokenInfo *const token, const tagType type));
static void nameTag __ARGS((tokenInfo *const token, const tagType type));
static void parseParenName __ARGS((tokenInfo *const token));
static void parseInterface __ARGS((tokenInfo *const token));
static void parseModule __ARGS((tokenInfo *const token));
static void tagSlashName __ARGS((tokenInfo *const token, const tagType type));
static void parseType __ARGS((tokenInfo *const token));
static void parseBlock __ARGS((tokenInfo *const token));
static void parseFile __ARGS((tokenInfo *const token));
static tokenInfo *newToken __ARGS((void));
static void deleteToken __ARGS((tokenInfo *const token));
static void init __ARGS((void));

/*============================================================================
=   Function definitions
============================================================================*/

static void buildFortranKeywordHash()
{
    const size_t count = sizeof(FortranKeywordTable) /
			 sizeof(FortranKeywordTable[0]);
    size_t i;

    for (i = 0  ;  i < count  ;  ++i)
    {
	const keywordDesc *p = &FortranKeywordTable[i];

	addKeyword(p->name, LANG_FORTRAN, (int)p->id);
    }
}

/*----------------------------------------------------------------------------
*-	Tag generation functions
----------------------------------------------------------------------------*/

static const char *tagName( type )
    const tagType type;
{
    /*  Note that the strings in this array must correspond to the types in
     *  the tagType enumeration.
     */
    static const char *names[] = {
	"undefined", "block data", "common", "entry", "function",
	"interface", "label", "module", "namelist", "program", "subroutine",
	"type", "variable"
    };

    Assert(sizeof(names)/sizeof(names[0]) == TAG_COUNT);

    return names[(int)type];
}

static int tagLetter( type )
    const tagType type;
{
    /*  Note that the characters in this list must correspond to the types in
     *  the tagType enumeration.
     */
    const char *const chars = "?bcefilmnpstv";

    Assert(strlen(chars) == TAG_COUNT);

    return chars[(int)type];
}

static boolean includeTag( type )
    const tagType type;
{
    const struct sFortranInclude *const inc = &Option.include.fortranTypes;
    boolean include;

    switch (type)
    {
	case TAG_BLOCK_DATA:	include = inc->blockData;	break;
	case TAG_COMMON_BLOCK:	include = inc->commonBlocks;	break;
	case TAG_ENTRY_POINT:	include = inc->entryPoints;	break;
	case TAG_FUNCTION:	include = inc->functions;	break;
	case TAG_INTERFACE:	include = inc->interfaces;	break;
	case TAG_MODULE:	include = inc->modules;		break;
	case TAG_NAMELIST:	include = inc->namelists;	break;
	case TAG_PROGRAM:	include = inc->programs;	break;
	case TAG_SUBROUTINE:	include = inc->subroutines;	break;
	case TAG_DERIVED_TYPE:	include = inc->types;		break;

	case TAG_LABEL:		include = (boolean)(inc->labels  &&
					   Option.include.fileScope); break;

	default:		include = FALSE;		break;
    }
    return include;
}

static void copyToken( dest, src )
    tokenInfo *const dest;
    const tokenInfo *const src;
{
    dest->type	  = src->type;
    dest->keyword = src->keyword;
    dest->tag     = src->tag;

    vStringCopy(dest->string, src->string);
}

static void makeFortranTag( token )
    const tokenInfo *const token;
{
    if (includeTag(token->tag))
    {
	const char *const name = vStringValue(token->string);
	const char *otherFields[2][1];
	tagEntryInfo e;

	initTagEntry(&e, name);

	if (token->tag == TAG_COMMON_BLOCK)
	    e.lineNumberEntry = (boolean)(Option.locate != EX_PATTERN);

	e.lineNumber	= token->lineNumber;
	e.filePosition	= token->filePosition;
	e.isFileScope	= (boolean)(token->tag == TAG_LABEL);
	e.kindName	= tagName(token->tag);
	e.kind		= tagLetter(token->tag);
	e.truncateLine	= (boolean)(token->tag != TAG_LABEL);

	switch (token->tag)
	{
	    case TAG_BLOCK_DATA:
	    case TAG_FUNCTION:
	    case TAG_PROGRAM:
	    case TAG_SUBROUTINE:
		copyToken(Parent, token);
		break;

	    case TAG_COMMON_BLOCK:
	    case TAG_VARIABLE:
	    case TAG_LABEL:
		if (Parent->tag != TAG_UNDEFINED)
		{
		    otherFields[0][0] = tagName(Parent->tag);
		    otherFields[1][0] = vStringValue(Parent->string);
		    e.otherFields.count = 1;
		    e.otherFields.label = otherFields[0];
		    e.otherFields.value = otherFields[1];
		}
		break;

	    default: break;
	}

	makeTagEntry(&e);
    }
}

/*----------------------------------------------------------------------------
*-	Parsing functions
----------------------------------------------------------------------------*/

static int skipLine()
{
    int c;

    do
	c = fileGetc();
    while (c != EOF  &&  c != '\n');

    return c;
}

static void makeLabelTag( label )
    vString *const label;
{
    tokenInfo token;

    token.type		= TOKEN_LABEL;
    token.keyword	= KEYWORD_NONE;
    token.tag		= TAG_LABEL;
    token.string	= label;
    token.lineNumber	= getFileLine();
    token.filePosition	= getFilePosition();

    makeFortranTag(&token);
}

static lineType getLineType()
{
    static vString *label = NULL;
    int column = 0;
    lineType type = LTYPE_UNDETERMINED;

    if (label == NULL)
	label = vStringNew();

    do		/* read in first 6 "margin" characters */
    {
	int c = fileGetc();

	/* 3.2.1  Comment_Line.  A comment line is any line that contains
	 * a C or an asterisk in column 1, or contains only blank characters
	 * in  columns 1 through 72.  A comment line that contains a C or
	 * an asterisk in column 1 may contain any character capable  of
	 * representation in the processor in columns 2 through 72.
	 */
	/*  EXCEPTION! Some compilers permit '!' as a commment character here.
	 *
	 *  Treat '#' in column 1 as comment to permit preprocessor directives.
	 */
	if (column == 0  &&  strchr("*Cc!#", c) != NULL)
	    type = LTYPE_COMMENT;
	else if (c == '\t')  /* EXCEPTION! Some compilers permit a tab here */
	{
	    column = 8;
	    type = LTYPE_INITIAL;
	}
	else if (column == 5)
	{
	    /* 3.2.2  Initial_Line.  An initial line is any line that is not
	     * a comment line and contains the character blank or the digit 0
	     * in column 6.  Columns 1 through 5 may contain a statement label
	     * (3.4), or each of the columns 1 through 5 must contain the
	     * character blank.
	     */
	    if (c == ' '  ||  c == '0')
		type = LTYPE_INITIAL;

	    /* 3.2.3  Continuation_Line.  A continuation line is any line that
	     * contains any character of the FORTRAN character set other than
	     * the character blank or the digit 0 in column 6 and contains
	     * only blank characters in columns 1 through 5.
	     */
	    else if (vStringLength(label) == 0)
		type = LTYPE_CONTINUATION;
	    else
		type = LTYPE_INVALID;
	}
	else if (c == ' ')
	    ;
	else if (c == EOF)
	    type = LTYPE_EOF;
	else if (c == '\n')
	    type = LTYPE_SHORT;
	else if (isdigit(c))
	    vStringPut(label, c);
	else
	    type = LTYPE_INVALID;

	++column;
    } while (column < 6  &&  type == LTYPE_UNDETERMINED);

    Assert(type != LTYPE_UNDETERMINED);

    if (vStringLength(label) > 0)
    {
	vStringTerminate(label);
	makeLabelTag(label);
	vStringClear(label);
    }
    return type;
}

static int getFixedFormChar()
{
    boolean newline = FALSE;
    lineType type;
    int c = '\0';

    if (Column > 0)
    {
#ifdef STRICT_FIXED_FORM
	/*  EXCEPTION! Some compilers permit more than 72 characters per line.
	 */
	if (Column > 71)
	    c = skipLine();
	else
#endif
	{
	    c = fileGetc();
	    ++Column;
	}
	if (c == '\n')
	{
	    newline = TRUE;	/* need to check for continuation line */
	    Column = 0;
	}
	else if (c == '&')	/* check for free source form */
	{
	    const int c2 = fileGetc();
	    if (c2 == '\n')
		longjmp(Exception, (int)ExceptionFixedFormat);
	    else
		fileUngetc(c2);
	}
    }
    while (Column == 0)
    {
	type = getLineType();
	switch (type)
	{
	    case LTYPE_UNDETERMINED:
	    case LTYPE_INVALID:
		longjmp(Exception, (int)ExceptionFixedFormat);
		break;

	    case LTYPE_SHORT: break;
	    case LTYPE_COMMENT: skipLine(); break;

	    case LTYPE_EOF:
		Column = 6;
		if (newline)
		    c = '\n';
		else
		    c = EOF;
		break;

	    case LTYPE_INITIAL:
		if (newline)
		{
		    c = '\n';
		    Column = 6;
		    break;
		}
		/* fall through to next case */
	    case LTYPE_CONTINUATION:
		Column = 5;
		do
		{
		    c = fileGetc();
		    ++Column;
		} while (isBlank(c));
		if (c == '\n')
		    Column = 0;
		else if (Column > 6)
		{
		    fileUngetc(c);
		    c = ' ';
		}
		break;

	    default:
		Assert("Unexpected line type" == NULL);
	}
    }
    return c;
}

static int getFreeFormChar()
{
    int c = fileGetc();

    if (c == '&')	/* handle line continuation */
    {
	c = fileGetc();
	if (c != '\n')
	    fileUngetc(c);
	else
	{
	    do
		c = fileGetc();
	    while (isBlank(c));
	    if (c == '&')
		c = fileGetc();
	}
    }
    return c;
}

static int getChar()
{
    int c;

    if (Ungetc != '\0')
    {
	c = Ungetc;
	Ungetc = '\0';
    }
    else if (FreeSourceForm)
	c = getFreeFormChar();
    else
	c = getFixedFormChar();

    return c;
}

static void ungetChar( c )
    const int c;
{
    Ungetc = c;
}

/*  If a numeric is passed in 'c', this is used as the first digit of the
 *  numeric being parsed.
 */
static vString *parseInteger( c )
    int c;
{
    static vString *string = NULL;

    if (string == NULL)
	string = vStringNew();
    vStringClear(string);

    if (c == '-')
    {
	vStringPut(string, c);
	c = getChar();
    }
    else if (! isdigit(c))
	c = getChar();
    while (c != EOF  &&  isdigit(c))
    {
	vStringPut(string, c);
	c = getChar();
    }
    vStringTerminate(string);

    if (c == '_')
    {
	do
	    c = getChar();
	while (c != EOF  &&  isalpha(c));
    }
    ungetChar(c);

    return string;
}

static vString *parseNumeric( c )
    int c;
{
    static vString *string = NULL;

    if (string == NULL)
	string = vStringNew();
    vStringCopy(string, parseInteger(c));

    c = getChar();
    if (c == '.')
    {
	vStringPut(string, c);
	vStringCat(string, parseInteger('\0'));
	c = getChar();
    }
    if (tolower(c) == 'e')
    {
	vStringPut(string, c);
	vStringCat(string, parseInteger('\0'));
    }
    else
	ungetChar(c);

    vStringTerminate(string);

    return string;
}

static void parseString( string, delimeter )
    vString *const string;
    const int delimeter;
{
    const unsigned long lineNumber = getFileLine();
    int c = getChar();

    while (c != delimeter  &&  c != '\n'  &&  c != EOF)
    {
	vStringPut(string, c);
	c = getChar();
    }
    if (c == '\n'  ||  c == EOF)
    {
	if (Option.verbose)
	    printf("%s: unterminated character string at line %lu\n",
		   getFileName(), lineNumber);
	if (c == EOF)
	    longjmp(Exception, (int)ExceptionEOF);
	else if (! FreeSourceForm)
	    longjmp(Exception, (int)ExceptionFixedFormat);
    }
    vStringTerminate(string);
}

/*  Read a C identifier beginning with "firstChar" and places it into "name".
 */
static void parseIdentifier( string, firstChar )
    vString *const string;
    const int firstChar;
{
    int c = firstChar;

    do
    {
	vStringPut(string, c);
	c = getChar();
    } while (isident(c));

    vStringTerminate(string);
    ungetChar(c);		/* unget non-identifier character */
}

/*  Analyzes the identifier contained in a statement described by the
 *  statement structure and adjusts the structure according the significance
 *  of the identifier.
 */
static keywordId analyzeToken( name )
    vString *const name;
{
    static vString *keyword = NULL;
    keywordId id;

    if (keyword == NULL)
	keyword = vStringNew();
    vStringCopyToLower(keyword, name);
    id = (keywordId)lookupKeyword(vStringValue(keyword), LANG_FORTRAN);

    return id;
}

static void checkForLabel()
{
    tokenInfo* token = NULL;
    int length;
    int c;

    do
	c = getChar();
    while (isBlank(c));

    for (length = 0  ;  isdigit(c)  &&  length < 5  ;  ++length)
    {
	if (token == NULL)
	{
	    token = newToken();
	    token->type = TOKEN_LABEL;
	    token->tag = TAG_LABEL;
	}
	vStringPut(token->string, c);
	c = getChar();
    }
    if (length > 0)
    {
	Assert(token != NULL);
	vStringTerminate(token->string);
	makeFortranTag(token);
	deleteToken(token);
    }
    ungetChar(c);
}

static void readToken( token )
    tokenInfo *const token;
{
    int c;

    token->type    = TOKEN_UNDEFINED;
    token->keyword = KEYWORD_NONE;
    vStringClear(token->string);

getNextChar:
    token->lineNumber	= getFileLine();
    token->filePosition	= getFilePosition();

    c = getChar();

    switch (c)
    {
	case EOF:  longjmp(Exception, (int)ExceptionEOF);	break;
	case ' ':  goto getNextChar;
	case '\t': goto getNextChar;
	case ',':  token->type = TOKEN_COMMA;			break;
	case '(':  token->type = TOKEN_PAREN_OPEN;		break;
	case ')':  token->type = TOKEN_PAREN_CLOSE;		break;

	case '*':
	case '/':
	case '+':
	case '-':
	case '=':
	case '<':
	case '>':
	{
	    const char *const operatorChars = "*/+-=<>";

	    do {
		vStringPut(token->string, c);
		c = getChar();
	    } while (strchr(operatorChars, c) != NULL);
	    ungetChar(c);
	    vStringTerminate(token->string);
	    token->type = TOKEN_OPERATOR;
	    break;
	}

	case '!':
	    skipLine();
	    Column = 0;
	    /* fall through to newline case */
	case '\n':
	    token->type = TOKEN_STATEMENT_END;
	    if (FreeSourceForm)
		checkForLabel();
	    break;

	case '.':
	    parseIdentifier(token->string, c);
	    c = getChar();
	    if (c == '.')
	    {
		vStringPut(token->string, c);
		vStringTerminate(token->string);
		token->type = TOKEN_OPERATOR;
	    }
	    else
	    {
		ungetChar(c);
		token->type = TOKEN_UNDEFINED;
	    }
	    break;

	case ':':
	    if (getChar() == ':')
		token->type = TOKEN_DECLARATION;
	    else
		token->type = TOKEN_UNDEFINED;
	    break;

	default:
	    if (isalpha(c))
	    {
		parseIdentifier(token->string, c);
		token->keyword = analyzeToken(token->string);
		if (isKeyword(token, KEYWORD_NONE))
		    token->type = TOKEN_IDENTIFIER;
		else
		    token->type = TOKEN_KEYWORD;
	    }
	    else if (isdigit(c))
	    {
		vStringCat(token->string, parseNumeric(c));
		token->type = TOKEN_NUMERIC;
	    }
	    else if (c == '"'  ||  c == '\'')
	    {
		parseString(token->string, c);
		token->type = TOKEN_STRING;
	    }
	    else if (c == ';'  &&  FreeSourceForm)
		token->type = TOKEN_STATEMENT_END;
	    else
		token->type = TOKEN_UNDEFINED;
	    break;
    }
}

/*----------------------------------------------------------------------------
*-	Scanning functions
----------------------------------------------------------------------------*/

#if 0
static void parseVariableDeclaration __ARGS((tokenInfo *const token));
static void parseVariableDeclaration( token )
    tokenInfo *const token;
{
    do
    {
	readToken(token);
	if (isType(token, TOKEN_IDENTIFIER))
	{
	    setTagType(token, TAG_VARIABLE);
	    makeFortranTag(token);
	    readToken(token);
	}
	while (! isType(token, TOKEN_COMMA)  &&  ! isType(token, TOKEN_NEWLINE))
	    readToken(token);
    } while (! isType(token, TOKEN_NEWLINE));
}
#endif

static void setTagType( token, type )
    tokenInfo *const token;
    const tagType type;
{
    token->tag = type;
}

static void nameTag( token, type )
    tokenInfo *const token;
    const tagType type;
{
    readToken(token);
    if (isType(token, TOKEN_IDENTIFIER))
    {
	setTagType(token, type);
	makeFortranTag(token);
    }
}

static void parseParenName( token )
    tokenInfo *const token;
{
    readToken(token);
    if (isType(token, TOKEN_PAREN_OPEN))
	readToken(token);
}

static void parseInterface( token )
    tokenInfo *const token;
{
    readToken(token);
    if (isType(token, TOKEN_IDENTIFIER))
    {
	setTagType(token, TAG_INTERFACE);
	makeFortranTag(token);
    }
    else if (isKeyword(token, KEYWORD_assignment) ||
	     isKeyword(token, KEYWORD_operator))
    {
	parseParenName(token);

	if (isType(token, TOKEN_OPERATOR))
	{
	    setTagType(token, TAG_INTERFACE);
	    makeFortranTag(token);
	}
    }
}

static void parseModule( token )
    tokenInfo *const token;
{
    readToken(token);
    if (isType(token, TOKEN_IDENTIFIER))
    {
	vString *moduleName = vStringNew();
	vStringCopyToLower(moduleName, token->string);
	if (strcmp(vStringValue(moduleName), "procedure") != 0)
	{
	    setTagType(token, TAG_MODULE);
	    makeFortranTag(token);
	}
    }
}

static void tagSlashName( token, type )
    tokenInfo *const token;
    const tagType type;
{
    readToken(token);
    if (isType(token, TOKEN_OPERATOR)  &&
	strcmp(vStringValue(token->string), "/") == 0)
    {
	readToken(token);
	if (isType(token, TOKEN_IDENTIFIER))
	{
	    setTagType(token, type);
	    makeFortranTag(token);
	}
    }
}

static void parseType( token )
    tokenInfo *const token;
{
    readToken(token);
    if (isType(token, TOKEN_PAREN_OPEN))
	;
    else
    {
	if (isType(token, TOKEN_COMMA))
	{
	    while (! isType(token, TOKEN_DECLARATION))
		readToken(token);
	    readToken(token);
	}
	if (isType(token, TOKEN_IDENTIFIER))
	{
	    setTagType(token, TAG_DERIVED_TYPE);
	    makeFortranTag(token);
	}
    }
}

static void parseBlock( token )
    tokenInfo *const token;
{
    readToken(token);
    if (isKeyword(token, KEYWORD_data))
	nameTag(token, TAG_BLOCK_DATA);
}

static void parseFile( token )
    tokenInfo *const token;
{
    do
    {
	readToken(token);
	switch (token->keyword)
	{
	case KEYWORD_block:	parseBlock(token);			break;
	case KEYWORD_common:	tagSlashName(token,TAG_COMMON_BLOCK);	break;
	case KEYWORD_end:	readToken(token);			break;
	case KEYWORD_entry:	nameTag(token, TAG_ENTRY_POINT);	break;
	case KEYWORD_function:  nameTag(token, TAG_FUNCTION);		break;
	case KEYWORD_interface:	parseInterface(token);			break;
	case KEYWORD_module:    parseModule(token);			break;
	case KEYWORD_namelist:	tagSlashName(token, TAG_NAMELIST);	break;
	case KEYWORD_program:   nameTag(token, TAG_PROGRAM);		break;
	case KEYWORD_subroutine:nameTag(token, TAG_SUBROUTINE);		break;
	case KEYWORD_type:	parseType(token);			break;

	default: break;
	}
    } while (TRUE);
}

static tokenInfo *newToken()
{
    tokenInfo *const token = (tokenInfo *)eMalloc(sizeof(tokenInfo));

    token->type	   = TOKEN_UNDEFINED;
    token->keyword = KEYWORD_NONE;
    token->tag	   = TAG_UNDEFINED;
    token->string  = vStringNew();
    token->lineNumber   = getFileLine();
    token->filePosition	= getFilePosition();

    return token;
}

static void deleteToken ( token )
    tokenInfo *const token;
{
    vStringDelete(token->string);
    eFree(token);
}

static void init()
{
    static boolean isInitialized = FALSE;

    if (! isInitialized)
    {
	buildFortranKeywordHash();
	isInitialized = TRUE;
    }
}

extern boolean createFortranTags( passCount )
    const unsigned int passCount;
{
    tokenInfo *token;
    exception_t exception;
    boolean retry;

    Assert(passCount < 3);
    init();
    Parent = newToken();
    token = newToken();
    FreeSourceForm = (boolean)(passCount > 1);
    Column = 0;
    exception = (exception_t)setjmp(Exception);
    if (exception == ExceptionEOF)
	retry = FALSE;
    else if (exception == ExceptionFixedFormat  &&  ! FreeSourceForm)
    {
	if (Option.verbose)
	    printf("%s: not fixed source form; retry as free source form\n",
		   getFileName());
	retry = TRUE;
    }
    else
    {
	parseFile(token);
	retry = FALSE;
    }
    deleteToken(token);
    deleteToken(Parent);

    return retry;
}

/* vi:set tabstop=8 shiftwidth=4: */
