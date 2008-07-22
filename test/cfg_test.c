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

void f(int);
void g(int);
void h(int);

void f(int a) {
  if (a > 13) {
    printf("greater than 13\n");
  } else {
    printf("not greater than 13\n");
  }
}

void g(int a) {
  h(a);

  if (a == 7) {
    printf("7\n");
  } else {
    printf("not 7\n");
  }

  i(a);
}

void h(int a) {
  if (a == -4) {
    printf("-4\n");
  } else {
    printf("not -4\n");
  }
}

void i(int a) {
  if (a == 100) {
    printf("100\n");
  } else {
    printf("not 100\n");
  }
}

int main(void) {
  int a;
  CREST_int(a);

  if (a == 19) {
    printf("19\n");
  } else {
    printf("not 19\n");
  }

  f(a);

  g(a);

  if (a != 1) {
    printf("not 1\n");
  } else {
    printf("1\n");
  }

  return 0;
}
