// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <limits>
#include "base/basic_types.h"

using std::numeric_limits;

namespace crest {

compare_op_t NegateCompareOp(compare_op_t op) {
  return static_cast<compare_op_t>(op ^ 1);
}

const char* kMinValueStr[] = {
  "0",
  "-128",
  "0",
  "-32768",
  "0",
  "-2147483648",
  "0",
  "-2147483648",
  "0",
  "-9223372036854775808",
};

const char* kMaxValueStr[] = {
  "255",
  "127",
  "65535",
  "32767",
  "4294967295",
  "2147483647",
  "4294967295",
  "2147483647",
  "18446744073709551615",
  "9223372036854775807",
};

const value_t kMinValue[] = {
  numeric_limits<unsigned char>::min(),
  numeric_limits<char>::min(),
  numeric_limits<unsigned short>::min(),
  numeric_limits<short>::min(),
  numeric_limits<unsigned int>::min(),
  numeric_limits<int>::min(),
  numeric_limits<unsigned long>::min(),
  numeric_limits<long>::min(),
  numeric_limits<unsigned long long>::min(),
  numeric_limits<long long>::min(),
};

const value_t kMaxValue[] = {
  numeric_limits<unsigned char>::max(),
  numeric_limits<char>::max(),
  numeric_limits<unsigned short>::max(),
  numeric_limits<short>::max(),
  numeric_limits<unsigned int>::max(),
  numeric_limits<int>::max(),
  numeric_limits<unsigned long>::max(),
  numeric_limits<long>::max(),
  numeric_limits<unsigned long long>::max(),
  numeric_limits<long long>::max(),
};

}  // namespace crest

