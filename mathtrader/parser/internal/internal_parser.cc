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

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ortools/base/map_util.h"
#include "re2/re2.h"

#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/internal/internal_wantlist.pb.h"
#include "mathtrader/parser/parser_result.pb.h"
#include "mathtrader/parser/util/item_util.h"

namespace mathtrader::parser::internal {
namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::Wantlist;

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

absl::Status InternalParser::ParseLine(absl::string_view line) {
  // Skips lines that should be ignored. Use of PartialMatch is recommended over
  // FullMatch as it deals better with unicode characters in comment lines, such
  // as the pound (GBP) character.
  if (re2::RE2::PartialMatch(line, kIgnoreLineRegex)) {
    return absl::OkStatus();
  }

  // Strips the prefix of the line, if it's a special line, e.g., option.
  absl::string_view stripped_line = StripPrefix(line);

  // The return status.
  absl::Status status = absl::OkStatus();

  if (IsOptionLine(line)) {
    status.Update(ParseOption(stripped_line));
  } else if (IsDirectiveLine(line)) {
    status.Update(ParseDirective(stripped_line));
  } else if (!line.empty()) {
    // Empty lines or comments are ignored.
    // Any other line: dependent on current state.
    switch (state_) {
      case ParserState::kOptionParsing: {
        state_ = ParserState::kWantlistParsing;
        status.Update(ParseWantlist(line));
        break;
      }
      case ParserState::kWantlistParsing: {
        status.Update(ParseWantlist(line));
        break;
      }
      case ParserState::kItemParsing: {
        status.Update(ParseItem(line));
        break;
      }
      default: {
        return absl::InternalError(
            absl::StrFormat("Internal error during Parser::State %d.", state_));
      }
    }
  }
  // In case of an error, append the line number where the error occurred.
  if (!status.ok()) {
    return absl::Status(
        status.code(),
        absl::StrFormat("(line %d) %s", line_count_, status.message()));
  }
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

  } else if (const std::string& id = item->id(); !gtl::InsertIfNotPresent(
                 parser_result_.mutable_items(), id, *item)) {
    // Failed to insert the item; already exists.
    return absl::AlreadyExistsError(absl::StrFormat(
        "Duplicate declaration of official item %s not allowed.", id));
  }

  // Registers the username.
  if (absl::string_view username = item->username(); !username.empty()) {
    gtl::InsertIfNotPresent(&users_, static_cast<std::string>(username));
  }

  return absl::OkStatus();
}

