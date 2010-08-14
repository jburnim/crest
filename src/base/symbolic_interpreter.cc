// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <utility>
#include <vector>

#include "base/symbolic_interpreter.h"
#include "base/yices_solver.h"

using std::make_pair;
using std::swap;
using std::vector;

#ifdef DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

namespace crest {

typedef map<addr_t,SymbolicExpr*>::const_iterator ConstMemIt;

SymbolicInterpreter::SymbolicInterpreter()
  : pred_(NULL), return_value_(false), ex_(true), num_inputs_(0) {
  stack_.reserve(16);
}

SymbolicInterpreter::SymbolicInterpreter(const vector<value_t>& input)
  : pred_(NULL), return_value_(false), ex_(true), num_inputs_(0) {
  stack_.reserve(16);
  ex_.mutable_inputs()->assign(input.begin(), input.end());
}

void SymbolicInterpreter::DumpMemory() {
  for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
    string s;
    i->second->AppendToString(&s);
    fprintf(stderr, "%lu: %s [%d]\n", i->first, s.c_str(), *(int*)(i->first));
  }
  for (size_t i = 0; i < stack_.size(); i++) {
    string s;
    if (stack_[i].expr) {
      stack_[i].expr->AppendToString(&s);
    } else if ((i == stack_.size() - 1) && pred_) {
      pred_->AppendToString(&s);
    }
    if ((i == (stack_.size() - 1)) && return_value_) {
      fprintf(stderr, "s%d: %lld [ %s ] (RETURN VALUE)\n",
	      i, stack_[i].concrete, s.c_str());
    } else {
      fprintf(stderr, "s%d: %lld [ %s ]\n", i, stack_[i].concrete, s.c_str());
    }
  }
  if ((stack_.size() == 0) && return_value_) {
    fprintf(stderr, "MISSING RETURN VALUE\n");
  }
}


