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

#include <array>
#include <string_view>

#include "gtest/gtest.h"

namespace {
using ::mathtrader::util::StrIndexer;

static constexpr std::array<std::string_view, 10> values = {
    "a", "b", "c", "d", "e", "12345", "", "_", "!@#$%", "foobar"};

// Verifies that each string is indexed and the index maps back to the same
// value.
TEST(StrIndexerTest, CanRetrieveStringFromIndex) {
  StrIndexer indexer;
  indexer.BuildIndexes(values);

  for (const std::string_view value : values) {
    const int32_t index = indexer.IndexOrDie(value);
    const auto& mapped = indexer.ValueOrDie(index);
    EXPECT_EQ(mapped, value);
  }
}

// Verifies that the indexer builds as many indexes as the input size.
TEST(StrIndexerTest, IndexerSizeEqualsInputSize) {
  StrIndexer indexer;
  indexer.BuildIndexes(values);
  EXPECT_EQ(indexer.size(), values.size());
}

// Verifies that if we call `BuildIndexes` again, the older values are
// discarded.
TEST(StrIndexerTest, NewBuildDiscardsOldValues) {
  static constexpr std::array<std::string_view, 3> other_values = {"1", "2",
                                                                   "3"};
  StrIndexer indexer;
  indexer.BuildIndexes(values);
  indexer.BuildIndexes(other_values);
  EXPECT_EQ(indexer.size(), other_values.size());
}
}  // namespace
