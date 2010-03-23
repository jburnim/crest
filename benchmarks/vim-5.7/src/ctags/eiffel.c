/*****************************************************************************
*   $Id: eiffel.c,v 8.8 2000/03/27 05:03:23 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for Eiffel language
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
#include "parse.h"
#include "entry.h"
#include "keyword.h"
#include "main.h"
#include "options.h"
#include "read.h"
#include "vstring.h"

/*============================================================================
=   Macros
============================================================================*/
#define isFreeOperatorChar(c)	((c) == '@' || (c) == '#' || \
				 (c) == '|' || (c) == '&')
#define isType(token,t)		(boolean)((token)->type == (t))
#define isKeyword(token,k)	(boolean)((token)->keyword == (k))

/*============================================================================
=   Data declarations
============================================================================*/

typedef enum eException { ExceptionNone, ExceptionEOF } exception_t;

/*  Used to specify type of keyword.
 */
typedef enum eKeywordId {
    KEYWORD_NONE,
    KEYWORD_alias, KEYWORD_all, KEYWORD_and, KEYWORD_as, KEYWORD_check,
    KEYWORD_class, KEYWORD_create, KEYWORD_creation, KEYWORD_Current,
    KEYWORD_debug, KEYWORD_deferred, KEYWORD_do, KEYWORD_else,
    KEYWORD_elseif, KEYWORD_end, KEYWORD_ensure, KEYWORD_expanded,
    KEYWORD_export, KEYWORD_external, KEYWORD_false, KEYWORD_feature,
    KEYWORD_from, KEYWORD_frozen, KEYWORD_if, KEYWORD_implies,
    KEYWORD_indexing, KEYWORD_infix, KEYWORD_inherit, KEYWORD_inspect,
    KEYWORD_invariant, KEYWORD_is, KEYWORD_like, KEYWORD_local,
    KEYWORD_loop, KEYWORD_not, KEYWORD_obsolete, KEYWORD_old, KEYWORD_once,
    KEYWORD_or, KEYWORD_prefix, KEYWORD_redefine, KEYWORD_rename,
    KEYWORD_require, KEYWORD_rescue, KEYWORD_Result, KEYWORD_retry,
    KEYWORD_select, KEYWORD_separate, KEYWORD_strip, KEYWORD_then,
    KEYWORD_true, KEYWORD_undefine, KEYWORD_unique, KEYWORD_until,
    KEYWORD_variant, KEYWORD_when, KEYWORD_xor
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
    TOKEN_BANG,
    TOKEN_CHARACTER,
    TOKEN_CLOSE_BRACE,
    TOKEN_CLOSE_BRACKET,
    TOKEN_CLOSE_PAREN,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_DOT,
    TOKEN_DOLLAR,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMERIC,
    TOKEN_OPEN_BRACE,
    TOKEN_OPEN_BRACKET,
    TOKEN_OPEN_PAREN,
    TOKEN_OPERATOR,
    TOKEN_OTHER,
    TOKEN_SEPARATOR,
    TOKEN_STRING
} tokenType;

typedef enum eTagType {
    TAG_UNDEFINED,
    TAG_CLASS,
    TAG_FEATURE,
    TAG_QUALIFIED_FEATURE,
    TAG_LOCAL,
    TAG_COUNT		/* must be last */
} tagType;

typedef struct sTokenInfo {
    tokenType	type;
    keywordId	keyword;
    boolean	isExported;
    vString *	string;
    vString *	className;
    vString *	featureName;
} tokenInfo;

/*============================================================================
=   Data definitions
============================================================================*/

static jmp_buf Exception;

