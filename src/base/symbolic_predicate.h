// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_SYMBOLIC_PREDICATE_H__
#define BASE_SYMBOLIC_PREDICATE_H__

#include <istream>
#include <map>
#include <ostream>
#include <set>

#include "base/symbolic_expression.h"

using std::istream;
using std::map;
using std::ostream;
using std::set;

namespace crest {

class SymbolicPred {
 public:
  SymbolicPred();
  SymbolicPred(compare_op_t op, SymbolicExpr* expr);
  ~SymbolicPred();

  void Negate();
  void AppendToString(string* s) const;

  void Serialize(string* s) const;
  bool Parse(istream& s);

  bool Equal(const SymbolicPred& p) const;

  void AppendVars(set<var_t>* vars) const {
    expr_->AppendVars(vars);
  }

  bool DependsOn(const map<var_t,type_t>& vars) const {
    return expr_->DependsOn(vars);
  }

  compare_op_t op() const { return op_; }
  const SymbolicExpr& expr() const { return *expr_; }

 private:
  compare_op_t op_;
  SymbolicExpr* expr_;
};

}  // namespace crest

#endif  // BASE_SYMBOLIC_PREDICATE_H__