void SymbolicInterpreter::ClearStack(id_t id) {
  IFDEBUG(fprintf(stderr, "clear\n"));
  for (vector<StackElem>::const_iterator it = stack_.begin(); it != stack_.end(); ++it) {
    delete it->expr;
  }
  stack_.clear();
  ClearPredicateRegister();
  return_value_ = false;
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::Load(id_t id, addr_t addr, value_t value) {
  IFDEBUG(fprintf(stderr, "load %lu %lld\n", addr, value));
  ConstMemIt it = mem_.find(addr);
  if (it == mem_.end()) {
    PushConcrete(value);
  } else {
    PushSymbolic(new SymbolicExpr(*it->second), value);
  }
  ClearPredicateRegister();
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::Store(id_t id, addr_t addr) {
  IFDEBUG(fprintf(stderr, "store %lu\n", addr));
  assert(stack_.size() > 0);

  const StackElem& se = stack_.back();
  if (se.expr) {
    if (!se.expr->IsConcrete()) {
      mem_[addr] = se.expr;
    } else {
      mem_.erase(addr);
      delete se.expr;
    }
  } else {
    mem_.erase(addr);
  }

  stack_.pop_back();
  ClearPredicateRegister();
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::ApplyUnaryOp(id_t id, unary_op_t op, value_t value) {
  IFDEBUG(fprintf(stderr, "apply1 %d %lld\n", op, value));
  assert(stack_.size() >= 1);
  StackElem& se = stack_.back();

  if (se.expr) {
    switch (op) {
    case ops::NEGATE:
      se.expr->Negate();
      ClearPredicateRegister();
      break;
    case ops::LOGICAL_NOT:
      if (pred_) {
	pred_->Negate();
	break;
      }
      // Otherwise, fall through to the concrete case.
    default:
      // Concrete operator.
      delete se.expr;
      se.expr = NULL;
      ClearPredicateRegister();
    }
  }

  se.concrete = value;
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::ApplyBinaryOp(id_t id, binary_op_t op, value_t value) {
  IFDEBUG(fprintf(stderr, "apply2 %d %lld\n", op, value));
  assert(stack_.size() >= 2);
  StackElem& a = *(stack_.rbegin()+1);
  StackElem& b = stack_.back();

  if (a.expr || b.expr) {
    switch (op) {
    case ops::ADD:
      if (a.expr == NULL) {
	swap(a, b);
	*a.expr += b.concrete;
      } else if (b.expr == NULL) {
	*a.expr += b.concrete;
      } else {
	*a.expr += *b.expr;
	delete b.expr;
      }
      break;

    case ops::SUBTRACT:
      if (a.expr == NULL) {
	b.expr->Negate();
	swap(a, b);
	*a.expr += b.concrete;
      } else if (b.expr == NULL) {
	*a.expr -= b.concrete;
      } else {
	*a.expr -= *b.expr;
	delete b.expr;
      }
      break;

    case ops::SHIFT_L:
      if (a.expr != NULL) {
        // Convert to multiplication by a (concrete) constant.
        *a.expr *= (1 << b.concrete);
      }
      delete b.expr;
      break;

    case ops::MULTIPLY:
      if (a.expr == NULL) {
	swap(a, b);
	*a.expr *= b.concrete;
      } else if (b.expr == NULL) {
	*a.expr *= b.concrete;
      } else {
	swap(a, b);
	*a.expr *= b.concrete;
	delete b.expr;
      }
      break;

    default:
      // Concrete operator.
      delete a.expr;
      delete b.expr;
      a.expr = NULL;
    }
  }

  a.concrete = value;
  stack_.pop_back();
  ClearPredicateRegister();
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::ApplyCompareOp(id_t id, compare_op_t op, value_t value) {
  IFDEBUG(fprintf(stderr, "compare2 %d %lld\n", op, value));
  assert(stack_.size() >= 2);
  StackElem& a = *(stack_.rbegin()+1);
  StackElem& b = stack_.back();

  if (a.expr || b.expr) {
    // Symbolically compute "a -= b".
    if (a.expr == NULL) {
      b.expr->Negate();
      swap(a, b);
      *a.expr += b.concrete;
    } else if (b.expr == NULL) {
      *a.expr -= b.concrete;
    } else {
      *a.expr -= *b.expr;
      delete b.expr;
    }
    // Construct a symbolic predicate (if "a - b" is symbolic), and
    // store it in the predicate register.
    if (!a.expr->IsConcrete()) {
      pred_ = new SymbolicPred(op, a.expr);
    } else {
      ClearPredicateRegister();
      delete a.expr;
    }
    // We leave a concrete value on the stack.
    a.expr = NULL;
  }

  a.concrete = value;
  stack_.pop_back();
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::Call(id_t id, function_id_t fid) {
  IFDEBUG(fprintf(stderr, "call %u\n", fid));
  ex_.mutable_path()->Push(kCallId);
  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::Return(id_t id) {
  IFDEBUG(fprintf(stderr, "return\n"));

  ex_.mutable_path()->Push(kReturnId);

  // There is either exactly one value on the stack -- the current function's
  // return value -- or the stack is empty.
  assert(stack_.size() <= 1);

  return_value_ = (stack_.size() == 1);

  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::HandleReturn(id_t id, value_t value) {
  IFDEBUG(fprintf(stderr, "handle_return %lld\n", value));

  if (return_value_) {
    // We just returned from an instrumented function, so the stack
    // contains a single element -- the (possibly symbolic) return value.
    assert(stack_.size() == 1);
    return_value_ = false;
  } else {
    // We just returned from an uninstrumented function, so the stack
    // still contains the arguments to that function.  Thus, we clear
    // the stack and push the concrete value that was returned.
    ClearStack(-1);
    PushConcrete(value);
  }

  IFDEBUG(DumpMemory());
}


void SymbolicInterpreter::Branch(id_t id, branch_id_t bid, bool pred_value) {
  IFDEBUG(fprintf(stderr, "branch %d %d\n", bid, pred_value));
  assert(stack_.size() == 1);
  stack_.pop_back();

  if (pred_ && !pred_value) {
    pred_->Negate();
  }

  ex_.mutable_path()->Push(bid, pred_);
  pred_ = NULL;
  IFDEBUG(DumpMemory());
}


value_t SymbolicInterpreter::NewInput(type_t type, addr_t addr) {
  IFDEBUG(fprintf(stderr, "symbolic_input %d %lu\n", type, addr));

  mem_[addr] = new SymbolicExpr(1, num_inputs_);
  ex_.mutable_vars()->insert(make_pair(num_inputs_ ,type));

  value_t ret = 0;
  if (num_inputs_ < ex_.inputs().size()) {
    ret = ex_.inputs()[num_inputs_];
  } else {
    // Generate a new random input.
    // TODO: User a better pseudorandom number generator.
    ret = CastTo(rand(), type);
    ex_.mutable_inputs()->push_back(ret);
  }

  num_inputs_ ++;

  IFDEBUG(DumpMemory());
  return ret;
}


void SymbolicInterpreter::PushConcrete(value_t value) {
  PushSymbolic(NULL, value);
}


void SymbolicInterpreter::PushSymbolic(SymbolicExpr* expr, value_t value) {
  stack_.push_back(StackElem());
  StackElem& se = stack_.back();
  se.expr = expr;
  se.concrete = value;
}


void SymbolicInterpreter::ClearPredicateRegister() {
  delete pred_;
  pred_ = NULL;
}


}  // namespace crest
