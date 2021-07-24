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

// Implements most of the parsing functions, except for special lines.

#include "mathtrader/parser/internal/internal_parser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ortools/base/map_util.h"
#include "re2/re2.h"

#include "mathtrader/common/item_attributes.h"
#include "mathtrader/common/wantlist.pb.h"

namespace mathtrader::internal_parser {

namespace {

// Prefix for directive lines.
constexpr absl::string_view kPrefixDirective = "!";
// Prefix for option lines.
constexpr absl::string_view kPrefixOption = "#!";

// Checks whether this line is a directive line.
bool IsDirectiveLine(absl::string_view line) {
  return absl::StartsWith(line, kPrefixDirective);
}

// Checks whether this line is an option line.
bool IsOptionLine(absl::string_view line) {
  return absl::StartsWith(line, kPrefixOption);
}

// Removes special line prefixes so that they can be subsequently parsed.
// Note that the argument must outlive the result.
absl::string_view StripPrefix(absl::string_view line) {
  if (IsDirectiveLine(line)) {
    line.remove_prefix(kPrefixDirective.size());
  } else if (IsOptionLine(line)) {
    line.remove_prefix(kPrefixOption.size());
  }
  return line;
}

}  // namespace

absl::Status InternalParser::ParseLine(
    absl::string_view line) {
  // Skips lines that should be ignored.
  if (re2::RE2::FullMatch(line, kIgnoreLineRegex)) {
    return absl::OkStatus();
  }

  // Strips the prefix of the line, if it's a special line, e.g., option.
  absl::string_view stripped_line = StripPrefix(line);

  // The return status.
  absl::Status status = absl::OkStatus();

  if (IsOptionLine(line)) {
    status.Update(ParseOption(stripped_line));;
  } else if (IsDirectiveLine(line)) {
    status.Update(ParseDirective(stripped_line));;
  } else if (!line.empty()) {
    // Empty lines or comments are ignored.
    // Any other line: dependent on current state.
    switch (state_) {
      case ParserState::kOptionParsing: {
        state_ = ParserState::kWantlistParsing;
        status.Update(ParseWantlist(line));;
        break;
      }
      case ParserState::kWantlistParsing: {
        status.Update(ParseWantlist(line));;
        break;
      }
      case ParserState::kItemParsing: {
        status.Update(ParseItem(line));
        break;
      }
      default: {
        return absl::InternalError(absl::StrFormat(
            "Internal error during Parser::State %d.",
            state_));
      }
    }
  }
  // In case of an error, append the line number where the error occurred.
  if (!status.ok()) {
    return absl::Status(
        status.code(),
        absl::StrFormat("(line %d) %s", line_count_, status.message()));
  }

  // Sets the final user count, if available.
  if (!users_.empty()) {
    parser_result_.set_user_count(users_.size());
  }

  // Sets the final item count.
  parser_result_.set_item_count(items_.size());

  return absl::OkStatus();
}

// Parses an option. Multiple options can be specified per line.
// Example usage:
//
//    ParseOption("#!OPTION-1 OPTION-2 OPTION-3=value");
absl::Status InternalParser::ParseOption(absl::string_view line) {
  return absl::OkStatus();
}

// Parses a line defining an official item name and adds it to the items_ set.
// Mandatory captures: item id.
// Optional captures: username, official name, copies.
// Returns an error if it fails to capture an item id.
// Example usage:
//
//    ParseItem(R"(0001-MKBG ==> "Mage Knight: Board Game" (by user))");
absl::Status InternalParser::ParseItem(absl::string_view line) {
  const auto item = items_parser_.ParseItem(line);
  if (!item.ok()) {
    return item.status();

  } else if (const std::string id = item->id();
             !gtl::InsertIfNotPresent(&items_, id, *item)) {
    // Failed to insert the item; already exists.
    return absl::InvalidArgumentError(absl::StrFormat(
        "Duplicate declaration of official item %s not allowed.",
        id));
  }

  // Registers the username.
  if (absl::string_view username = item->official_data().username();
      !username.empty()) {
    gtl::InsertIfNotPresent(&users_, static_cast<std::string>(username));
  }

  return absl::OkStatus();
}

namespace {
// Retrieves the id of an item, which belongs to a given username (optional).
// If the item is a dummy, it appends the username of its owner to the item id,
// in order to disambiguify it from similarly-named dummy items. In this case, a
// username must exist.
std::string GetProperItemId(const Item& item, const std::string& user) {
  // The item id to return.
  std::string id = item.id();
  if (IsDummyItem(id)) {
    // Retrieves the username and the line id where the username was first
    // defined. At this point, it is guaranteed that these operations succeed.
    const std::string& username = item.official_data().username();
    CHECK(!username.empty());

    // Appends the line number as a unique id.
    id.push_back('-');
    id += username;
  }
  return id;
}
}  // namespace

// Parses a wantlist, generates a Wantlist message and adds it to the
// wantlists_ member.
// Example usage:
//
//    ParseWantlist("0001-MKBG : 0002-PANDE 0003-TTAANSOC 0004-SCYTHE");
absl::Status InternalParser::ParseWantlist(absl::string_view line) {
  auto wantlist = wantlist_parser_.ParseWantlist(line);
  if (!wantlist.ok()) {
    return wantlist.status();
  }

  // The offered item, as retrieved from the wantlist. Can optionally define a
  // username.
  const Item& offered_item = wantlist->offered_item();

  // Registers the username, if it has been given. This is possible if no
  // official names where previously given and this is the first wantlist for
  // this username.
  if (absl::string_view username = offered_item.official_data().username();
      !username.empty()) {
    gtl::InsertIfNotPresent(&users_, static_cast<std::string>(username));
  }

  // The "raw", i.e., as specified, id of the offered item.
  const absl::string_view raw_offered_id = offered_item.id();

  // The "proper" id of the offered item; username is appended if dummy.
  const std::string offered_id = GetProperItemId(
      offered_item,
      offered_item.official_data().username());

  // Registers the wantlist and verifies that no other wantlist has been
  // declared.
  if (const auto& [it, inserted] = wantlist_of_item_.emplace(offered_id,
                                                             line_count_);
      !inserted) {
    // Reports the line number of the existing wantlist.
    return absl::InvalidArgumentError(absl::StrFormat(
        "Cannot declare multiple wantlists for item %s. Previous wantlist "
        "declared in line %d.",
        raw_offered_id,
        it->second));
  }

  // Registers the offered item or retrieves the official data from an already
  // registered official item.
  if (IsDummyItem(raw_offered_id)) {
    // Registers the offered dummy item id. Must succeed, as this is the first
    // wantlist of the item.
    CHECK(gtl::InsertIfNotPresent(&dummy_items_, offered_id));

  } else if (!has_official_names_) {
    // Registers the non-dummy offered item. Must succeed, as this is the first
    // wantlist of the item and no official names have been previously declared.
    CHECK(gtl::InsertIfNotPresent(&items_, offered_id, offered_item));

  } else {
    // Verifies that the non-dummy offered item exists, because official names
    // have been already declared.
    if (const Item* existing_item = gtl::FindOrNull(items_, offered_id);
        !existing_item) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Missing official name for offered item %s.", offered_id));
    }
  }

  // Removes all non-dummy wanted items without official name.
  if (has_official_names_) {
    auto* const wanted_items = wantlist->mutable_wanted_item();

    // Checks if a wanted item is missing and, if so, records it as a missing
    // item in `parser_result_`.
    wanted_items->erase(
        std::remove_if(
            wanted_items->begin(), wanted_items->end(),
            [this](const Item& wanted_item) {
                const std::string& id = wanted_item.id();

                if (IsDummyItem(id)) {
                  // Does not erase the item, because it is dummy.
                  return false;

                // Checks whether the item has an official name.
                } else if (gtl::FindOrNull(items_, id)) {
                  // Does not erase the item, because it has been found.
                  return false;
                }
                // Initializes the frequency of the missing item to zero or
                // retrieves it if present.
                int32_t& frequency =
                    gtl::LookupOrInsert(parser_result_.mutable_missing_items(),
                                        id, /*frequency=*/0);
                ++frequency;
                return true;
            }),
        wanted_items->end());
  }

  (*parser_result_.add_wantlist()) = std::move(*wantlist);
  return absl::OkStatus();
}

}  // namespace mathtrader::internal_parser
