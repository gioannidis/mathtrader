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

// Defines utility functions for strings.

#ifndef MATHTRADER_UTIL_STR_TOUPPER_H_
#define MATHTRADER_UTIL_STR_TOUPPER_H_

#include <string>

namespace mathtrader::util {

// Converts a string to uppercase.
// Example:
//    CHECK_EQ(toupper("abCDeFGh"), "ABCDEFGH");
std::string StrToUpper(std::string str);

// As above, but operates directly on a mutable string. Does nothing on null.
void StrToUpper(std::string* str);

}  // namespace mathtrader::util

#endif  // MATHTRADER_UTIL_STR_TOUPPER_H_
