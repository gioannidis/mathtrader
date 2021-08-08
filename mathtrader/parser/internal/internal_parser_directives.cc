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

// Internal parser module that handles directives.

#include "mathtrader/parser/internal/internal_parser.h"

#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ortools/base/map_util.h"

namespace mathtrader::parser::internal {
namespace {
// Indicates start of official names.
constexpr char kDirectiveOfficialNamesBegin[] = "BEGIN-OFFICIAL-NAMES";

// Indicates end of official names.
constexpr char kDirectiveOfficialNamesEnd[] = "END-OFFICIAL-NAMES";
}  // namespace

// Parses a directive. Exactly one directive can be specified per line.
// Example usage:
//
//    ParseDirective("!DIRECTIVE-1");
absl::Status InternalParser::ParseDirective(absl::string_view line) {
  static constexpr absl::string_view kInternalErrorMsg =
      "Internal error when processing directive %s.";

  const absl::string_view directive = line;

  // Registers the directive to identify double declarations. Returns error if
  // already inserted.
  if (const auto [it, inserted] = directives_.try_emplace(
          static_cast<std::string>(directive), line_count_);
      !inserted) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Duplicate declaration of directive %s not permitted, previously "
        "declared in line %d.",
        directive, it->second));
  }

  // Handles the directive.
  if (directive == kDirectiveOfficialNamesBegin) {
    // Directs the parser to start reading official item names.
    switch (state_) {
      case ParserState::kOptionParsing: {
        state_ = ParserState::kItemParsing;

        // Indicates that all items stored in this->items_ represent official
        // names and are not auto-added during wantlist parsing.
        has_official_names_ = true;
        break;
      }
      case ParserState::kWantlistParsing: {
        return absl::InvalidArgumentError(absl::StrFormat(
            "Encountered directive %s but wantlists are currently being "
            "processed.",
            kDirectiveOfficialNamesBegin));
      }
      default: {
        // Other errors: expected to be caught by the double directive check.
        return absl::InternalError(
            absl::StrFormat(kInternalErrorMsg, kDirectiveOfficialNamesBegin));
      }
    }
  } else if (line == kDirectiveOfficialNamesEnd) {
    // Directs the parser to stop reading official item names and start reading
    // wantlists.
    if (!gtl::FindOrNull(directives_, kDirectiveOfficialNamesBegin)) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Declaring directive %s requires previous declaration of directive "
          "%s, which is missing.",
          kDirectiveOfficialNamesEnd, kDirectiveOfficialNamesBegin));
    }
    switch (state_) {
      case ParserState::kItemParsing: {
        state_ = ParserState::kWantlistParsing;
        break;
      }
      default: {
        // Other errors: expected to be caught by the double directive check.
        return absl::InternalError(
            absl::StrFormat(kInternalErrorMsg, kDirectiveOfficialNamesBegin));
        break;
      }
    }
  } else {
    // Handles an unknown directive.
    return absl::InvalidArgumentError(
        absl::StrFormat("Encountered unsupported directive: %s.", directive));
  }
  return absl::OkStatus();
}
}  // namespace mathtrader::parser::internal
