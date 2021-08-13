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

#include "mathtrader/parser/util/item_util.h"

#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "ortools/base/logging.h"

#include "mathtrader/common/item.pb.h"

namespace mathtrader::parser::util {
namespace {
using ::mathtrader::common::Item;

// Processes the item id, assuming that it already represents a dummy id.
absl::Status InternalProcessDummy(std::string_view username,
                                  std::string* item_id) {
  CHECK_NOTNULL(item_id);
  if (username.empty()) {
    return absl::NotFoundError(absl::StrFormat(
        "Missing or empty username for item %s. (Tip: this usually indicates "
        "that the username is missing from the wantlist.)",
        *item_id));
  }

  // Appends the username to the item_id if it is not already there. Applies the
  // operation if the username and the item_id are identical.
  if (!absl::EndsWith(*item_id, username) || (*item_id == username)) {
    absl::StrAppend(item_id, "-", username);
  }
  return absl::OkStatus();
}
}  // namespace

// Processes the item id if represents a dummy item. Makes the id unique by
// appending the username of its owner in order to disambiguify it from
// similarly-named dummy items of other users. Does nothing if the item is
// non-dummy. Returns `NotFoundError` if a dummy item has no username.
absl::Status ProcessIfDummy(std::string_view username, std::string* item_id) {
  CHECK_NOTNULL(item_id);
  if (!IsDummyItem(*item_id)) {
    // Skips non-dummy items.
    return absl::OkStatus();
  }
  return InternalProcessDummy(username, item_id);
}

// As above, but operates on an item.  Sets the `is_dummy` field and copies the
// original id to the `original_id` field. Does nothing if the item is
// non-dummy. Returns `NotFoundError` if a dummy item has no username.
absl::Status ProcessIfDummy(std::string_view username, Item* item) {
  CHECK_NOTNULL(item);
  if (!IsDummyItem(*item)) {
    // Skips non-dummy items.
    return absl::OkStatus();
  }

  // Caches the unmodified id before processing.
  item->set_unmodified_id(item->id());
  item->set_is_dummy(true);

  return InternalProcessDummy(username, item->mutable_id());
}

absl::Status ProcessIfDummy(Item* item) {
  CHECK_NOTNULL(item);
  return ProcessIfDummy(item->username(), item);
}
}  // namespace mathtrader::parser::util
