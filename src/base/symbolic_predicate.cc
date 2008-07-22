// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include "base/symbolic_predicate.h"

namespace crest {

SymbolicPred::SymbolicPred()
  : op_(ops::EQ), expr_(new SymbolicExpr(0)) { }

SymbolicPred::SymbolicPred(compare_op_t op, SymbolicExpr* expr)
  : op_(op), expr_(expr) { }

SymbolicPred::~SymbolicPred() {
  delete expr_;
}

void SymbolicPred::Negate() {
  op_ = NegateCompareOp(op_);
}

void SymbolicPred::AppendToString(string* s) const {
  const char* symbol[] = { "=", "/=", ">", "<=", "<", ">=" };
  s->push_back('(');
  s->append(symbol[op_]);
  s->push_back(' ');
  expr_->AppendToString(s);
  s->append(" 0)");
}

void SymbolicPred::Serialize(string* s) const {
  s->push_back(static_cast<char>(op_));
  expr_->Serialize(s);
}

bool SymbolicPred::Parse(istream& s) {
  op_ = static_cast<compare_op_t>(s.get());
  return (expr_->Parse(s) && !s.fail());
}

bool SymbolicPred::Equal(const SymbolicPred& p) const {
  return ((op_ == p.op_) && (*expr_ == *p.expr_));
}


}  // namespace crest
