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

// Internal parser module that handles the public API.

#include "mathtrader/parser/internal/internal_parser.h"

#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "ortools/base/filelineiter.h"

namespace mathtrader::internal_parser {
namespace {
// Regex defining the lines to ignore: empty lines, lines with all whitespaces,
// or comment lines beginning with "#" but not "#!".
constexpr char kIgnoreLineRegexStr[] = R"(^#[^!]|^\s*$)";
}  // namespace

// Initializes the constant members.
InternalParser::InternalParser() : kIgnoreLineRegex(kIgnoreLineRegexStr) {}

// Opens and parses the given want file. Dies if the file is empty or not found.
absl::Status InternalParser::ParseFile(absl::string_view filename) {
  // Takes ownership of the file and reads its lines one-by-one.
  line_count_ = 0;
  for (absl::string_view line : FileLines(static_cast<std::string>(filename),
                                          FileLineIterator::DEFAULT)) {
    ++line_count_;
    if (absl::Status status = ParseLine(line); !status.ok()) {
      return status;
    }
  }
  if (!line_count_) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Could not open input file or file is empty: %s", filename));
  }
  FinalizeParserResult();
  return absl::OkStatus();
}

// Identical to `Parse`, but operates on a string.
absl::Status InternalParser::ParseText(absl::string_view data) {
  if (data.empty()) {
    return absl::InvalidArgumentError("Empty data.");
  }

  std::vector<absl::string_view> lines = absl::StrSplit(data, '\n');
  line_count_ = 0;
  for (absl::string_view line : lines) {
    ++line_count_;
    if (absl::Status status = ParseLine(line); !status.ok()) {
      return status;
    }
  }
  FinalizeParserResult();
  return absl::OkStatus();
}
}  // namespace mathtrader::internal_parser
