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

#include "absl/strings/str_cat.h"
#include "ortools/base/logging.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/offered_item.pb.h"

namespace mathtrader::parser::util {
// Makes the id of the dummy item unique by appending the username of its owner
// in order to disambiguify it from similarly-named dummy items of other users.
// Does nothing if the item is non-dummy. Dies if a dummy item has no username.
void UniquifyDummyItem(Item* item) {
  CHECK_NOTNULL(item);
  if (IsDummyItem(item)) {
    const std::string_view username = item->GetExtension(OfferedItem::username);
    CHECK(!username.empty()) << "Empty username for dummy item " << item->id()
        << " not allowed.";
    absl::StrAppend(item->mutable_id(), "-", username);
  }
}
}  // namespace mathtrader::parser::util
