// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_YICES_SOLVER_H__
#define BASE_YICES_SOLVER_H__

#include <map>
#include <vector>

#include "base/basic_types.h"
#include "base/symbolic_predicate.h"

using std::map;
using std::vector;

namespace crest {

class YicesSolver {
 public:
  static bool IncrementalSolve(const vector<value_t>& old_soln,
			       const map<var_t,type_t>& vars,
                               const vector<const SymbolicPred*>& constraints,
			       map<var_t,value_t>* soln);

  static bool Solve(const map<var_t,type_t>& vars,
                    const vector<const SymbolicPred*>& constraints,
		    map<var_t,value_t>* soln);

  static bool ReadSolutionFromFileOrDie(const string& file,
                                        map<var_t,value_t>* soln);
};

}  // namespace crest


#endif  // BASE_YICES_SOLVER_H__
