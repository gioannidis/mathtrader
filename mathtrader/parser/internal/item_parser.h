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

// Parser for lines defining official items.

#ifndef MATHTRADER_PARSER_INTERNAL_ITEM_PARSER_H_
#define MATHTRADER_PARSER_INTERNAL_ITEM_PARSER_H_

#include "absl/status/statusor.h"
#include "re2/re2.h"

#include "mathtrader/common/item.pb.h"

namespace mathtrader::parser::internal {
// Parses a line of text defining an official item name and returns an `Item`.
// Example usage:
//
//    ItemParser item_parser;
//    auto item_or = item_parser.ParseItem(text);
//    CHECK(item_or.ok());
//    Item item = std::move(*item_or);
//
// Expected text format for an item without copies:
//    1234-SOME1ID2 ==> "official name" (from username)
//
// Expected text format for an item with copies:
//    1234-SOMEID-COPY1 ==> "official name" (from username) [copy 1 of 3]
class ItemParser {
 public:
  // Constructs the parser and dies if the regular expression was not created
  // properly.
  ItemParser();

  // Disables copy constructor and assignment operator.
  ItemParser(const ItemParser&) = delete;
  ItemParser& operator=(const ItemParser&) = delete;

  // Parses the input text and returns an Item on success. If it fails, returns
  // an InvalidArgumentError.
  absl::StatusOr<common::Item> ParseItem(std::string_view text) const;

 private:
  // Matches a line declaring an official item. Note that is not declared as
  // static, as the destructor of RE2 is not trivial.
  const re2::RE2 kOfficialItemRegex;
};
}  // namespace mathtrader::parser::internal
#endif  // MATHTRADER_PARSER_INTERNAL_ITEM_PARSER_H_