static const keywordDesc EiffelKeywordTable[] = {
    /* keyword		keyword ID */
    { "alias",		KEYWORD_alias		},
    { "all",		KEYWORD_all		},
    { "and",		KEYWORD_and		},
    { "as",		KEYWORD_as		},
    { "check",		KEYWORD_check		},
    { "class",		KEYWORD_class		},
    { "create",		KEYWORD_create		},
    { "creation",	KEYWORD_creation	},
    { "current",	KEYWORD_Current		},
    { "debug",		KEYWORD_debug		},
    { "deferred",	KEYWORD_deferred	},
    { "do",		KEYWORD_do		},
    { "else",		KEYWORD_else		},
    { "elseif",		KEYWORD_elseif		},
    { "end",		KEYWORD_end		},
    { "ensure",		KEYWORD_ensure		},
    { "expanded",	KEYWORD_expanded	},
    { "export",		KEYWORD_export		},
    { "external",	KEYWORD_external	},
    { "false",		KEYWORD_false		},
    { "feature",	KEYWORD_feature		},
    { "from",		KEYWORD_from		},
    { "frozen",		KEYWORD_frozen		},
    { "if",		KEYWORD_if		},
    { "implies",	KEYWORD_implies		},
    { "indexing",	KEYWORD_indexing	},
    { "infix",		KEYWORD_infix		},
    { "inherit",	KEYWORD_inherit		},
    { "inspect",	KEYWORD_inspect		},
    { "invariant",	KEYWORD_invariant	},
    { "is",		KEYWORD_is		},
    { "like",		KEYWORD_like		},
    { "local",		KEYWORD_local		},
    { "loop",		KEYWORD_loop		},
    { "not",		KEYWORD_not		},
    { "obsolete",	KEYWORD_obsolete	},
    { "old",		KEYWORD_old		},
    { "once",		KEYWORD_once		},
    { "or",		KEYWORD_or		},
    { "prefix",		KEYWORD_prefix		},
    { "redefine",	KEYWORD_redefine	},
    { "rename",		KEYWORD_rename		},
    { "require",	KEYWORD_require		},
    { "rescue",		KEYWORD_rescue		},
    { "result",		KEYWORD_Result		},
    { "retry",		KEYWORD_retry		},
    { "select",		KEYWORD_select		},
    { "separate",	KEYWORD_separate	},
    { "strip",		KEYWORD_strip		},
    { "then",		KEYWORD_then		},
    { "true",		KEYWORD_true		},
    { "undefine",	KEYWORD_undefine	},
    { "unique",		KEYWORD_unique		},
    { "until",		KEYWORD_until		},
    { "variant",	KEYWORD_variant		},
    { "when",		KEYWORD_when		},
    { "xor",		KEYWORD_xor		}
};

/*============================================================================
=   Function prototypes
============================================================================*/
static void buildEiffelKeywordHash __ARGS((void));
static const char *tagName __ARGS((const tagType type));
static int tagLetter __ARGS((const tagType type));
static boolean includeTag __ARGS((const tagType type, const boolean fileScope));
static void qualifyClassTag __ARGS((tokenInfo *const token));
static void qualifyFeatureTag __ARGS((tokenInfo *const token));
static void qualifyLocalTag __ARGS((tokenInfo *const token));
static int skipToCharacter __ARGS((const int c));
static vString *parseInteger __ARGS((int c));
static vString *parseNumeric __ARGS((int c));
static int parseEscapedCharacter __ARGS((void));
static int parseCharacter __ARGS((void));
static void parseString __ARGS((vString *const string));
static void parseIdentifier __ARGS((vString *const string, const int firstChar));
static void parseFreeOperator __ARGS((vString *const string, const int firstChar));
static keywordId analyzeToken __ARGS((vString *const name));
static void readToken __ARGS((tokenInfo *const token));
static boolean isIdentifierMatch __ARGS((const tokenInfo *const token, const char *const name));
static void locateToken __ARGS((tokenInfo *const token, const tokenType type));
static void skipPastMatchingBracket __ARGS((tokenInfo *const token));
static void skipEntityType __ARGS((tokenInfo *const token));
static void parseLocal __ARGS((tokenInfo *const token));
static void findFeatureEnd __ARGS((tokenInfo *const token));
static boolean readFeatureName __ARGS((tokenInfo *const token));
static void parseFeature __ARGS((tokenInfo *const token));
static void parseExport __ARGS((tokenInfo *const token));
static void locateKeyword __ARGS((tokenInfo *const token, const keywordId keyword));
static void parseFeatureClauses __ARGS((tokenInfo *const token));
static void parseRename __ARGS((tokenInfo *const token));
static void parseInherit __ARGS((tokenInfo *const token));
static void parseClass __ARGS((tokenInfo *const token));
static tokenInfo *newToken __ARGS((void));
static void deleteToken __ARGS((tokenInfo *const token));
static void init __ARGS((void));

