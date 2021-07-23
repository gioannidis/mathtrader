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

#ifndef MATHTRADER_PARSER_MATH_PARSER_H_
#define MATHTRADER_PARSER_MATH_PARSER_H_

#include "absl/status/statusor.h"

#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader {

class MathParser {
 public:
  MathParser() = default;

  // Disables copy constructor and move assignment.
  MathParser(const MathParser&) = delete;
  MathParser& operator=(const MathParser&) = delete;

  // Parses the OLWLG-generated file and generates the trade data.
  static absl::StatusOr<ParserResult> ParseFile(absl::string_view filename);

  // Identical to `ParseFile`, but operates directly on the data string.
  static absl::StatusOr<ParserResult> ParseText(absl::string_view text);
};

}  // namespace mathtrader

#endif  // MATHTRADER_PARSER_MATH_PARSER_H_
