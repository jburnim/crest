// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_SYMBOLIC_PATH_H__
#define BASE_SYMBOLIC_PATH_H__

#include <algorithm>
#include <istream>
#include <ostream>
#include <vector>

#include "base/basic_types.h"
#include "base/symbolic_predicate.h"

using std::istream;
using std::ostream;
using std::swap;
using std::vector;

namespace crest {

class SymbolicPath {
 public:
  SymbolicPath();
  SymbolicPath(bool pre_allocate);
  ~SymbolicPath();

  void Swap(SymbolicPath& sp);

  void Push(branch_id_t bid);
  void Push(branch_id_t bid, SymbolicPred* constraint);
  void Serialize(string* s) const;
  bool Parse(istream& s);

  const vector<branch_id_t>& branches() const { return branches_; }
  const vector<SymbolicPred*>& constraints() const { return constraints_; }
  const vector<size_t>& constraints_idx() const { return constraints_idx_; }

 private:
  vector<branch_id_t> branches_;
  vector<size_t> constraints_idx_;
  vector<SymbolicPred*> constraints_;
};

}  // namespace crest

#endif  // BASE_SYMBOLIC_PATH_H__
