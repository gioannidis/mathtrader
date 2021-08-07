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

#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "ortools/base/logging.h"

#include "mathtrader/common/item.pb.h"

namespace mathtrader::parser::util {
// Processes the item if it is a dummy. Makes the id unique by appending the
// username of its owner in order to disambiguify it from similarly-named dummy
// items of other users. Sets the `is_dummy` field and copies the original id
// to the `unmodified_id` field. Does nothing if the item is non-dummy.
// Returns `InvalidArgumentError` if a dummy item has no username.
absl::Status ProcessIfDummy(std::string_view username, Item* item) {
  CHECK_NOTNULL(item);
  if (IsDummyItem(item)) {
    // Sets the field, in case the item has a dummy id, but the field is not yet
    // set.
    item->set_is_dummy(true);

    if (username.empty()) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Missing or empty username for item %s. (Tip: this usually indicates "
          "that the username is missing from the wantlist.)",
          item->id()));
    }
    item->set_unmodified_id(item->id());
    absl::StrAppend(item->mutable_id(), "-", username);
  }
  return absl::OkStatus();
}

absl::Status ProcessIfDummy(Item* item) {
  CHECK_NOTNULL(item);
  return ProcessIfDummy(item->username(), item);
}
}  // namespace mathtrader::parser::util
