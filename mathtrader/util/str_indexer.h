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

// Defines an indexer for strings.

#ifndef MATHTRADER_UTIL_STR_INDEXER_H_
#define MATHTRADER_UTIL_STR_INDEXER_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

namespace mathtrader::util {
// Builds unique indexes for a span of N strings without duplicates. The indexes
// are guaranteed to be in the range [0, N). There is a 1:1 mapping between the
// N strings and the N indexes. Do not depend on a specific mapping between
// indexes and strings, e.g., assume that the first string is mapped to index
// '0'. Example usage:
//
//   StrIndexer indexer;
//   indexer.BuildIndexes({"abcd", "1234", "QWERTY0123#foo"});
//
//   const int32_t index = indexer.IndexOrDie("abcd");
//   const std::string_view mapped = indexer.ValueOrDie(index);
//   CHECK_EQ(mapped, "abcd");
class StrIndexer {
 public:
  StrIndexer() = default;

  // Disables copy constructor and assignment operator.
  StrIndexer(const StrIndexer&) = delete;
  StrIndexer& operator=(const StrIndexer&) = delete;

  ~StrIndexer() = default;

  // Builds indexes for the given string span. Discards any existing stored
  // indexes and strings.
  void BuildIndexes(absl::Span<const std::string_view> strings);

  // Finds and returns the string for a given index. Dies if not found.
  ABSL_MUST_USE_RESULT const std::string& ValueOrDie(int32_t index) const;

  // Finds and returns the index for a given string. Dies if not found.
  ABSL_MUST_USE_RESULT int32_t IndexOrDie(std::string_view value) const;

  // Returns the size of the indexer which corresponds to both the number of
  // stored indexes and the number of mapped strings.
  ABSL_MUST_USE_RESULT size_t size() const { return index_to_str_.size(); }

 private:
  // Maps an index to its mapped std::string.
  absl::flat_hash_map<int32_t, std::string> index_to_str_;

  // Reverse mapping of a std::string to its index.
  absl::flat_hash_map<std::string, int32_t> str_to_index_;
};
}  // namespace mathtrader::util
#endif  // MATHTRADER_UTIL_STR_INDEXER_H_
