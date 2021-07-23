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

// Defines common functions defining item attributes.

#ifndef MATHTRADER_COMMON_ITEM_ATTRIBUTES_H_
#define MATHTRADER_COMMON_ITEM_ATTRIBUTES_H_

#include "mathtrader/common/item.pb.h"

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"

namespace mathtrader {

// Determines whether the given item_id represents a dummy item. Filters any
// leading whitespaces from the item_id before checking.
inline bool IsDummyItem(absl::string_view item_id) {
  item_id.remove_prefix(item_id.find_first_not_of(" \t\r\n"));
  return absl::StartsWith(item_id, "%");
}

// Determines whether the given item is a dummy item.
inline bool IsDummyItem(const Item& item) {
  return IsDummyItem(item.id());
}

// Determines whether the given item is a dummy item. Returns false if null.
inline bool IsDummyItem(const Item* item) {
  return (item && IsDummyItem(item->id()));
}

}  // namespace mathtrader

#endif  // MATHTRADER_COMMON_ITEM_ATTRIBUTES_H_
