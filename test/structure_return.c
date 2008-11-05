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

struct banana {
  int price;
  int weight;
};

struct banana symbolic_banana() {
  struct banana b;
  CREST_int(b.price);
  CREST_int(b.weight);
  return b;
}

int main(void) {
  struct banana b = symbolic_banana();

  if ((b.weight > 136) && (b.price < 15)) { }

  return 0;
}