namespace {
// Retrieves the `unmodified_id` field if set, otherwise `id`.
const std::string& GetUnmodifiedId(const Item& item) {
  return (item.has_unmodified_id() ? item.unmodified_id() : item.id());
}

// Identifies and removes duplicate items from a wantlist, reporting them to
// parser_result.
void RemoveDuplicateItems(Wantlist* wantlist, ParserResult* parser_result) {
  CHECK_NOTNULL(wantlist);
  CHECK_NOTNULL(parser_result);

  // Tracks the frequency of each wanted item in the wantlist.
  // key: item ID in wantlist.
  // mapped: frequency
  absl::flat_hash_map<std::string, int32_t> frequencies;

  // Tracks the actual items that were found to be duplicates.
  // key: item ID in wantlist.
  absl::flat_hash_set<std::string> duplicates;

  // Erases items that occur 2+ times in the wantlist. Updates `frequencies` for
  // all items and `duplicates` for repeated items.
  auto* const wanted_items = wantlist->mutable_wanted();
  wanted_items->erase(
      std::remove_if(
          wanted_items->begin(), wanted_items->end(),

          // Lambda: decides whether an item should be erased.
          [&frequencies, &duplicates](const Wantlist::WantedItem& wanted_item) {
            const std::string& id = wanted_item.id();

            // Retrieves the item's frequency, if previously defined,
            // otherwise initializes it.
            int32_t& frequency =
                gtl::LookupOrInsert(&frequencies, id, /*frequency=*/0);
            ++frequency;

            // Removes item if we have already encountered it, adding
            // it to `duplicates`.
            const bool is_duplicate = (frequency > 1);
            if (is_duplicate) {
              gtl::InsertIfNotPresent(&duplicates, id);
            }
            return is_duplicate;
          }),
      wanted_items->end());  // 2nd argument of `erase()`.

  // Populates the `parser_result` with the duplicate items that appear 2+ times
  // in the wantlist.
  for (const std::string& duplicate_id : duplicates) {
    // The offered item and the duplicate wanted item must both exist, because
    // the wantlist has been fully parsed by this point.
    const Item& offered_item =
        gtl::FindOrDie(parser_result->items(), wantlist->offered());
    const Item& wanted_item =
        gtl::FindOrDie(parser_result->items(), duplicate_id);

    // Creates a new duplicate wanted item.
    auto* const duplicate_item = parser_result->add_duplicate_wanted_items();
    duplicate_item->set_offered_item_id(offered_item.id());
    duplicate_item->set_wanted_item_id(wanted_item.id());

    // Retrieves the frequency from the `frequencies` map and sets it.
    const int32_t frequency = gtl::FindOrDie(frequencies, duplicate_id);
    CHECK_GT(frequency, 1);
    duplicate_item->set_frequency(frequency);
  }
}

// Identifies and removes missing items from a wantlist, reporting them to
// `missing_items`.
void RemoveMissingItems(
    const google::protobuf::Map<std::string, Item>& official_items,
    Wantlist* wantlist,
    absl::flat_hash_map<std::string, int32_t>* missing_items) {
  CHECK_NOTNULL(wantlist);
  CHECK_NOTNULL(missing_items);
  auto* const wanted_items = wantlist->mutable_wanted();

  // Checks if a wanted item is missing and, if so, records it as a missing
  // item in `parser_result_`.
  wanted_items->erase(
      std::remove_if(
          wanted_items->begin(), wanted_items->end(),

          // Lambda: decides whether an item should be erased.
          [&official_items,
           missing_items](const Wantlist::WantedItem& wanted_item) {
            const std::string& id = wanted_item.id();

            // Does not erase the item if it is dummy or if it has
            // an official name.
            if (util::IsDummyItem(id) || official_items.contains(id)) {
              return false;
            }
            // Initializes the frequency of the missing item to zero
            // or retrieves it if present.
            int32_t& frequency =
                gtl::LookupOrInsert(missing_items, id, /*frequency=*/0);
            ++frequency;
            return true;
          }),
      wanted_items->end());  // 2nd argument of `erase()`.
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
  const std::string& offered_id = wantlist->offered();
  const std::string& username =
      wantlist->GetExtension(InternalWantlist::username);

  // Registers the username, if it has been given. An empty username is possible
  // if no official names where previously given and this is the first wantlist
  // for this username.
  if (!username.empty()) {
    gtl::InsertIfNotPresent(&users_, username);
  }

  // Registers the wantlist and verifies that no other wantlist has been
  // declared.
  if (const auto& [it, inserted] =
          wantlist_of_item_.emplace(offered_id, line_count_);
      !inserted) {
    // Reports the line number of the existing wantlist.
    const Item& duplicate = gtl::FindOrDie(parser_result_.items(), offered_id);
    return absl::AlreadyExistsError(absl::StrFormat(
        "Cannot declare multiple wantlists for item %s%s. Previous wantlist "
        "declared in line %d.",
        GetUnmodifiedId(duplicate),
        // Displays the user offering the item if available; otherwise empty.
        duplicate.has_username()
            ? absl::StrCat(" from user: ", duplicate.username())
            : "",
        it->second));
  }

  // Handles the offered item:
  // * Dummy: creates and registers.
  // * Non-dummy without official names: creates and registers.
  // TODO(gioannidis) merge these two cases if possible.
  // * Non-dummy with official names: checks if official name has been given.
  if (util::IsDummyItem(offered_id)) {
    // Creates and registers the offered dummy item id. It may be already
    // present, if the dummy item has been listed as a wanted item in a previous
    // wantlist.
    // TODO(gioannidis) At the end check if every dummy item has a wantlist.
    gtl::InsertIfNotPresent(parser_result_.mutable_items(), offered_id,
                            util::MakeItem(offered_id, username));

  } else if (!has_official_names_) {
    // Creates and registers the non-dummy offered item. Because no official
    // names have been, it is allowed for the offered item to be previously
    // undeclared. It is possible that the offered item exists if it has been
    // previously defined as a wanted item in another wantlist.
    // Overwrites username if it exists, which may be empty.
    // TODO(gioannidis) if it overwrites, ensure that we update an empty name.
    gtl::InsertOrUpdate(parser_result_.mutable_items(), offered_id,
                        util::MakeItem(offered_id, username));
  } else {
    // Verifies that the non-dummy offered item exists, because official names
    // have been already declared.
    if (!parser_result_.items().contains(offered_id)) {
      return absl::NotFoundError(absl::StrFormat(
          "Missing official name for offered item %s.", offered_id));
    }
  }

  // Registers all dummy wanted items if not already present. A dummy item is
  // allowed to appear first as a wanted item and subsequently as an offered
  // item, because some dummy lists are auto-generated, e.g., when multiple
  // COPY items are given.
  for (const Wantlist::WantedItem& wanted_item : wantlist->wanted()) {
    const std::string_view wanted_id = wanted_item.id();
    if (util::IsDummyItem(wanted_id)) {
      gtl::InsertIfNotPresent(parser_result_.mutable_items(),
                              std::string(wanted_id),
                              util::MakeItem(std::string(wanted_id), username));
    }
  }

  // Removes all non-dummy wanted items without official name.
  if (has_official_names_) {
    // Note: `wantlist` is StatusOr, so we dereference the union first and then
    // pass by pointer.
    RemoveMissingItems(parser_result_.items(), &(*wantlist), &missing_items_);
  } else {
    // Creates and registers all wanted items. Since no official names have been
    // given, it is possible that some wanted items are declared now for the
    // first time.
    for (const Wantlist::WantedItem& wanted : wantlist->wanted()) {
      const std::string& id = wanted.id();
      gtl::InsertIfNotPresent(parser_result_.mutable_items(), id,
                              util::MakeItem(id, username));
    }
  }

  // Identifies and removes duplicate items that have an official name. This
  // should be done after items without an official named have been removed, so
  // as to reduce redundant errors. Note: `wantlist` is StatusOr, so we
  // dereference the union first and then pass by pointer.
  RemoveDuplicateItems(&(*wantlist), &parser_result_);

  // Finally, clears any internal extensions.
  wantlist->ClearExtension(InternalWantlist::username);

  (*parser_result_.add_wantlists()) = std::move(*wantlist);
  return absl::OkStatus();
}

// Propagates data from the data members to the parser_result.
void InternalParser::FinalizeParserResult() {
  // Sets the final item count. Counts non-dummy items.
  parser_result_.set_item_count(std::count_if(
      parser_result_.items().cbegin(), parser_result_.items().cend(),
      [](const auto& map_pair) { return !map_pair.second.is_dummy(); }));

  // Moves the usernames to parser_result.
  while (!users_.empty()) {
    auto internal_node = users_.extract(users_.begin());
    parser_result_.add_users(std::move(internal_node.value()));
  }

  // Moves the missing items to parser_result.
  while (!missing_items_.empty()) {
    ParserResult_MissingItem* missing_item = parser_result_.add_missing_items();

    // Internal node: maps item_id -> frequency.
    auto internal_node = missing_items_.extract(missing_items_.begin());
    missing_item->set_item_id(std::move(internal_node.key()));
    missing_item->set_frequency(internal_node.mapped());
  }
}
}  // namespace mathtrader::parser::internal
