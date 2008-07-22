// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_SYMBOLIC_EXECUTION_H__
#define BASE_SYMBOLIC_EXECUTION_H__

#include <istream>
#include <ostream>
#include <utility>
#include <vector>

#include "base/basic_types.h"
#include "base/symbolic_path.h"

using std::istream;
using std::make_pair;
using std::ostream;
using std::vector;

namespace crest {

class SymbolicExecution {
 public:
  SymbolicExecution();
  explicit SymbolicExecution(bool pre_allocate);
  ~SymbolicExecution();

  void Swap(SymbolicExecution& se);

  void Serialize(string* s) const;
  bool Parse(istream& s);

  const map<var_t,type_t>& vars() const { return vars_; }
  const vector<value_t>& inputs() const { return inputs_; }
  const SymbolicPath& path() const      { return path_; }

  map<var_t,type_t>* mutable_vars() { return &vars_; }
  vector<value_t>* mutable_inputs() { return &inputs_; }
  SymbolicPath* mutable_path() { return &path_; }

 private:
  map<var_t,type_t>  vars_;
  vector<value_t> inputs_;
  SymbolicPath path_;  
};

}  // namespace crest

#endif  // BASE_SYMBOLIC_EXECUTION_H__
