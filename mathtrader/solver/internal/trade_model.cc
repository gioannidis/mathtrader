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
  gtl::InsertOrDieNoPrint(&assignments_, {key, assignment});
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
