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

// Entry point of stateless parser for official wants file provided by the
// Online Want List Generator (OLWLG): https://bgg.activityclub.org/olwlg/

#include "mathtrader/util/str_toupper.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace mathtrader::util {

// Converts a string to uppercase.
// Example:
//    CHECK_EQ(toupper("abCDeFGh"), "ABCDEFGH");
std::string StrToUpper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return str;
}

}  // namespace mathtrader::util