/*============================================================================
=   Function definitions
============================================================================*/

static void buildEiffelKeywordHash()
{
    const size_t count = sizeof(EiffelKeywordTable) /
			 sizeof(EiffelKeywordTable[0]);
    size_t i;

    for (i = 0  ;  i < count  ;  ++i)
    {
	const keywordDesc *p = &EiffelKeywordTable[i];

	addKeyword(p->name, LANG_EIFFEL, (int)p->id);
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
	"undefined", "class", "feature", "feature", "local" };

    Assert(sizeof(names)/sizeof(names[0]) == TAG_COUNT);

    return names[(int)type];
}

static int tagLetter( type )
    const tagType type;
{
    /*  Note that the characters in this list must correspond to the types in
     *  the tagType enumeration.
     */
    const char *const chars = "?cffl";

    Assert(strlen(chars) == TAG_COUNT);

    return chars[(int)type];
}

static boolean includeTag( type, fileScope )
    const tagType type;
    const boolean fileScope;
{
    const struct sEiffelInclude *const inc = &Option.include.eiffelTypes;
    boolean include = FALSE;

    if (fileScope  &&  ! Option.include.fileScope)
	include = FALSE;
    else switch (type)
    {
	case TAG_CLASS:			include = inc->classNames;	break;
	case TAG_FEATURE:		include = inc->features;	break;
	case TAG_QUALIFIED_FEATURE:	include = inc->classPrefix;	break;
	case TAG_LOCAL:			include = inc->localEntities;	break;

	default: Assert("Bad type in Eiffel file" == NULL); break;
    }
    return include;
}

static void qualifyClassTag( token )
    tokenInfo *const token;
{
    if (includeTag(TAG_CLASS, FALSE))
    {
	const char *const name = vStringValue(token->string);
	tagEntryInfo e;

	initTagEntry(&e, name);

	e.kindName = tagName(TAG_CLASS);
	e.kind     = tagLetter(TAG_CLASS);

	makeTagEntry(&e);
    }
    vStringCopy(token->className, token->string);
}

static void qualifyFeatureTag( token )
    tokenInfo *const token;
{
    const boolean isFileScope = (boolean)(! token->isExported);

    if (includeTag(TAG_FEATURE, isFileScope))
    {
	const char *const name = vStringValue(token->string);
	const char *otherFields[2][1];
	tagEntryInfo e;

	otherFields[0][0] = tagName(TAG_CLASS);
	otherFields[1][0] = vStringValue(token->className);

	initTagEntry(&e, name);

	e.isFileScope		= isFileScope;
	e.kindName		= tagName(TAG_FEATURE);
	e.kind			= tagLetter(TAG_FEATURE);
	e.otherFields.count	= 1;
	e.otherFields.label	= otherFields[0];
	e.otherFields.value	= otherFields[1];

	makeTagEntry(&e);

	if (includeTag(TAG_QUALIFIED_FEATURE, isFileScope))
	{
	    vString* qualified = vStringNewInit(vStringValue(token->className));
	    vStringPut(qualified, '.');
	    vStringCat(qualified, token->string);
	    e.name = vStringValue(qualified);
	    makeTagEntry(&e);
	    vStringDelete(qualified);
	}
    }
    vStringCopy(token->featureName, token->string);
}

