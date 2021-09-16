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

#ifndef MATHTRADER_SOLVER_INTERNAL_TRADE_MODEL_H_
#define MATHTRADER_SOLVER_INTERNAL_TRADE_MODEL_H_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"

#include "mathtrader/common/trade_response.pb.h"

// CpModelBuilder wrapper to model a math trade.

namespace mathtrader::solver::internal {
// `TradeModel` is a wrapper around `CpModelBuilder`, defining an API oriented
// towards modeling a math trade. You can use the `TradeModel` as follows:
//
// 1. Define all observable items.
// 2. Define the allowed trades by calling `AddAllowedTrade`.
// 3. Constraints: max one wanted item per offered item and vice-versa.
// 4. Objective: add costs for item trades and negative costs for trading users.
//
// Example:
//   // Defines the tradeable items: Mage Knight, Puerto Rico, Pandemic.
//   TradeModel trade_model({"0001-MKBG", "0002-PUERIC", "0003-PANDE"});
//
//   // Equivalent wantlist: 0001-MKBG: 0002-PUERIC 0003-PANDE
//   trade_model.AddAllowedTrade("0001-MKBG", "0002-PUERIC", 1);
//   trade_model.AddAllowedTrade("0001-MKBG", "0003-PANDE", 2);
//
//   // Equivalent wantlist: 0002-PUERIC
//   trade_model.AddAllowedTrade("0002-PUERIC", "0001-MKBG", 1);
class TradeModel {
 public:
  // Debugging struct. Wrapper around the internal representation of item
  // assignments. Represents the allowed trades between offered and wanted items
  // using the actual item ids. A self-trade is represented by
  // `offered == wanted`.
  struct Assignment {
    std::string offered{};
    std::string wanted{};
    int64_t cost{};
  };

  // Debugging struct. Wrapper around the internal representation of owners.
  // Represents the items that a given user owns.
  struct Owner {
    std::string owner;
    std::vector<std::string> items;
  };

  TradeModel() = default;

  // Makes the TradeModel moveable (but not copyable). This is required to be
  // storable in vectors, because vectors may need to reallocate memory, thus
  // moving their contents.
  TradeModel(TradeModel&& other) = default;
  TradeModel& operator=(TradeModel&& other) = default;

  // Disables copy constructor and assignment operator.
  TradeModel(const TradeModel&) = delete;
  TradeModel& operator=(const TradeModel&) = delete;

  ~TradeModel() = default;

  // Adds an assignment between an offered and a wanted item, representing an
  // allowed trade. The two items must be different.
  void AddAssignment(std::string_view offered, std::string_view wanted,
                     int64_t cost);

  // Adds the owner of an item. This is desired if the target is to maximize the
  // owners that trade an item. It is possible to add multiple times the same
  // item to an owner.
  void AddOwner(std::string_view owner, std::string_view item);

  // Builds the costs associated with each potential item trade:
  // 'offered -> wanted'. An item trade incurs its cost if the solver actually
  // selects it.
  //
  // * Higher priority wanted items in the same wantlist incur a lower cost.
  //   This guides the solver to choose higher priority items when possible.
  //   For example, if:
  //     c1 = cost{offered -> high_priority_wanted}
  //     c2 = cost{offered -> low_priority_wanted}
  //   Then c1 < c2 because the wanted item has a higher priority.
  //
  // * Self-trades incur costs that are orders of magnitude higher than costs of
  //   actual trades. This guides the solver to choose actual trades over
  //   non-trades, even if it means selecting a lower-priority trade.
  //   For example, if:
  //     c3 = cost{offered -> offered}
  //   Then c3 >> c2 > c1.
  void BuildItemTradingCost();

  // Builds the costs associated with non-trading owners. An item owner incurs
  // an additional cost if they don't trade any item that they own. Owners that
  // trade at least one item incur no costs.
  void BuildNonTradingUserCosts();

  // Commits the objective function: minimizes the total cost that has been
  // build through previously-called `Build*Cost()` functions.
  void CommitObjectiveFunction() { cp_model_.Minimize(total_cost_); }

  // Mandates that each offered item must be traded with exactly one wanted item
  // and that each wanted item must be traded with exactly one offered item.
  // This also allows items to be traded with themselves, representing a
  // non-trade.
  void BuildConstraints();

  // Populates the solver result with the trading items from the solver
  // response and other statistics.
  void PopulateResponse(
      const operations_research::sat::CpSolverResponse& response,
      common::TradeResponse& trade_response) const;

  // Returns the internal CP model.
  const operations_research::sat::CpModelBuilder& cp_model() const {
    return cp_model_;
  }

  // Returns the trading owners for the given CpSolverResponse. The TradeModel
  // must outlive the returned vector.
  std::vector<std::string_view> GetTradingOwners(
      const operations_research::sat::CpSolverResponse& response) const;

  // Returns all constructed assignments representing the item trades, including
  // self-trades. Intended to be used for debugging because it constructs a new
  // vector.
  std::vector<Assignment> assignments() const;

  // Returns all owners with the respective owned items. Intended to be used for
  // debugging because it constructs a new vector.
  std::vector<Owner> owners() const;

  // Returns all cofficients of the cost linear expression. Intended to be used
  // for debugging.
  const absl::Span<const int64_t> cost_coefficients() const {
    return total_cost_.coefficients();
  }

 private:
  // Internal representation of an allowed assignment between an offered and a
  // wanted item.
  struct InternalAssignment {
    // The boolean variable representing whether the assignment is selected or
    // not. If true, the offered item trades with the wanted item.
    operations_research::sat::BoolVar var;

    // The cost of this trade if `var` is true.
    int64_t cost{};
  };

  // Represents data associated with item owners. An owner typically submits
  // multiple items in a math trade.
  struct OwnerData {
    // Items owned by the specific owner.
    std::vector<std::string> items;

    // Boolean variable indicating whether this owner trades at least one item.
    operations_research::sat::BoolVar is_trading;
  };

  // Internal implementation of adding an assignment between an offered and a
  // wanted item. This allows to call `InternalAddAssignment` to add self-trade
  // assignments. Returns `true` if the assignment was successfully added,
  // `false` if it already exists.
  bool InternalAddAssignment(std::string_view offered, std::string_view wanted,
                             int64_t cost);

  // The actual CP Model that represents that math trade.
  operations_research::sat::CpModelBuilder cp_model_;

  // The linear expression that represents the total cost of the trade.
  operations_research::sat::LinearExpr total_cost_;

  // Stores the allowed assignments. Each `assignment_[i][j]` represents an
  // allowed trade between the offered item `i` and the wanted item `j`. The
  // value of the boolean variable `assignment_[i][j].var` represents whether
  // this trade takes place or not. We define `assignment_[i][i]` for every
  // item, representing a self-trade, i.e., the item not being traded.
  absl::flat_hash_map<std::string,
                      absl::flat_hash_map<std::string, InternalAssignment>>
      assignments_;

  // Stores the item owners. Maps an owner to their owned item indexes.
  absl::flat_hash_map<std::string, OwnerData> owners_;
};
}  // namespace mathtrader::solver::internal
#endif  // MATHTRADER_SOLVER_INTERNAL_TRADE_MODEL_H_
