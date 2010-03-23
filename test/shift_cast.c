/* Copyright (c) 2009, Jacob Burnim (jburnim@cs.berkeley.edu)
 *
 * This file is part of CREST, which is distributed under the revised
 * BSD license.  A copy of this license can be found in the file LICENSE.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
 * for details.
 */

#include <crest.h>

#include <stdio.h>

int main(void) {
  unsigned short us;
  signed char sc;
  int x;

  CREST_unsigned_short(us);
  CREST_char(sc);
  CREST_int(x);

  unsigned short kFFFF = 0xFFFFu;

  if ((x >> 18) == 0xFFFFFFFF) {
    printf("A\n");
  } else if ((us >> 7) == kFFFF) {
    printf("B\n");
  } else if ((int)sc == -42) {
    printf("C\n");
  } else if ((int)us == -37) {
    printf("D\n");
  } else if ((unsigned int)(sc << 5) == 0xFFFFFFA0) {
    printf("E\n");
  }

  return 0;
}