static void qualifyLocalTag( token )
    tokenInfo *const token;
{
    if (includeTag(TAG_LOCAL, TRUE))
    {
	const char *const name = vStringValue(token->string);
	const char *otherFields[2][2];
	tagEntryInfo e;

	otherFields[0][0] = tagName(TAG_FEATURE);
	otherFields[1][0] = vStringValue(token->featureName);

	otherFields[0][1] = tagName(TAG_CLASS);
	otherFields[1][1] = vStringValue(token->className);

	initTagEntry(&e, name);

	e.isFileScope		= TRUE;
	e.kindName		= tagName(TAG_LOCAL);
	e.kind			= tagLetter(TAG_LOCAL);
	e.otherFields.count	= 2;
	e.otherFields.label	= otherFields[0];
	e.otherFields.value	= otherFields[1];

	makeTagEntry(&e);
    }
}

/*----------------------------------------------------------------------------
*-	Parsing functions
----------------------------------------------------------------------------*/

static int skipToCharacter( c )
    const int c;
{
    int d;

    do
    {
	d = fileGetc();
    } while (d != EOF  &&  d != c);

    return d;
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
	c = fileGetc();
    }
    else if (! isdigit(c))
	c = fileGetc();
    while (c != EOF  &&  (isdigit(c)  ||  c == '_'))
    {
	vStringPut(string, c);
	c = fileGetc();
    }
    vStringTerminate(string);
    fileUngetc(c);

    return string;
}

static vString *parseNumeric( c )
    int c;
{
    static vString *string = NULL;

    if (string == NULL)
	string = vStringNew();
    vStringCopy(string, parseInteger(c));

    c = fileGetc();
    if (c == '.')
    {
	vStringPut(string, c);
	vStringCat(string, parseInteger('\0'));
	c = fileGetc();
    }
    if (tolower(c) == 'e')
    {
	vStringPut(string, c);
	vStringCat(string, parseInteger('\0'));
    }
    else
	fileUngetc(c);

    vStringTerminate(string);

    return string;
}

static int parseEscapedCharacter()
{
    int d = '\0';
    int c = fileGetc();

    switch (c)
    {
	case 'A':	d = '@';	break;
	case 'B':	d = '\b';	break;
	case 'C':	d = '^';	break;
	case 'D':	d = '$';	break;
	case 'F':	d = '\f';	break;
	case 'H':	d = '\\';	break;
	case 'L':	d = '~';	break;
	case 'N':	d = '\n';	break;
#ifdef QDOS
	case 'Q':	d = 0x9F;	break;
#else
	case 'Q':	d = '`';	break;
#endif
	case 'R':	d = '\r';	break;
	case 'S':	d = '#';	break;
	case 'T':	d = '\t';	break;
	case 'U':	d = '\0';	break;
	case 'V':	d = '|';	break;
	case '%':	d = '%';	break;
	case '\'':	d = '\'';	break;
	case '"':	d = '"';	break;
	case '(':	d = '[';	break;
	case ')':	d = ']';	break;
	case '<':	d = '{';	break;
	case '>':	d = '}';	break;

	case '\n': skipToCharacter('%'); break;

	case '/':
	{
	    vString *string = parseInteger('\0');
	    const char *value = vStringValue(string);
	    const unsigned long ascii = atol(value);

	    c = fileGetc();
	    if (c == '/'  &&  ascii < 256)
		d = ascii;
	    break;
	}

	default: break;
    }
    return d;
}

static int parseCharacter()
{
    int c = fileGetc();
    int result = c;

    if (c == '%')
	result = parseEscapedCharacter();

    c = fileGetc();
    if (c != '\'')
	skipToCharacter('\n');

    return result;
}

