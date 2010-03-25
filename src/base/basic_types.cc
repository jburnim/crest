// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <cassert>
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

value_t CastTo(value_t val, type_t type) {
  switch (type) {
  case types::U_CHAR:   return static_cast<unsigned char>(val);
  case types::CHAR:     return static_cast<char>(val);
  case types::U_SHORT:  return static_cast<unsigned short>(val);
  case types::SHORT:    return static_cast<short>(val);
  case types::U_INT:    return static_cast<unsigned int>(val);
  case types::INT:      return static_cast<int>(val);
  case types::U_LONG:   return static_cast<unsigned long>(val);
  case types::LONG:     return static_cast<long>(val);

  case types::U_LONG_LONG:
  case types::LONG_LONG:
    // Cast would do nothing in these cases.
    return val;
  }

  // Cannot reach here.
  assert(false);
  return 0;
}

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

const size_t kByteSize[] = {
  sizeof(unsigned char),       sizeof(char),
  sizeof(unsigned short),      sizeof(short),
  sizeof(unsigned int),        sizeof(int),
  sizeof(unsigned long),       sizeof(long),
  sizeof(unsigned long long),  sizeof(long long)
};

}  // namespace crest

