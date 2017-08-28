/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#include "catch.hpp"
#include "utility.h"

TEST_CASE("utility getBitmask", "utility getBitmask") {

  CHECK(getBitmask<uint64_t>(8) == 0x00000000000000FFULL);
  CHECK((getBitmask<uint64_t>(24) << 40) == 0xFFFFFF0000000000ULL);
  CHECK((getBitmask<uint64_t>(12) << 8) == 0x00000000000FFF00ULL);
  CHECK((getBitmask<uint64_t>(10) << 20) == 0x000000003FF00000ULL);
  CHECK((getBitmask<uint64_t>(10) << 30) == 0x000000FFC0000000);
}

TEST_CASE("utility getSetBitsCount") {

  CHECK(getSetBitsCount(0x00000000000000FFULL) == 8);
  CHECK(getSetBitsCount(0xFFFFFF0000000000ULL) == 24);
  CHECK(getSetBitsCount(0x00000000000FFF00ULL) == 12);
  CHECK(getSetBitsCount(0x000000003FF00000ULL) == 10);
  CHECK(getSetBitsCount(0x000000FFC0000000) == 10);
}

TEST_CASE("utility getShiftBitsCount") {

  CHECK(getShiftBitsCount(0x00000000000000FFULL) == 0);
  CHECK(getShiftBitsCount(0xFFFFFF0000000000ULL) == 40);
  CHECK(getShiftBitsCount(0x00000000000FFF00ULL) == 8);
  CHECK(getShiftBitsCount(0x000000003FF00000ULL) == 20);
  CHECK(getShiftBitsCount(0x000000FFC0000000) == 30);
}