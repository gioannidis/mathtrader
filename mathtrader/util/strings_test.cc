// This file is part of mathtrader.
//
// Copyright (C) 2021 George Ioannidis
//
// mathtrader is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// mathtrader is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

#include "mathtrader/util/str_toupper.h"

#include <string>

#include "gtest/gtest.h"

namespace {
using ::mathtrader::util::StrToUpper;

TEST(StrToUpperTest, TestAllLowerChars) {
  EXPECT_EQ(StrToUpper("qwertyuiop"), "QWERTYUIOP");
}

TEST(StrToUpperTest, TestAllUpperChars) {
  EXPECT_EQ(StrToUpper("QWERTYUIOP"), "QWERTYUIOP");
}

TEST(StrToUpperTest, TestMixedChars) {
  EXPECT_EQ(StrToUpper("QWerTYuiOPasdfGHjkLzxCVbnm"),
            "QWERTYUIOPASDFGHJKLZXCVBNM");
}

TEST(StrToUpperTest, TestNonAlphaChars) {
  EXPECT_EQ(StrToUpper("qwerty!@#$%^&*()uiop"), "QWERTY!@#$%^&*()UIOP");
}

TEST(StrToUpperTest, TestWithMutable) {
  static constexpr char kOriginalString[] = "abCdeFGHiJKlm!@#$nOp";
  std::string str = kOriginalString;
  StrToUpper(&str);
  EXPECT_EQ(str, "ABCDEFGHIJKLM!@#$NOP");
}

TEST(StrToUpperTest, TestWithNullMutable) {
  std::string* const str = nullptr;
  StrToUpper(str);
  EXPECT_EQ(str, nullptr);
}
}  // namespace
