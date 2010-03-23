/*****************************************************************************
*   $Id: keyword.c,v 8.2 1999/09/29 02:20:25 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   Manages a keyword hash.
*****************************************************************************/

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

#include <string.h>

#include "debug.h"
#include "keyword.h"
#include "main.h"
#include "options.h"

/*============================================================================
=   Macros
============================================================================*/
#define HASH_EXPONENT	7	/* must be less than 17 */

/*============================================================================
=   Data declarations
============================================================================*/
typedef struct sHashEntry {
    const char *string;
    struct sHashEntry *next;
    int value[LANG_COUNT];
} hashEntry;

/*============================================================================
=   Data definitions
============================================================================*/
const unsigned int TableSize = 1 << HASH_EXPONENT;
static hashEntry **HashTable = NULL;

/*============================================================================
=   Function prototypes
============================================================================*/
static hashEntry **getHashTable __ARGS((void));
static hashEntry *getHashTableEntry __ARGS((unsigned int hashedValue));
static unsigned int hashValue __ARGS((const char *const string));
static hashEntry *newEntry __ARGS((const char *const string, langType language, int value));

/*============================================================================
=   Function definitions
============================================================================*/

static hashEntry **getHashTable()
{
    static boolean allocated = FALSE;

    if (! allocated)
    {
	unsigned int i;

	HashTable =(hashEntry**)eMalloc((size_t)TableSize * sizeof(hashEntry*));

	for (i = 0  ;  i < TableSize  ;  ++i)
	    HashTable[i] = NULL;

	allocated = TRUE;
    }
    return HashTable;
}

static hashEntry *getHashTableEntry( hashedValue )
    unsigned int hashedValue;
{
    hashEntry **const table = getHashTable();
    hashEntry *entry;

    Assert(hashedValue < TableSize);
    entry = table[hashedValue];

    return entry;
}

static unsigned int hashValue( string )
    const char *const string;
{
    unsigned long value = 0;
    const unsigned char *p;

    Assert(string != NULL);

    /*  We combine the various words of the multiword key using the method
     *  described on page 512 of Vol. 3 of "The Art of Computer Programming".
     */
    for (p = (const unsigned char *)string  ;  *p != '\0'  ;  ++p)
    {
	value <<= 1;
	if (value & 0x00000100L)
	    value = (value & 0x000000ffL) + 1L;
	value ^= *p;
    }
    /*  Algorithm from page 509 of Vol. 3 of "The Art of Computer Programming"
     *  Treats "value" as a 16-bit integer plus 16-bit fraction.
     */
    value *= 40503L;		/* = 2^16 * 0.6180339887 ("golden ratio") */
    value &= 0x0000ffffL;	/* keep fractional part */
    value >>= 16 - HASH_EXPONENT; /* scale up by hash size and move down */

    return value;
}

static hashEntry *newEntry( string, language, value )
    const char *const string;
    langType language;
    int value;
{
    hashEntry *const entry = (hashEntry *)eMalloc(sizeof(hashEntry));
    unsigned int i;

    entry->string = string;
    entry->next = NULL;
    for (i = 0  ;  i < LANG_COUNT  ;  ++i)
	entry->value[i] = 0;

    entry->value[(int)language] = value;

    return entry;
}

/*  Note that it is assumed that a "value" of zero means an undefined keyword
 *  and clients of this function should observe this. Also, all keywords added
 *  should be added in lower case. If we encounter a case-sensitive language
 *  whose keywords are in upper case, we will need to redesign this.
 */
extern void addKeyword( string, language, value )
    const char *const string;
    langType language;
    int value;
{
    const unsigned int hashedValue = hashValue(string);
    hashEntry *tableEntry = getHashTableEntry(hashedValue);
    hashEntry *entry = tableEntry;

    if (entry == NULL)
    {
	hashEntry **const table = getHashTable();
	hashEntry *new = newEntry(string, language, value);

	table[hashedValue] = new;
    }
    else
    {
	hashEntry *prev = NULL;

	while (entry != NULL)
	{
	    if (strcmp(string, entry->string) == 0)
	    {
		entry->value[language] = value;
		break;
	    }
	    prev = entry;
	    entry = entry->next;
	}
	if (entry == NULL)
	{
	    hashEntry *new = newEntry(string, language, value);

	    Assert(prev != NULL);
	    prev->next = new;
	}
    }
}

extern int lookupKeyword( string, language )
    const char *const string;
    langType language;
{
    const unsigned int hashedValue = hashValue(string);
    hashEntry *entry = getHashTableEntry(hashedValue);
    int value = 0;

    Assert(language < LANG_COUNT);

    while (entry != NULL)
    {
	if (strcmp(string, entry->string) == 0)
	{
	    value = entry->value[language];
	    break;
	}
	entry = entry->next;
    }
    return value;
}

extern void freeKeywordTable()
{
    if (HashTable != NULL)
    {
	unsigned int i;

	for (i = 0  ;  i < TableSize  ;  ++i)
	{
	    hashEntry *entry = HashTable[i];

	    while (entry != NULL)
	    {
		hashEntry *next = entry->next;
		eFree(entry);
		entry = next;
	    }
	}
	eFree(HashTable);
    }
}

#ifdef DEBUG

static void printEntry __ARGS((const hashEntry *const entry));
static void printEntry( entry )
    const hashEntry *const entry;
{
    unsigned int i;

    printf("  %-15s", entry->string);

    for (i = 0  ;  i < LANG_COUNT  ;  ++i)
	printf("%-7s  ",
	       entry->value[i] == 0 ? "" : getLanguageName((langType)i));
    printf("\n");
}

static unsigned int printBucket __ARGS((const unsigned int i));
static unsigned int printBucket( i )
    const unsigned int i;
{
    hashEntry **const table = getHashTable();
    hashEntry *entry = table[i];
    unsigned int measure = 1;
    boolean first = TRUE;

    printf("%2d:", i);
    if (entry == NULL)
	printf("\n");
    else while (entry != NULL)
    {
	if (! first)
	    printf("    ");
	else
	{
	    printf(" ");
	    first = FALSE;
	}
	printEntry(entry);
	entry = entry->next;
	measure = 2 * measure;
    }
    return measure - 1;
}

extern void printKeywordTable()
{
    unsigned long emptyBucketCount = 0;
    unsigned long measure = 0;
    unsigned int i;

    for (i = 0  ;  i < TableSize  ;  ++i)
    {
	const unsigned int pass = printBucket(i);

	measure += pass;
	if (pass == 0)
	    ++emptyBucketCount;
    }

    printf("spread measure = %ld\n", measure);
    printf("%ld empty buckets\n", emptyBucketCount);
}

#endif
