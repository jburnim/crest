// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <utility>

#include "base/symbolic_execution.h"

namespace crest {

SymbolicExecution::SymbolicExecution() { }

SymbolicExecution::SymbolicExecution(bool pre_allocate)
  : path_(pre_allocate) { }

SymbolicExecution::~SymbolicExecution() { }

void SymbolicExecution::Swap(SymbolicExecution& se) {
  vars_.swap(se.vars_);
  inputs_.swap(se.inputs_);
  path_.Swap(se.path_);
}

void SymbolicExecution::Serialize(string* s) const {
  typedef map<var_t,type_t>::const_iterator VarIt;

  // Write the inputs.
  size_t len = vars_.size();
  s->append((char*)&len, sizeof(len));
  for (VarIt i = vars_.begin(); i != vars_.end(); ++i) {
    s->push_back(static_cast<char>(i->second));
    s->append((char*)&inputs_[i->first], sizeof(value_t));
  }

  // Write the path.
  path_.Serialize(s);
}

bool SymbolicExecution::Parse(istream& s) {
  // Read the inputs.
  size_t len;
  s.read((char*)&len, sizeof(len));
  vars_.clear();
  inputs_.resize(len);
  for (size_t i = 0; i < len; i++) {
    vars_[i] = static_cast<type_t>(s.get());
    s.read((char*)&inputs_[i], sizeof(value_t));
  }

  // Write the path.
  return (path_.Parse(s) && !s.fail());
}

}  // namespace crest