static void parseString( string )
    vString *const string;
{
    int c = fileGetc();

    while (c != EOF  &&  c != '"')
    {
	if (c == '%')
	    c = parseEscapedCharacter();
	vStringPut(string, c);
	c = fileGetc();
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
	c = fileGetc();
    } while (isident(c));

    vStringTerminate(string);
    fileUngetc(c);		/* unget non-identifier character */
}

static void parseFreeOperator( string, firstChar )
    vString *const string;
    const int firstChar;
{
    int c = firstChar;

    do
    {
	vStringPut(string, c);
	c = fileGetc();
    } while (c > ' ');

    vStringTerminate(string);
    fileUngetc(c);		/* unget non-identifier character */
}

static keywordId analyzeToken( name )
    vString *const name;
{
    static vString *keyword = NULL;
    keywordId id;

    if (keyword == NULL)
	keyword = vStringNew();
    vStringCopyToLower(keyword, name);
    id = (keywordId)lookupKeyword(vStringValue(keyword), LANG_EIFFEL);

    return id;
}

static void readToken( token )
    tokenInfo *const token;
{
    int c;

    token->type    = TOKEN_UNDEFINED;
    token->keyword = KEYWORD_NONE;
    vStringClear(token->string);

getNextChar:

    do
	c = fileGetc();
    while (c == '\t'  ||  c == ' '  ||  c == '\n');

    switch (c)
    {
	case EOF:	longjmp(Exception, (int)ExceptionEOF);	break;
	case '!':	token->type = TOKEN_BANG;		break;
	case '$':	token->type = TOKEN_DOLLAR;		break;
	case '(':	token->type = TOKEN_OPEN_PAREN;		break;
	case ')':	token->type = TOKEN_CLOSE_PAREN;	break;
	case ',':	token->type = TOKEN_COMMA;		break;
	case '.':	token->type = TOKEN_DOT;		break;
	case ';':	goto getNextChar;
	case '[':	token->type = TOKEN_OPEN_BRACKET;	break;
	case ']':	token->type = TOKEN_CLOSE_BRACKET;	break;
	case '{':	token->type = TOKEN_OPEN_BRACE;		break;
	case '}':	token->type = TOKEN_CLOSE_BRACE;	break;

	case '+':
	case '*':
	case '^':
	case '=':	token->type = TOKEN_OPERATOR;		break;

	case '-':
	    c = fileGetc();
	    if (c != '-')		/* is this the start of a comment? */
		fileUngetc(c);
	    else
	    {
		skipToCharacter('\n');
		goto getNextChar;
	    }
	    break;

	case '?':
	case ':':
	    c = fileGetc();
	    if (c == '=')
		token->type = TOKEN_OPERATOR;
	    else
	    {
		token->type = TOKEN_COLON;
		fileUngetc(c);
	    }
	    break;

	case '<':
	    c = fileGetc();
	    if (c != '='  &&  c != '>')
		fileUngetc(c);
	    token->type = TOKEN_OPERATOR;
	    break;

	case '>':
	    c = fileGetc();
	    if (c != '='  &&  c != '>')
		fileUngetc(c);
	    token->type = TOKEN_OPERATOR;
	    break;

	case '/':
	    c = fileGetc();
	    if (c != '/'  &&  c != '=')
		fileUngetc(c);
	    token->type = TOKEN_OPERATOR;
	    break;

	case '\\':
	    c = fileGetc();
	    if (c != '\\')
		fileUngetc(c);
	    token->type = TOKEN_OPERATOR;
	    break;

	case '"':
	    token->type = TOKEN_STRING;
	    parseString(token->string);
	    break;

	case '\'':
	    token->type = TOKEN_CHARACTER;
	    parseCharacter();
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
	    else if (isFreeOperatorChar(c))
	    {
		parseFreeOperator(token->string, c);
		token->type = TOKEN_OPERATOR;
	    }
	    else
	    {
		token->type = TOKEN_UNDEFINED;
		Assert(! isType(token, TOKEN_UNDEFINED));
	    }
	    break;
    }
}

/*----------------------------------------------------------------------------
*-	Scanning functions
----------------------------------------------------------------------------*/

