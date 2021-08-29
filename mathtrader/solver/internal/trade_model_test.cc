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

#include "mathtrader/solver/internal/trade_model.h"

#include <array>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using ::mathtrader::solver::internal::TradeModel;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::FieldsAre;
using ::testing::Gt;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

static constexpr std::array<std::string_view, 6> items = {
    "Pandemic", "MageKnight", "PuertoRico", "SanJuan", "a", "1"};

// Verifies that the constructor builds self-arcs.
TEST(TradeModelTest, HasSelfTrades) {
  const TradeModel model(items);
  const auto assignments = model.assignments();

  // One self-assignment has been creates for every item.
  EXPECT_THAT(assignments, SizeIs(items.size()));

  // Verifies the actual self-assignments for each item. The self-trading cost
  // is expected to be a big number.
  for (const std::string_view item : items) {
    EXPECT_THAT(assignments,
                Contains(FieldsAre(/*offered=*/item, /*wanted=*/item,
                                   /*cost=*/Gt(1'000))));
  }
}
}  // namespace
