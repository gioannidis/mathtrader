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
#include <string_view>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "glog/logging.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/offered_item.pb.h"
#include "mathtrader/parser/util/item_util.h"
#include "mathtrader/util/str_toupper.h"

namespace mathtrader::parser::internal {
namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::OfferedItem;
using ::mathtrader::util::StrToUpper;

// Captures item id at the beginning of the text. Stops at a ':' character or at
// a whitespace. Filters any leading whitespaces.
static constexpr std::string_view kItemIdRegexStr =
    R"regex(^\s*([^:\s]+))regex";

// Captures official name within quotation marks. The quotation marks are not
// captured. The official name itself is allowed to contain quotation marks or
// whitespaces, which are captured.
// Note: official names with unicode characters may not be properly parsed.
static constexpr std::string_view kOfficialNameRegexStr = R"regex("(.+)")regex";

// Captures optional username. Expected format:
//    (from USERNAME)
//
// Only the USERNAME is captured. The expected username format is:
//    WWWW...
// Where:
//    W = word character: A-Z, a-z, 0-9, underscore (_)
//    Minimum: 4 characters
//
// Note: new usernames must begin with an alpha character, but older
// usernames may not conform to this rule.
// Note: we don't enforce the expected username format.
static constexpr std::string_view kFromUsernameRegexStr =
    R"regex(\(from\s+(.+)\))regex";

// Captures optional copy ids. Expected format:
//    [copy 1 of 10]
static constexpr std::string_view kCopiesRegexStr =
    R"regex(\[copy\s+(\d+)\s+of\s+(\d+)\])regex";
}  // namespace

// Constructs the parser and dies if the regular expressions were not created
// properly.
ItemParser::ItemParser()
    : kItemIdRegex(kItemIdRegexStr),
      kOfficialNameRegex(kOfficialNameRegexStr),
      kFromUsernameRegex(kFromUsernameRegexStr),
      kCopiesRegex(kCopiesRegexStr) {
  CHECK(kItemIdRegex.ok()) << kItemIdRegex.error();
  CHECK(kOfficialNameRegex.ok()) << kOfficialNameRegex.error();
  CHECK(kFromUsernameRegex.ok()) << kFromUsernameRegex.error();
  CHECK(kCopiesRegex.ok()) << kCopiesRegex.error();
}

// Parses the input text and returns an Item on success. If it fails, returns an
// InvalidArgumentError.
absl::StatusOr<Item> ItemParser::ParseItem(std::string_view text) const {
  // Mandatory match: item id.
  std::string item_id;

  // Captures the item id and returns an error on failure. The item id must be
  // present.
  if (!re2::RE2::PartialMatch(text, kItemIdRegex, &item_id)) {
    return absl::InvalidArgumentError("Could not extract official item id.");
  }

  // Retrieves the substring corresponding to the item id. Returns if empty or
  // if the item corresponds to a dummy item.
  if (item_id.empty()) {
    return absl::InvalidArgumentError(
        "Empty official item name is not allowed.");
  } else if (util::IsDummyItem(item_id)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Specifying dummy item name as official item name is not allowed: ",
        item_id));
  }

  // The item to be returned.
  Item item;

  // Sets the item-id, case-insensitive.
  item.set_id(StrToUpper(item_id));

  // Sets optional username.
  {
    std::string username;
    if (re2::RE2::PartialMatch(text, kFromUsernameRegex, &username)) {
      CHECK(!username.empty())
          << "Username cannot be empty; indicates a regex error";

      // Makes the username case-insensitive.
      item.set_username(StrToUpper(username));
    }
  }

  // Sets optional official name.
  {
    std::string official_name;
    if (re2::RE2::PartialMatch(text, kOfficialNameRegex, &official_name)) {
      item.SetExtension(OfferedItem::official_name, official_name);
    }
  }

  // Sets the optional copy id and number of copies.
  {
    int32_t copy_id = 0;
    int32_t num_copies = 0;
    if (re2::RE2::PartialMatch(text, kCopiesRegex, &copy_id, &num_copies)) {
      item.SetExtension(OfferedItem::copy_id, copy_id);
      item.SetExtension(OfferedItem::num_copies, num_copies);
    }
  }
  return item;
}
}  // namespace mathtrader::parser::internal
