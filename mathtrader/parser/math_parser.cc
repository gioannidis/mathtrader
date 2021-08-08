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

#include "mathtrader/parser/math_parser.h"

#include <utility>

#include "absl/status/statusor.h"

#include "mathtrader/parser/internal/internal_parser.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::parser {
namespace {
// Creates a stateful internal parser, parses the OLWLG data and returns the
// result.
absl::StatusOr<ParserResult> ParseFileOrText(absl::string_view file_or_data,
                                             bool is_file) {
  internal::InternalParser parser;

  // Calls the internal parser and returns if an error has been raised.
  if (absl::Status status = (is_file ? parser.ParseFile(file_or_data)
                                     : parser.ParseText(file_or_data));
      !status.ok()) {
    return status;
  }
  return std::move(*parser.mutable_parser_result());
}
}  // namespace

// Parses the OLWLG-generated file and generates the trade data.
absl::StatusOr<ParserResult> MathParser::ParseFile(absl::string_view filename) {
  return ParseFileOrText(filename, /*is_file=*/true);
}

// Identical to `ParseFile`, but operates directly on the data string.
absl::StatusOr<ParserResult> MathParser::ParseText(absl::string_view text) {
  return ParseFileOrText(text, /*is_file=*/false);
}
}  // namespace mathtrader::parser
