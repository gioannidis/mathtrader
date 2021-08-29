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

TradeModel::TradeModel(absl::Span<const std::string_view> items) {
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
  // Key that identifies this trade: <offered, wanted>
  const auto key = MakeItemPair(offered, wanted);

  // Creates a new BoolVar representing the allowed trade and hashes it by the
  // ids of the offered and wanted item. Note that we call the "NoPrint"
  // operation because there is no default `operator<<` overload for an
  // `std::pair`.
  InternalAssignment assignment = {cp_model_.NewBoolVar(), cost};

  // Adds a linear expression term representing the cost of this trade.
  total_cost_.AddTerm(/*var=*/assignment.var, /*coeff=*/assignment.cost);

  // Finally, registers the assignment.
  gtl::InsertOrDieNoPrint(&assignments_, {key, assignment});
}

void TradeModel::BuildConstraints() {
  using ::operations_research::sat::LinearExpr;

  // offered_sums[i] = sum{ BoolVar[i][j] } for all `j`.
  // Represents the sum of how many wanted items trade with the offered item
  // `i`.
  absl::flat_hash_map<int32_t, LinearExpr> offered_sums;

  // wanted_sums[j] = sum{ BoolVar[i][j] } for all `i`.
  // Represents the sum of how many offered items trade with the wanted item
  // `j`.
  absl::flat_hash_map<int32_t, LinearExpr> wanted_sums;

  for (const auto& [key, assignment] : assignments_) {
    int32_t offered = 0;
    int32_t wanted = 0;
    std::tie(offered, wanted) = key;

    // Retrieves the respective sums of the offered and wanted item, or creates
    // them if they do not exist.
    LinearExpr& offered_sum =
        gtl::LookupOrInsert(&offered_sums, offered, LinearExpr());
    LinearExpr& wanted_sum =
        gtl::LookupOrInsert(&wanted_sums, wanted, LinearExpr());

    // Adds the BoolVar[i][j] to offered_sum[i] and wanted_sum[j].
    offered_sum.AddVar(assignment.var);
    wanted_sum.AddVar(assignment.var);
  }

  // Mandates that each offered item trades with exactly one wanted item.
  for (const auto& [index, offered_sum] : offered_sums) {
    cp_model_.AddEquality(offered_sum, 1);
  }

  // Mandates that each wanted item trades with exactly one offered item.
  for (const auto& [index, wanted_sum] : wanted_sums) {
    cp_model_.AddEquality(wanted_sum, 1);
  }
}

std::vector<TradeModel::Assignment> TradeModel::assignments() const {
  std::vector<Assignment> assignments;
  for (const auto& [key, internal_assignment] : assignments_) {
    Assignment assignment;
    assignment.offered = indexer_.ValueOrDie(key.first);
    assignment.wanted = indexer_.ValueOrDie(key.second);
    assignment.cost = internal_assignment.cost;
    assignments.emplace_back(std::move(assignment));
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
