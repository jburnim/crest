/* Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
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
  int a, b, c, d;
  CREST_int(a);
  CREST_int(b);
  CREST_int(c);
  CREST_int(d);

  if (a == 5) {
    if (b == 19) {
      if (c == 7) {
        if (d == 4) {
          fprintf(stderr, "GOAL!\n");
	}
      }
    }
  }

  return 0;
}
