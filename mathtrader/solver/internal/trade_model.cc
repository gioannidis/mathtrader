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

#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "ortools/base/map_util.h"

#include "mathtrader/util/str_indexer.h"

namespace mathtrader::solver::internal {
namespace {
// The trade cost of an item not trading. This is set to a large number to guide
// the CP Solver to trade as many items as possible.
static constexpr int64_t kSelfTradeCost = 1'000'000;
}  // namespace

// Creates an entry in the `assignments_` vector for each item.
TradeModel::TradeModel(absl::Span<const std::string_view> items)
    : assignments_(items.size()) {
  // Creates the indexes for each item.
  indexer_.BuildIndexes(items);

  // Builds a self-trade trade for each item, representing the item not being
  // traded. This is required so that the solver always finds a solution.
  for (const std::string_view item : items) {
    AddSelfAssignment(item);
  }
}

void TradeModel::AddAssignment(std::string_view offered,
                               std::string_view wanted, int64_t cost) {
  const int32_t offered_id = indexer_.IndexOrDie(offered);
  const int32_t wanted_id = indexer_.IndexOrDie(wanted);

  // Creates a new BoolVar representing the allowed trade.
  InternalAssignment assignment = {cp_model_.NewBoolVar(), cost};

  // Registers the allowed assignment between the `offered` and the `wanted`
  // items.
  gtl::InsertOrDie(&assignments_[offered_id], wanted_id, assignment);
}

void TradeModel::BuildConstraints() {
  using ::operations_research::sat::LinearExpr;

  // wanted_sums[j] = sum{ BoolVar[i][j] } for all `i`.
  // Represents the sum of how many offered items trade with the wanted item
  // `j`.
  absl::flat_hash_map<int32_t, LinearExpr> wanted_sums;

  // The index of the offered item. We increment it within the loop to avoid
  // mixing signed with unsigned types.
  int32_t offered_id = 0;
  for (const auto& assignment_row : assignments_) {
    // The sum of wanted items that trade with `offered_id`.
    LinearExpr offered_sum;

    for (const auto& [wanted_id, assignment] : assignment_row) {
      // Retrieves the sum of offered items that trade with `wanted id`. Creates
      // it if they do not exist.
      LinearExpr& wanted_sum =
          gtl::LookupOrInsert(&wanted_sums, wanted_id, LinearExpr());

      // Adds the BoolVar[i][j] to offered_sum[i] and wanted_sum[j].
      offered_sum.AddVar(assignment.var);
      wanted_sum.AddVar(assignment.var);
    }
    // Mandates that the offered item trades with exactly one wanted item.
    cp_model_.AddEquality(offered_sum, 1);

    ++offered_id;
  }

  // Mandates that each wanted item trades with exactly one offered item.
  for (const auto& [index, wanted_sum] : wanted_sums) {
    cp_model_.AddEquality(wanted_sum, 1);
  }
}

void TradeModel::BuildTotalCost() {
  for (const auto& assignment_row : assignments_) {
    for (const auto& [wanted_id, assignment] : assignment_row) {
      total_cost_.AddTerm(/*var=*/assignment.var, /*coeff=*/assignment.cost);
    }
  }
}

std::vector<TradeModel::Assignment> TradeModel::assignments() const {
  // The assignment vector to return.
  std::vector<Assignment> assignments;

  // The index of the offered item. We increment it within the loop to avoid
  // mixing signed with unsigned types.
  int32_t offered_id = 0;
  for (const auto& assignment_row : assignments_) {
    for (const auto& [wanted_id, internal_assignment] : assignment_row) {
      Assignment assignment;
      assignment.offered = indexer_.ValueOrDie(offered_id);
      assignment.wanted = indexer_.ValueOrDie(wanted_id);
      assignment.cost = internal_assignment.cost;
      assignments.emplace_back(std::move(assignment));
    }
    ++offered_id;
  }
  return assignments;
}

void TradeModel::AddSelfAssignment(std::string_view item) {
  // Calls the public API with a fixed cost for self-trades.
  AddAssignment(item, item, kSelfTradeCost);
}

std::pair<int32_t, int32_t> TradeModel::MakeItemPair(
    std::string_view offered, std::string_view wanted) const {
  return std::make_pair(indexer_.IndexOrDie(offered),
                        indexer_.IndexOrDie(wanted));
}
}  // namespace mathtrader::solver::internal
