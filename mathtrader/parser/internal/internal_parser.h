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

// Parser for official wants file provided by the Online Want List Generator:
// https://bgg.activityclub.org/olwlg/

#ifndef MATHTRADER_PARSER_INTERNAL_INTERNAL_PARSER_H_
#define MATHTRADER_PARSER_INTERNAL_INTERNAL_PARSER_H_

#include <string>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "re2/re2.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/internal/item_parser.h"
#include "mathtrader/parser/internal/wantlist_parser.h"
#include "mathtrader/parser/trade_request.pb.h"

namespace mathtrader::parser::internal {
class InternalParser {
 public:
  InternalParser();

  // Disables copy constructor and move assignment.
  InternalParser(const InternalParser&) = delete;
  InternalParser& operator=(const InternalParser&) = delete;

  ~InternalParser() = default;

  // Opens and parses the given want file.
  ABSL_MUST_USE_RESULT absl::Status ParseFile(std::string_view filename);

  // Identical to `ParseFile`, but operates directly on the data string.
  ABSL_MUST_USE_RESULT absl::Status ParseText(std::string_view data);

  // Returns the parsed TradeRequest.
  const TradeRequest& trade_request() const { return trade_request_; }

  // Returns a mutable pointer to the parsed TradeRequest.
  TradeRequest* mutable_trade_request() { return &trade_request_; }

  // Returns the number of parsed lines.
  const int64_t get_line_count() const { return line_count_; }

 private:
  // Defines the state of the parser and the context it should expect to parse.
  enum class ParserState {
    kOptionParsing = 0,  // parses options
    kItemParsing,        // parses official item names
    kWantlistParsing,    // parses wantlists
  };

  // Defines the result of a line parsing: a wantlist or an official item.
  ABSL_MUST_USE_RESULT absl::Status ParseLine(std::string_view line);

  // Parses a directive. Exactly one directive can be specified per line.
  // Example usage:
  //
  //    ParseOption("#!OPTION-1 OPTION-2 OPTION-3=value");
  ABSL_MUST_USE_RESULT absl::Status ParseDirective(std::string_view line);

  // Parses an option. Multiple options can be specified per line.
  // Example usage:
  //
  //    ParseOption("#!OPTION-1 OPTION-2 OPTION-3=value");
  ABSL_MUST_USE_RESULT absl::Status ParseOption(std::string_view line);

  // Parses a line defining an official item name and adds it to the items_ set.
  // Mandatory captures: item id.
  // Optional captures: username, official name, copies.
  // Returns an error if it fails to capture an item id.
  // Example usage:
  //
  //    ParseItem(R"(0001-MKBG ==> "Mage Knight: Board Game" (by user))");
  ABSL_MUST_USE_RESULT absl::Status ParseItem(std::string_view line);

  // Parses a wantlist, generates a Wantlist message and adds it to the
  // wantlists_ member.
  // Example usage:
  //
  //    ParseWantlist("0001-MKBG : 0002-PANDE 0003-TTAANSOC 0004-SCYTHE");
  ABSL_MUST_USE_RESULT absl::Status ParseWantlist(std::string_view line);

  // Propagates data from the data members to the trade_request.
  void FinalizeTradeRequest();

  // Defines the lines that the parser ignores.
  const re2::RE2 kIgnoreLineRegex;

  // Internal parser processing an official item name.
  ItemParser items_parser_;

  // Internal parser processing a specific wantlist.
  WantlistParser wantlist_parser_;

  ParserState state_ = ParserState::kOptionParsing;
  int64_t line_count_ = 0;  // next line number to process for error reporting

  // Indicates whether official item names have been given.
  bool has_official_names_ = false;

  // Tracks the given directives and the line number where the directives were
  // first defined.
  absl::flat_hash_map<std::string, int32_t> directives_;

  // Tracks the users with offered items and/or with declared official items.
  absl::flat_hash_set<std::string> users_;

  // Tracks the line number of each offered item's wantlist. Used to check
  // against double wantlists.
  absl::flat_hash_map<std::string, int32_t> wantlist_of_item_;

  // Tracks the missing items, defined as the wanted items without an official
  // name. Each item maps to its occurence across the entire parser input.
  absl::flat_hash_map<std::string, int32_t> missing_items_;

  // The trade input to return.
  TradeRequest trade_request_;
};
}  // namespace mathtrader::parser::internal
#endif  // MATHTRADER_PARSER_INTERNAL_INTERNAL_PARSER_H_
