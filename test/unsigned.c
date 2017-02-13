/* Copyright (c) 2017, Jacob Burnim (jburnim@gmail.com)
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
  unsigned int a;
  CREST_unsigned_int(a);

  if (a > 2147483647u) {
    if (a > 4294967294u) {
      printf("a\n");
    } else {
      printf("b\n");

    }
  } else {
    printf("c\n");
  }

  return 0;
}
