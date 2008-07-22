// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include "base/symbolic_path.h"

namespace crest {

SymbolicPath::SymbolicPath() { }

SymbolicPath::SymbolicPath(bool pre_allocate) {
  if (pre_allocate) {
    // To cut down on re-allocation.
    branches_.reserve(4000000);
    constraints_idx_.reserve(50000);
    constraints_.reserve(50000);
  }
}

SymbolicPath::~SymbolicPath() {
  for (size_t i = 0; i < constraints_.size(); i++)
    delete constraints_[i];
}

void SymbolicPath::Swap(SymbolicPath& sp) {
  branches_.swap(sp.branches_);
  constraints_idx_.swap(sp.constraints_idx_);
  constraints_.swap(sp.constraints_);
}

void SymbolicPath::Push(branch_id_t bid) {
  branches_.push_back(bid);
}

void SymbolicPath::Push(branch_id_t bid, SymbolicPred* constraint) {
  if (constraint) {
    constraints_.push_back(constraint);
    constraints_idx_.push_back(branches_.size());
  }
  branches_.push_back(bid);
}

void SymbolicPath::Serialize(string* s) const {
  typedef vector<SymbolicPred*>::const_iterator ConIt;

  // Write the path.
  size_t len = branches_.size();
  s->append((char*)&len, sizeof(len));
  s->append((char*)&branches_.front(), branches_.size() * sizeof(branch_id_t));

  // Write the path constraints.
  len = constraints_.size();
  s->append((char*)&len, sizeof(len));
  s->append((char*)&constraints_idx_.front(), constraints_.size() * sizeof(size_t));
  for (ConIt i = constraints_.begin(); i != constraints_.end(); ++i) {
    (*i)->Serialize(s);
  }
}

bool SymbolicPath::Parse(istream& s) {
  typedef vector<SymbolicPred*>::iterator ConIt;
  size_t len;

  // Read the path.
  s.read((char*)&len, sizeof(size_t));
  branches_.resize(len);
  s.read((char*)&branches_.front(), len * sizeof(branch_id_t));
  if (s.fail())
    return false;

  // Clean up any existing path constraints.
  for (size_t i = 0; i < constraints_.size(); i++)
    delete constraints_[i];

  // Read the path constraints.
  s.read((char*)&len, sizeof(size_t));
  constraints_idx_.resize(len);
  constraints_.resize(len);
  s.read((char*)&constraints_idx_.front(), len * sizeof(size_t));
  for (ConIt i = constraints_.begin(); i != constraints_.end(); ++i) {
    *i = new SymbolicPred();
    if (!(*i)->Parse(s))
      return false;
  }

  return !s.fail();
}

}  // namespace crest
