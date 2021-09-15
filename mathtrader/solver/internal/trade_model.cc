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

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "ortools/sat/cp_model.pb.h"

#include "mathtrader/common/trade_response.pb.h"

namespace mathtrader::solver::internal {
namespace {
// The trade cost of an item not trading. This is set to a large number to guide
// the CP Solver to trade as many items as possible.
static constexpr int64_t kSelfTradeCost = 10'000;

// The trade cost of a user not trading. This is set to a large number to guide
// the CP Solver towards solutions where users trade at least one item. Note
// that it should be at least one order of magnitude larger than
// `kSelfTradeCost` so that the CP Solver prioritizes more trading users over
// more trading items.
static constexpr int64_t kNonTradingUserCost = 100'000'000;
}  // namespace

void TradeModel::AddAssignment(std::string_view offered,
                               std::string_view wanted, int64_t cost) {
  CHECK_NE(offered, wanted) << "Offered and Wanted items must be different";

  // This must be the first such assignment; caller is not allowed to declare
  // the same assignment twice.
  // TODO(gioannidis) this is defined in ortools logging, which is included via
  // cp_model->model->logging. Use glog instead.
  CHECK(InternalAddAssignment(offered, wanted, cost));

  // Also adds the self-costs for the offered and wanted items, if they do not
  // already exist. This allows to ignore items that do not appear in any
  // wantlist.
  InternalAddAssignment(offered, offered, kSelfTradeCost);
  InternalAddAssignment(wanted, wanted, kSelfTradeCost);
}

void TradeModel::AddOwner(std::string_view owner, std::string_view item) {
  // Retrieves existing owner data or creates new data.
  auto [iter, was_inserted] = owners_.try_emplace(owner);
  OwnerData& owner_data = iter->second;

  // If it's a new user, creates a boolean variable to mark whether the user
  // trades at least one item.
  if (was_inserted) {
    owner_data.is_trading = cp_model_.NewBoolVar();
  }

  // Adds the new item to the vector of items owned by `owner`.
  owner_data.items.emplace_back(item);
}

void TradeModel::BuildConstraints() {
  using ::operations_research::sat::LinearExpr;

  // wanted_sums[j] = sum{ BoolVar[i][j] } for all `i`.
  // Represents the sum of offered items that trade with the wanted item `j`.
  absl::flat_hash_map<std::string_view, LinearExpr> wanted_sums;

  for (const auto& [offered, assignments_of_offered] : assignments_) {
    // The sum of wanted items that trade with `offered` item.
    LinearExpr offered_sum;

    for (const auto& [wanted, assignment] : assignments_of_offered) {
      // Retrieves the sum of offered items that trade with `wanted id`. Creates
      // it if they do not exist.
      LinearExpr& wanted_sum = wanted_sums[wanted];

      // Adds the BoolVar[i][j] to offered_sum[i] and wanted_sum[j].
      offered_sum.AddVar(assignment.var);
      wanted_sum.AddVar(assignment.var);
    }
    // Mandates that the offered item trades with exactly one wanted item.
    cp_model_.AddEquality(offered_sum, 1);
  }

  // Mandates that each wanted item trades with exactly one offered item.
  for (const auto& [unused_wanted, wanted_sum] : wanted_sums) {
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
  for (const auto& [offered, assignments_of_offered] : assignments_) {
    for (const auto& [wanted, assignment] : assignments_of_offered) {
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

  for (const auto& [owner, owner_data] : owners_) {
    // Represents the number of items that are trading. Implemented as:
    // `sum{ Not(assignment[i][i]) }`. The variable `assignment[i][i]`
    // represents the item not trading (self-trade), so we take its inversion.
    LinearExpr trading_items_sum;

    for (const std::string_view item : owner_data.items) {
      // Retrieves the self-trading assignment of the given item, if it exists.
      // It is possible for the self-assignment cost not to exist if the item
      // was never declared in any wantlist. In this case, the item is entirely
      // ignored.
      if (assignments_.contains(item)) {
        // Equivalent to: `assignment_[item][item]`, but also checks that it
        // actually exists.
        const auto& assignment = assignments_[item].at(item);

        // Adds the inversion of the self-trading variable.
        trading_items_sum.AddVar(assignment.var.Not());
      }
    }

    // Intermediate boolean variable that represents whether the given user
    // trades at least one item or not.
    const BoolVar user_trades = owner_data.is_trading;

    // Implements user_trades = (trading_items_sum >= 1).
    cp_model_.AddGreaterOrEqual(trading_items_sum, 1)
        .OnlyEnforceIf(user_trades);
    cp_model_.AddLessThan(trading_items_sum, 1).OnlyEnforceIf(Not(user_trades));

    // Creates a half-reified constraint. A non-trading user increases the cost
    // of the math trade. Trading users incur no additional costs.
    total_cost_.AddTerm(Not(user_trades), kNonTradingUserCost);
  }
}

void TradeModel::PopulateResponse(
    const operations_research::sat::CpSolverResponse& response,
    common::TradeResponse& trade_response) const {
  using ::operations_research::sat::SolutionBooleanValue;

  // Iterates all boolean variables representing an offered->wanted assignment
  // to check whether they were chosen.
  for (const auto& [offered, assignments_of_offered] : assignments_) {
    for (const auto& [wanted, assignment] : assignments_of_offered) {
      // Checks whether this offered->wanted trade was eventually chosen.
      // Ignores self-trades.
      if ((offered != wanted) &&
          SolutionBooleanValue(response, assignment.var)) {
        // Adds the trading pair to the trade_response.
        common::TradePair* const trade_pair = trade_response.add_trade_pairs();
        trade_pair->set_offered(offered);
        trade_pair->set_wanted(wanted);
      }
    }
  }
}

std::vector<std::string_view> TradeModel::GetTradingOwners(
    const operations_research::sat::CpSolverResponse& response) const {
  std::vector<std::string_view> trading_users;
  for (const auto& [owner, owner_data] : owners_) {
    if (SolutionBooleanValue(response, owner_data.is_trading)) {
      trading_users.emplace_back(owner);
    }
  }
  return trading_users;
}

std::vector<TradeModel::Assignment> TradeModel::assignments() const {
  // The assignment vector to return.
  std::vector<Assignment> assignments;

  // The index of the offered item. We increment it within the loop to avoid
  // mixing signed with unsigned types.
  for (const auto& [offered, assignments_of_offered] : assignments_) {
    for (const auto& [wanted, internal_assignment] : assignments_of_offered) {
      Assignment assignment;
      assignment.offered = offered;
      assignment.wanted = wanted;
      assignment.cost = internal_assignment.cost;
      assignments.emplace_back(std::move(assignment));
    }
  }
  return assignments;
}

std::vector<TradeModel::Owner> TradeModel::owners() const {
  std::vector<Owner> owners;
  for (const auto& [owner_name, owner_data] : owners_) {
    Owner owner;
    owner.owner = owner_name;
    for (const std::string_view item : owner_data.items) {
      owner.items.emplace_back(item);
    }
    owners.emplace_back(std::move(owner));
  }
  return owners;
}

bool TradeModel::InternalAddAssignment(std::string_view offered,
                                       std::string_view wanted, int64_t cost) {
  // Registers the allowed assignment between the `offered` and the `wanted`
  // items. Equivalent to calling `assignments_[offered][wanted] = assignment`,
  // but checks the insertion. Creates a new BoolVar representing the allowed
  // trade if the insertion is successful.
  const auto [unused_it, was_inserted] = assignments_[offered].try_emplace(
      wanted, InternalAssignment({cp_model_.NewBoolVar(), cost}));

  return was_inserted;
}
}  // namespace mathtrader::solver::internal