static boolean isIdentifierMatch( token, name )
    const tokenInfo *const token;
    const char *const name;
{
    return (boolean)(isType(token, TOKEN_IDENTIFIER)  &&
		     strequiv(vStringValue(token->string), name));
}

static void locateToken( token, type )
    tokenInfo *const token;
    const tokenType type;
{
    do
	readToken(token);
    while (token->type != type);

    readToken(token);
}

static void skipPastMatchingBracket( token )
    tokenInfo *const token;
{
    int depth = 0;

    do
    {
	if (isType(token, TOKEN_OPEN_BRACKET))
	    ++depth;
	else if (isType(token, TOKEN_CLOSE_BRACKET))
	    --depth;
	readToken(token);
    } while (depth > 0);
}

static void skipEntityType( token )
    tokenInfo *const token;
{
    /*  When called, we are at the ':'. Move to the type.
     */
    readToken(token);

    if (isKeyword(token, KEYWORD_expanded))
	readToken(token);

    /*  Skip over the type name, with possible generic parameters.
     */
    if (isKeyword(token, KEYWORD_like)  ||
	strequiv(vStringValue(token->string), "BIT"))
    {
	readToken(token);
    }
    readToken(token);
    if (isType(token, TOKEN_OPEN_BRACKET))
	skipPastMatchingBracket(token);
}

static void parseLocal( token )
    tokenInfo *const token;
{
    readToken(token);	    /* skip past "local" keyword */

    /*  Check keyword first in case local clause is empty
     */
    while (! isKeyword(token, KEYWORD_do)  &&
	   ! isKeyword(token, KEYWORD_once))
    {
	if (isType(token, TOKEN_IDENTIFIER))
	    qualifyLocalTag (token);

	readToken(token);
	if (isType(token, TOKEN_COLON))
	    skipEntityType(token);
    }
}

static void findFeatureEnd( token )
    tokenInfo *const token;
{
    readToken(token);

    switch (token->keyword)
    {
	default:
	    break;

	case KEYWORD_deferred:
	case KEYWORD_do:
	case KEYWORD_external:
	case KEYWORD_local:
	case KEYWORD_obsolete:
	case KEYWORD_once:
	case KEYWORD_require:
	{
	    int depth = 1;

	    while (depth > 0)
	    {
		switch (token->keyword)
		{
		    case KEYWORD_check:
		    case KEYWORD_debug:
		    case KEYWORD_from:
		    case KEYWORD_if:
		    case KEYWORD_inspect:
			++depth;
			break;

		    case KEYWORD_local:
			parseLocal(token);
			break;

		    case KEYWORD_end:
			--depth;
			break;

		    default:
			break;
		}
		readToken(token);
	    }
	    break;
	}
    }
}

static boolean readFeatureName( token )
    tokenInfo *const token;
{
    boolean isFeatureName = FALSE;

    if (isType(token, TOKEN_IDENTIFIER))
	isFeatureName = TRUE;
    else if (isKeyword(token, KEYWORD_infix)  ||
	    isKeyword(token, KEYWORD_prefix))
    {
	readToken(token);
	if (isType(token, TOKEN_STRING))
	    isFeatureName = TRUE;
    }
    return isFeatureName;
}

static void parseFeature( token )
    tokenInfo *const token;
{
    do
    {
	if (readFeatureName(token))
	    qualifyFeatureTag(token);
	readToken(token);
    } while (isType(token, TOKEN_COMMA));


    /*  Skip signature.
     */
    if (isType(token, TOKEN_OPEN_PAREN))	/* arguments? */
	locateToken(token, TOKEN_CLOSE_PAREN);
    if (isType(token, TOKEN_COLON))		/* a query? */
	skipEntityType(token);

    if (isKeyword(token, KEYWORD_is))
	findFeatureEnd(token);
}

