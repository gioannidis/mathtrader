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

#include "mathtrader/util/str_indexer.h"

#include <cstdint>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "ortools/base/map_util.h"

namespace mathtrader::util {
void StrIndexer::BuildIndexes(absl::Span<const std::string_view> strings) {
  index_to_str_.clear();
  str_to_index_.clear();

  int32_t next_index = 0;
  for (const std::string_view str : strings) {
    gtl::InsertOrDie(&index_to_str_, next_index, std::string(str));
    gtl::InsertOrDie(&str_to_index_, std::string(str), next_index);
    ++next_index;
  }
  CHECK_EQ(str_to_index_.size(), index_to_str_.size());
}

const std::string& StrIndexer::ValueOrDie(int32_t index) const {
  return gtl::FindOrDie(index_to_str_, index);
}

int32_t StrIndexer::IndexOrDie(std::string_view value) const {
  return gtl::FindOrDie(str_to_index_, std::string(value));
}
}  // namespace mathtrader::util
