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

// Parser for lines defining wantlists.

#ifndef MATHTRADER_PARSER_INTERNAL_WANTLIST_PARSER_H_
#define MATHTRADER_PARSER_INTERNAL_WANTLIST_PARSER_H_

#include <cstdint>

#include "absl/status/statusor.h"
#include "re2/re2.h"

#include "mathtrader/common/wantlist.pb.h"

namespace mathtrader::parser::internal {
// Parses a line of text defining a wantlist, where one item is offered in
// exchange for any one of the wanted items.
// Example usage:
//
//    WantlistParser wantlist_parser;
//    auto wantlist_or = wantlist_parser.ParseWantlist("1-A : 2-B 3-C 4-D");
//    CHECK(wantlist_or.ok());
//    Wantlist wantlist = *wantlist;
class WantlistParser {
 public:
  // Constructs the parser and dies if the regular expressions were not created
  // properly.
  explicit WantlistParser(int32_t small_step = 1, int32_t big_step = 9);

  // Disables copy constructor and assignment operator.
  WantlistParser(const WantlistParser&) = delete;
  WantlistParser& operator=(const WantlistParser&) = delete;

  // Parses the input text and returns a Wantlist on success. If it fails,
  // returns an InvalidArgumentError. Does not check whether the offered or any
  // of the wanted items correspond to an actual official item, if official item
  // names have been given.
  absl::StatusOr<common::Wantlist> ParseWantlist(std::string_view text) const;

  // Returns the internal string that is used to build the regex that parses the
  // wantlists.
  static std::string_view get_regex_str();

 private:
  // `kSmallStep` and `kBigStep` denote the default and the additional
  // increment of the wanted items' ranks in a wantlist. The rank of an item
  // corresponds roughly to its relative position in a want list.
  //
  // The rank of the first item is "1", with subsequent ranks increasing by
  // `kSmallStep`. If `kBigStepChar` (see below) is denoted between two items,
  // the rank of the latter item is additionally incremented by `kBigStep`.
  //
  // Example wantlist, with kSmallStep=1 and kBigStep=9:
  //    A : B ; C D
  //    rank(A) = 1
  //    rank(B) = rank(A)
  //    rank(C) = rank(B) + kSmallStep + kBigStep
  //    rank(D) = rank(C) + kSmallStep
  const int32_t kSmallStep;

  // Additional increment for wanted item's rank. See `kSmallStep` for details.
  const int32_t kBigStep;

  // An optional delimeter to denote a bigger rank increment for the next item
  // in a wantlist. See `kBigStep` for details.
  static constexpr char kBigStepChar = ';';

  // Computes the priority of a wanted item based on its rank.
  int32_t ComputePriority(int32_t rank) const;

  // Matches the prefix of a wantlist. Captures username, offered item and an
  // optional colon ':'. Example:
  //    Text: "(user) 0001-A : 0002-B ; 0003-C"
  //    Captured texts: "user", "0001-A", ":"
  const re2::RE2 kWantlistPrefixRegex;
};
}  // namespace mathtrader::parser::internal
#endif  // MATHTRADER_PARSER_INTERNAL_WANTLIST_PARSER_H_
