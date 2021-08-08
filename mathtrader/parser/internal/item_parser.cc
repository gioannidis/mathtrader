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

#include "mathtrader/parser/internal/item_parser.h"

#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "glog/logging.h"

#include "mathtrader/common/offered_item.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/util/item_util.h"
#include "mathtrader/util/str_toupper.h"

namespace mathtrader::parser::internal {
namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::OfferedItem;
using ::mathtrader::util::StrToUpper;

// Internal regex that matches a text line defining an official item.
constexpr char kOfficialItemRegexStr[]
    // Prefix matching at the beginning of the text.
    = "^"

      // Captures item id. Stops at a ':' character or at a whitespace. Also
      // filters leading and trailing whitespaces.
      R"regex(\s*([^:\s]+)\s*)regex"

      // Filters optional colon character.
      R"regex(:?\s*)regex"

      // Optional arrow pointing at official name and/or username.
      R"regex((?:==>)?\s*)regex"

      // Optional: item number on some wantlists, e.g., "42.".
      R"regex((?:\d+\.)?\s*)regex"

      // Captures optional official name within quotation marks. The quotation
      // marks are not captured. The official name itself is allowed to contain
      // quotation marks or whitespaces, which are captured.
      R"regex((?:"(.+)")?\s*)regex"

      // Captures optional username. Expected format:
      //    (from USERNAME)
      //
      // Only the USERNAME is captured. The expected username format is:
      //    WWWW...
      // Where:
      //    W = word character: A-Z, a-z, 0-9, underscore (_)
      //    Minimum: 4 characters
      // Note: new usernames must begin with an alpha character, but older
      // usernames may not conform to this rule.
      R"regex((?:\(from\s+(.+)\))?\s*)regex"

      // Captures optional copy ids. Expected format:
      //    [copy 1 of 10]
      R"regex((?:\[copy\s+(\d+)\s+of\s+(\d+)\])?)regex";
}  // namespace

// Constructs the parser and dies if the regular expression was not created
// properly.
ItemParser::ItemParser() : kOfficialItemRegex(kOfficialItemRegexStr) {
  CHECK(kOfficialItemRegex.ok()) << kOfficialItemRegex.error();
}

// Parses the input text and returns an Item on success. If it fails, returns an
// InvalidArgumentError.
absl::StatusOr<Item> ItemParser::ParseItem(absl::string_view text) const {
  // Mandatory match: item id.
  std::string item_id;

  // Optional matches: official name and username.
  std::string official_name;
  std::string username;

  // Copy id and number of copies in string format. We use strings because they
  // are optionally matched. If they don't, they are set to empty strings.
  std::string copy_id_str;
  std::string num_copies_str;

  // Matches the text and returns an error on failure. The item_id must match;
  // all other matches are optional.
  if (!re2::RE2::PartialMatch(text, kOfficialItemRegex, &item_id,
                              &official_name, &username, &copy_id_str,
                              &num_copies_str)) {
    return absl::InvalidArgumentError("Could not extract official item id.");
  }

  // Retrieves the substring corresponding to the item id. Returns if empty or
  // if the item corresponds to a dummy item.
  if (item_id.empty()) {
    return absl::InvalidArgumentError(
        "Empty official item name is not allowed.");
  } else if (parser::util::IsDummyItem(item_id)) {
    // TODO(gioannidis) remove 'parser' qualifier once InternalParser has been
    // moved in this namespace.
    return absl::InvalidArgumentError(absl::StrCat(
        "Specifying dummy item name as official item name is not allowed: ",
        item_id));
  }

  // The item to be returned.
  Item item;

  // Sets the item-id, case-insensitive.
  item.set_id(StrToUpper(item_id));

  // Sets any available official data.
  if (!username.empty()) {
    // Makes the username case-insensitive.
    item.set_username(StrToUpper(username));
  }
  if (!official_name.empty()) {
    item.SetExtension(OfferedItem::official_name, official_name);
  }
  if (!copy_id_str.empty()) {
    int64_t copy_id;
    CHECK(absl::SimpleAtoi(copy_id_str, &copy_id));
    item.SetExtension(OfferedItem::copy_id, copy_id);
  }
  if (!num_copies_str.empty()) {
    int64_t num_copies;
    CHECK(absl::SimpleAtoi(num_copies_str, &num_copies));
    item.SetExtension(OfferedItem::num_copies, num_copies);
  }
  return item;
}
}  // namespace mathtrader::parser::internal
