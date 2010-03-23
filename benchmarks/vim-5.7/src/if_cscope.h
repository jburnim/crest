/* vi:set ts=8 sts=4 sw=4:
 *
 * CSCOPE support for Vim added by Andy Kahn <kahn@zk3.dec.com>
 *
 * The basic idea/structure of cscope for Vim was borrowed from Nvi.
 * There might be a few lines of code that look similar to what Nvi
 * has.  If this is a problem and requires inclusion of the annoying
 * BSD license, then sue me; I'm not worth much anyway.
 */

#if defined(USE_CSCOPE) || defined(PROTO)

#include <sys/types.h>		/* pid_t */
#include <sys/stat.h>		/* dev_t, ino_t */

#define CSCOPE_SUCCESS		0
#define CSCOPE_FAILURE		-1
#define CSCOPE_MAX_CONNECTIONS	8   /* you actually need more? */

#define	CSCOPE_DBFILE		"cscope.out"
#define	CSCOPE_PROMPT		">> "
#define	CSCOPE_QUERIES		"sgdct efi"

/*
 * s 0name	Find this C symbol
 * g 1name	Find this definition
 * d 2name	Find functions called by this function
 * c 3name	Find functions calling this function
 * t 4string	find text string (cscope 12.9)
 * t 4name	Find assignments to (cscope 13.3)
 *   5pattern	change pattern -- NOT USED
 * e 6pattern	Find this egrep pattern
 * f 7name	Find this file
 * i 8name	Find files #including this file
 */
#define	FIND_USAGE "find c|d|e|f|g|i|s|t name"
#define FIND_HELP "\
       c: Find functions calling this function\n\
       d: Find functions called by this function\n\
       e: Find this egrep pattern\n\
       f: Find this file\n\
       g: Find this definition\n\
       i: Find files #including this file\n\
       s: Find this C symbol\n\
       t: Find assignments to\n"


typedef struct {
    char *  name;
    int     (*func) __ARGS((EXARG *eap));
    char *  help;
    char *  usage;
} cscmd_t;

typedef struct csi {
    char *	    fname;	/* cscope db name */
    char *	    ppath;	/* path to prepend (the -P option) */
    char *	    flags;	/* additional cscope flags/options (e.g, -p2) */
    pid_t	    pid;	/* PID of the connected cscope process. */
    dev_t	    st_dev;	/* ID of dev containing cscope db */
    ino_t	    st_ino;	/* inode number of cscope db */

    FILE *	    fr_fp;	/* from cscope: FILE. */
    int		    fr_fd;	/* from cscope: file descriptor. */
    FILE *	    to_fp;	/* to cscope: FILE. */
    int		    to_fd;	/* to cscope: file descriptor. */
} csinfo_t;

enum { Add, Find, Help, Kill, Reset, Show } csid;

typedef enum {
    Store,
    Get,
    Free,
    Print
} mcmd_e;


typedef unsigned char bool_t;	/* should prob go elsewhere */


#endif	/* USE_CSCOPE */

/* the end */
