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
using ::testing::IsSupersetOf;
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

TEST(TradeModelTest, OneAssignmentPerItem) {
  TradeModel model(items);

  // Each item but the last is assigned to its next item.
  for (unsigned i = 0; i < items.size() - 1; ++i) {
    model.AddAssignment(/*offered=*/items.at(i), /*wanted=*/items.at(i + 1),
                        /*cost=*/1);
  }
  // Last item is assigned to the first item.
  model.AddAssignment(/*offered=*/items.back(), /*wanted=*/items.front(),
                      /*cost=*/1);

  const auto assignments = model.assignments();
  for (unsigned i = 0; i < items.size(); ++i) {
    // Next item: wraps around past the last item to the first item.
    unsigned next_i = (i + 1) % (items.size());
    EXPECT_THAT(assignments,
                Contains(FieldsAre(/*offered=*/items.at(i),
                                   /*wanted=*/items.at(next_i), /*cost=*/1)));
  }
}

TEST(TradeModelTest, MultipleAssignmentsPerItem) {
  TradeModel model(items);

  model.AddAssignment("Pandemic", "MageKnight", 1);
  model.AddAssignment("Pandemic", "PuertoRico", 2);
  model.AddAssignment("Pandemic", "a", 3);
  model.AddAssignment("SanJuan", "Pandemic", 1);

  const auto assignments = model.assignments();
  EXPECT_THAT(assignments, Contains(FieldsAre("Pandemic", "MageKnight", 1)));
  EXPECT_THAT(assignments, Contains(FieldsAre("Pandemic", "PuertoRico", 2)));
  EXPECT_THAT(assignments, Contains(FieldsAre("Pandemic", "a", 3)));
  EXPECT_THAT(assignments, Contains(FieldsAre("SanJuan", "Pandemic", 1)));
}

// Tests an item with multiple assignments where there is a big gap (step) in
// the costs.
TEST(TradeModelTest, BigStepCost) {
  static constexpr int64_t kBigStepCost = 42;
  TradeModel model(items);

  model.AddAssignment("Pandemic", "MageKnight", 1);
  model.AddAssignment("Pandemic", "PuertoRico", kBigStepCost);
  model.AddAssignment("Pandemic", "SanJuan", kBigStepCost + 1);

  const auto assignments = model.assignments();
  EXPECT_THAT(assignments, Contains(FieldsAre("Pandemic", "MageKnight", 1)));
  EXPECT_THAT(assignments,
              Contains(FieldsAre("Pandemic", "PuertoRico", kBigStepCost)));
  EXPECT_THAT(assignments,
              Contains(FieldsAre("Pandemic", "SanJuan", kBigStepCost + 1)));
}

// Tests the cost coefficients for self-trading items.
TEST(TradeModelTest, SelfAssignmentCoefficients) {
  TradeModel model(items);
  model.BuildTotalCost();
  EXPECT_THAT(model.cost_coefficients(),
              Contains(Gt(1'000)).Times(items.size()));
}

// Tests the cost coefficients for allowed trades.
TEST(TradeModelTest, AssignmentCoefficients) {
  TradeModel model(items);

  model.AddAssignment("Pandemic", "MageKnight", 1);
  model.AddAssignment("Pandemic", "PuertoRico", 2);
  model.AddAssignment("Pandemic", "a", 3);
  model.AddAssignment("SanJuan", "Pandemic", 1);
  model.AddAssignment("a", "PuertoRico", 1);
  model.AddAssignment("a", "Pandemic", 2);
  model.AddAssignment("a", "1", 3);
  model.AddAssignment("a", "SanJuan", 4);

  model.BuildTotalCost();

  // Verifies that it contains at least the coefficients of the above
  // assignments.
  const auto assignments = model.assignments();
  EXPECT_THAT(model.cost_coefficients(),
              IsSupersetOf({1, 2, 3, 1, 1, 2, 3, 4}));
}
}  // namespace