static void parseExport( token )
    tokenInfo *const token;
{
    token->isExported = TRUE;

    readToken(token);
    if (isType(token, TOKEN_OPEN_BRACE)) do
    {
	readToken(token);
	if (isType(token, TOKEN_CLOSE_BRACE))
	{
	    token->isExported = FALSE;
	    readToken(token);
	}
	else
	{
	    if (isIdentifierMatch(token, "NONE"))
		token->isExported = FALSE;
	    locateToken(token, TOKEN_CLOSE_BRACE);
	}
    } while (isType(token, TOKEN_COMMA));
}

static void locateKeyword( token, keyword )
    tokenInfo *const token;
    const keywordId keyword;
{
    do
	readToken(token);
    while (! isKeyword(token, keyword));

    readToken(token);
}

static void parseFeatureClauses( token )
    tokenInfo *const token;
{
    do
    {
	while (isKeyword(token, KEYWORD_feature))
	    parseExport(token);
	parseFeature(token);
    } while (! isKeyword(token, KEYWORD_end) &&
	     ! isKeyword(token, KEYWORD_invariant));
}

static void parseRename( token )
    tokenInfo *const token;
{
    do {
	readToken(token);
	if (readFeatureName(token))
	{
	    readToken(token);
	    if (isKeyword(token, KEYWORD_as))
	    {
		readToken(token);
		if (readFeatureName(token))
		{
		    qualifyFeatureTag(token);	    /* renamed feature */
		    readToken(token);
		}
	    }
	}
    } while (isType(token, TOKEN_COMMA));

    if (! isKeyword(token, KEYWORD_end))
	locateKeyword(token, KEYWORD_end);
}

static void parseInherit( token )
    tokenInfo *const token;
{
    do
    {
	readToken(token);
	switch (token->keyword)	    /* check for feature adaptation */
	{
	    case KEYWORD_rename:
		parseRename(token);
		if (isKeyword(token, KEYWORD_end))
		    readToken(token);
		break;

	    case KEYWORD_export:
	    case KEYWORD_undefine:
	    case KEYWORD_redefine:
	    case KEYWORD_select:
		locateKeyword(token, KEYWORD_end);
		break;

	    case KEYWORD_end:
		readToken(token);
		break;

	    default: break;
	}
    } while (! isType(token, TOKEN_KEYWORD));
}

static void parseClass( token )
    tokenInfo *const token;
{
    if (isType(token, TOKEN_IDENTIFIER))
    {
	qualifyClassTag(token);
	readToken(token);
    }

    do
    {
	switch (token->keyword)
	{
	    case KEYWORD_inherit:  parseInherit(token);        break;
	    case KEYWORD_feature:  parseFeatureClauses(token); break;
	    default:               readToken(token);           break;
	}
    } while (! isKeyword(token, KEYWORD_end));
}

static tokenInfo *newToken()
{
    tokenInfo *const token = (tokenInfo *)eMalloc(sizeof(tokenInfo));

    token->type		= TOKEN_UNDEFINED;
    token->keyword	= KEYWORD_NONE;
    token->isExported	= TRUE;

    token->string = vStringNew();
    token->className = vStringNew();
    token->featureName = vStringNew();

    return token;
}

static void deleteToken ( token )
    tokenInfo *const token;
{
    vStringDelete(token->string);
    vStringDelete(token->className);
    vStringDelete(token->featureName);

    eFree(token);
}

static void init()
{
    static boolean isInitialized = FALSE;

    if (! isInitialized)
    {
	buildEiffelKeywordHash();
	isInitialized = TRUE;
    }
}

extern boolean createEiffelTags()
{
    tokenInfo *const token = newToken();
    exception_t exception;

    init();
    exception = (exception_t)(setjmp(Exception));
    while (exception == ExceptionNone)
    {
	locateKeyword(token, KEYWORD_class);
	parseClass(token);
    }
    deleteToken(token);

    return FALSE;
}

/* vi:set tabstop=8 shiftwidth=4: */
