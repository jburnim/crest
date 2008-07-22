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

int main(void) {
  int a, b, c, d, e;
  CREST_int(a);
  CREST_int(b);
  CREST_int(c);
  CREST_int(d);
  CREST_int(e);
  if (3*a + 3*(b - 5*c) + (b+c) - a == d - 17*e) {
    return 1;
  } else {
    return 0;
  }
}
