/*****************************************************************************
*   $Id: parse.h,v 8.1 1999/03/04 04:16:38 darren Exp $
*
*   Copyright (c) 1998-1999, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   External interface to all language parsing modules.
*****************************************************************************/
#ifndef _PARSE_H
#define _PARSE_H

/*============================================================================
=   Include files
============================================================================*/
#include "general.h"

/*============================================================================
=   Function prototypes
============================================================================*/
extern boolean createCTags __ARGS((const unsigned int passCount));
extern boolean createEiffelTags __ARGS((void));
extern boolean createFortranTags __ARGS((const unsigned int passCount));

#endif	/* _PARSE_H */

/* vi:set tabstop=8 shiftwidth=4: */
