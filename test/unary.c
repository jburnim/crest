/* Copyright (c) 2012, Jacob Burnim (jburnim@cs.berkeley.edu)
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
  int x = 2;
  int a, b;
  CREST_int(a);
  b = -a;
  if (-x * a == 8) {
    printf("8 (%d %d)\n", a, b);
  } else {
    printf("not 8 (%d %d)\n", a, b);
  }
  return 0;
}
