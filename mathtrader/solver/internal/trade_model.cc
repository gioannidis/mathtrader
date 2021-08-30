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

// The trade cost of a user not trading. This is set to a large number to guide
// the CP Solver towards solutions where users trade at least one item. Note
// that it should be at least one order of magnitude larger than
// `kSelfTradeCost` so that the CP Solver prioritizes more trading users over
// more trading items.
static constexpr int64_t kNonTradingUserCost = 10'000'000;
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

void TradeModel::AddOwner(std::string_view owner, std::string_view item) {
  const int32_t item_id = indexer_.IndexOrDie(item);

  // Retrieves the owned items for this owner. Creates an empty container if the
  // owner doesn't exist.
  auto& owned_items = gtl::LookupOrInsert(&owners_, std::string(owner), {});
  owned_items.emplace_back(item_id);
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

// Implementation details: each term is in the form: `t[i][j] * c[i][j]`,
// where:
//   i: an offered item
//   j: a wanted item
//   t[i][j]: {0, 1}, represents whether item `i` trades with `j`. If
//            `i == j`, this represents item `i` not being traded.
//   c[i][j]: the cost of item `i` trading with item `j`. Trading costs are
//            determined by the position of `j` in the wantlist of `i`.
//            Self-trades are assigned an internal value: `cost >> 1`.
void TradeModel::BuildItemTradingCost() {
  for (const auto& assignment_row : assignments_) {
    for (const auto& [wanted_id, assignment] : assignment_row) {
      total_cost_.AddTerm(/*var=*/assignment.var, /*coeff=*/assignment.cost);
    }
  }
}

// Implementation details: a user who does not trade any item incurs an
// additional cost. This is implemented as:
//   `!sum{!t[i][i]} => kNonTradingUserCost` for all owned items `i`.
//
// * The variable `t[i][i]` encodes a self-trade, i.e., the item `i` not
//   trading.
// * The negation `!t[i][i]` represents the item `i` trading with some other
//   item other than itself.
// * `sum{!t[i][i]}` represents the user trading at least one item.
// * `!sum{!t[i][i]}` represents the user not trading at all.
void TradeModel::BuildNonTradingUserCosts() {
  using operations_research::sat::BoolVar;
  using operations_research::sat::LinearExpr;
  using operations_research::sat::Not;

  for (const auto& [owner, items] : owners_) {
    // Represents the number of items that are trading. Implemented as:
    // `sum{ Not(assignment[i][i]) }`. The variable `assignment[i][i]`
    // represents the item not trading (self-trade), so we take its inversion.
    LinearExpr trading_items_sum;

    for (const int32_t item_id : items) {
      // Retrieves the self-trading assignment of the given item. Equivalent to:
      // `assignment_[item_id][item_id]`.
      const auto& assignment = gtl::FindOrDie(assignments_[item_id], item_id);

      // Adds the inversion of the self-trading variable.
      trading_items_sum.AddVar(assignment.var.Not());
    }

    // Declares an intermediate boolean variable that represents whether the
    // given user trades at least one item or not.
    const BoolVar user_trades = cp_model_.NewBoolVar();

    // Implements user_trades = (trading_items_sum >= 1).
    cp_model_.AddGreaterOrEqual(trading_items_sum, 1)
        .OnlyEnforceIf(user_trades);
    cp_model_.AddLessThan(trading_items_sum, 1).OnlyEnforceIf(Not(user_trades));

    // Creates a half-reified constraint. A non-trading user increases the cost
    // of the math trade. Trading users incur no additional costs.
    total_cost_.AddTerm(Not(user_trades), kNonTradingUserCost);
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

std::vector<TradeModel::Owner> TradeModel::owners() const {
  std::vector<Owner> owners;
  for (const auto& [owner_name, items] : owners_) {
    Owner owner;
    owner.owner = owner_name;
    for (const int32_t item_id : items) {
      owner.items.emplace_back(indexer_.ValueOrDie(item_id));
    }
    owners.emplace_back(std::move(owner));
  }
  return owners;
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
