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
#include <stdio.h>
#include "base/symbolic_expression.h"

namespace crest {

typedef map<var_t,value_t>::iterator It;
typedef map<var_t,value_t>::const_iterator ConstIt;


SymbolicExpr::~SymbolicExpr() { }

SymbolicExpr::SymbolicExpr() : const_(0) { }

SymbolicExpr::SymbolicExpr(value_t c) : const_(c) { }

SymbolicExpr::SymbolicExpr(value_t c, var_t v) : const_(0) {
  coeff_[v] = c;
}

SymbolicExpr::SymbolicExpr(const SymbolicExpr& e)
  : const_(e.const_), coeff_(e.coeff_) { }


void SymbolicExpr::Negate() {
  const_ = -const_;
  for (It i = coeff_.begin(); i != coeff_.end(); ++i) {
    i->second = -i->second;
  }
}


void SymbolicExpr::AppendVars(set<var_t>* vars) const {
  for (ConstIt i = coeff_.begin(); i != coeff_.end(); ++i) {
    vars->insert(i->first);
  }
}

bool SymbolicExpr::DependsOn(const map<var_t,type_t>& vars) const {
  for (ConstIt i = coeff_.begin(); i != coeff_.end(); ++i) {
    if (vars.find(i->first) != vars.end())
      return true;
  }
  return false;
}


void SymbolicExpr::AppendToString(string* s) const {
  char buff[32];
  sprintf(buff, "(+ %lld", const_);
  s->append(buff);

  for (ConstIt i = coeff_.begin(); i != coeff_.end(); ++i) {
    sprintf(buff, " (* %lld x%u)", i->second, i->first);
    s->append(buff);
  }

  s->push_back(')');
}


void SymbolicExpr::Serialize(string* s) const {
  assert(coeff_.size() < 128);
  s->push_back(static_cast<char>(coeff_.size()));
  s->append((char*)&const_, sizeof(value_t));
  for (ConstIt i = coeff_.begin(); i != coeff_.end(); ++i) {
    s->append((char*)&i->first, sizeof(var_t));
    s->append((char*)&i->second, sizeof(value_t));
  }
}


bool SymbolicExpr::Parse(istream& s) {
  size_t len = static_cast<size_t>(s.get());
  s.read((char*)&const_, sizeof(value_t));
  if (s.fail())
    return false;

  coeff_.clear();
  for (size_t i = 0; i < len; i++) {
    var_t v;
    value_t c;
    s.read((char*)&v, sizeof(v));
    s.read((char*)&c, sizeof(c));
    coeff_[v] = c;
  }

  return !s.fail();
}


const SymbolicExpr& SymbolicExpr::operator+=(const SymbolicExpr& e) {
  const_ += e.const_;
  for (ConstIt i = e.coeff_.begin(); i != e.coeff_.end(); ++i) {
    It j = coeff_.find(i->first);
    if (j == coeff_.end()) {
      coeff_.insert(*i);
    } else {
      j->second += i->second;
      if (j->second == 0) {
	coeff_.erase(j);
      }
    }
  }
  return *this;
}


const SymbolicExpr& SymbolicExpr::operator-=(const SymbolicExpr& e) {
  const_ -= e.const_;
  for (ConstIt i = e.coeff_.begin(); i != e.coeff_.end(); ++i) {
    It j = coeff_.find(i->first);
    if (j == coeff_.end()) {
      coeff_[i->first] = -i->second;
    } else {
      j->second -= i->second;
      if (j->second == 0) {
	coeff_.erase(j);
      }
    }
  }
  return *this;
}


const SymbolicExpr& SymbolicExpr::operator+=(value_t c) {
  const_ += c;
  return *this;
}


const SymbolicExpr& SymbolicExpr::operator-=(value_t c) {
  const_ -= c;
  return *this;
}


const SymbolicExpr& SymbolicExpr::operator*=(value_t c) {
  if (c == 0) {
    coeff_.clear();
    const_ = 0;
  } else {
    const_ *= c;
    for (It i = coeff_.begin(); i != coeff_.end(); ++i) {
      i->second *= c;
    }
  }
  return *this;
}

bool SymbolicExpr::operator==(const SymbolicExpr& e) const {
  return ((const_ == e.const_) && (coeff_ == e.coeff_));
}


}  // namespace crest
