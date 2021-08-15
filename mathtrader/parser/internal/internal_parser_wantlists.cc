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

#include <string>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/internal/internal_wantlist.pb.h"
#include "mathtrader/parser/util/item_util.h"

namespace mathtrader::parser::internal {
namespace {
using ::mathtrader::common::Item;

// Retrieves the `unmodified_id` field if set, otherwise `id`.
const std::string& GetUnmodifiedId(const Item& item) {
  return (item.has_unmodified_id() ? item.unmodified_id() : item.id());
}

// Registers the offered item in the given item map by creating the appropriate
// Item from the given id and username (if applicable).
// * Dummy: creates and registers.
// * Non-dummy without official names: creates and registers.
// TODO(gioannidis) merge these two cases if possible.
// * Non-dummy with official names: checks if official name has been given.
ABSL_MUST_USE_RESULT absl::Status RegisterOfferedItem(
    std::string_view offered_id, std::string_view username, bool must_exist,
    google::protobuf::Map<std::string, Item>&
        item_map) {  // NOLINT(runtime/references)
  if (util::IsDummyItem(offered_id)) {
    // Creates and registers the offered dummy item id. It may be already
    // present, if the dummy item has been listed as a wanted item in a previous
    // wantlist.
    gtl::InsertIfNotPresent(&item_map, std::string(offered_id),
                            util::MakeItem(offered_id, username));

  } else if (!must_exist) {
    // Creates and registers the non-dummy offered item. Because no official
    // names have been, it is allowed for the offered item to be previously
    // undeclared. It is possible that the offered item exists if it has been
    // previously defined as a wanted item in another wantlist.
    // Overwrites username if it exists, which may be empty.
    // TODO(gioannidis) if it overwrites, ensure that we update an empty name.
    gtl::InsertOrUpdate(&item_map, std::string(offered_id),
                        util::MakeItem(offered_id, username));
  } else {
    // Verifies that the non-dummy offered item exists, because official names
    // have been already declared.
    if (!item_map.contains(offered_id)) {
      return absl::NotFoundError(absl::StrFormat(
          "Missing official name for offered item %s.", offered_id));
    }
  }
  return absl::OkStatus();
}
}  // namespace

// Parses a wantlist, generates a Wantlist message and adds it to the
// wantlists_ member.
// Example usage:
//
//    ParseWantlist("0001-MKBG : 0002-PANDE 0003-TTAANSOC 0004-SCYTHE");
absl::Status InternalParser::ParseWantlist(std::string_view line) {
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

  // Registers the offered item.
  if (const absl::Status status = RegisterOfferedItem(
          offered_id, username, /*must_exist=*/has_official_names_,
          *parser_result_.mutable_items());
      !status.ok()) {
    return status;
  }

  // Finally, clears any internal extensions.
  wantlist->ClearExtension(InternalWantlist::username);

  (*parser_result_.add_wantlists()) = std::move(*wantlist);
  return absl::OkStatus();
}
}  // namespace mathtrader::parser::internal
