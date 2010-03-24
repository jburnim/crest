/* Copyright (c) 2010, Jacob Burnim (jburnim@cs.berkeley.edu)
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
  int x, y, z;

  CREST_int(x);
  CREST_int(y);
  CREST_int(z);

  if (((x + 3) << 2) == 96) {
    printf("A\n");
    if (((y - 15) << x) == z) {
      printf("B\n");
    } else {
      printf("C\n");
    }
  }

  return 0;
}
