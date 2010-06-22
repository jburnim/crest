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

int A[100];
char B[12][97];

int main(void) {
  int x, y, i, j;

  CREST_int(x);
  CREST_int(y);

  if ((x < 0) || (x >= 97))
    return 0;
  if ((y < 0) || (y >= 12))
    return 0;

  for (i = 0; i < 100; i++) {
    A[i] = i;
  }

  for (i = 0; i < 12; i++) {
    for (j = 0; j < 97; j++) {
      B[i][j] = i + j;
    }
  }

  if (A[x] == 7) {
    /*
     * CREST cannot solve for 'x' to reach this branch because it does
     * not treat array indexing symbolically.
     */
    printf("Hello!\n");
  }

  if (B[y][x] == 42) {
    /*
     * CREST cannot solve for 'x' and 'y' to reach this branch because
     * it does not treat array indexing symbolically.
     */
    printf("World!\n");
  }

  return 0;
}
