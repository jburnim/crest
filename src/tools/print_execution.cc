// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <assert.h>
#include <fstream>
#include <iostream>
#include "base/symbolic_execution.h"

using namespace crest;
using namespace std;

int main(void) {
  SymbolicExecution ex;

  ifstream in("szd_execution", ios::in | ios::binary);
  assert(ex.Parse(in));
  in.close();

  // Print input.
  for (size_t i = 0; i < ex.inputs().size(); i++) {
    cout << "(= x" << i << " " << ex.inputs()[i] << ")\n";
  }
  cout << endl;

  { // Print the constraints.
    string tmp;
    for (size_t i = 0; i < ex.path().constraints().size(); i++) {
      tmp.clear();
      ex.path().constraints()[i]->AppendToString(&tmp);
      cout << tmp << endl;
    }
    cout << endl;
  }

  // Print the branches.
  for (size_t i = 0; i < ex.path().branches().size(); i++) {
    cout << ex.path().branches()[i] << "\n";
  }
  cout << endl << endl;

  return 0;
}
