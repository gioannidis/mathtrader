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

#include <string_view>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "ortools/sat/cp_model.h"

#include "mathtrader/assignment/assignment.pb.h"
#include "mathtrader/util/str_indexer.h"

// CpModelBuilder wrapper to model a math trade.

namespace mathtrader::solver::internal {
// `TradeModel` is a wrapper around `CpModelBuilder`, defining an API oriented
// towards modeling a math trade. You can use the `TradeModel` as follows:
//
// 1. Construct the `TradeModel` by defining all items.
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
  // Constructs the TradeModel and passes all items.
  explicit TradeModel(absl::Span<const std::string_view> items);

  // Disables copy constructor and assignment operator.
  TradeModel(const TradeModel&) = delete;
  TradeModel& operator=(const TradeModel&) = delete;

  ~TradeModel() = default;

  // Adds an allowed trade assignment between an offered and a wanted item.
  void AddAllowedTrade(std::string_view offered, std::string_view wanted,
                       int64_t cost);

  // Allows each offered item to be paired with at most one wanted item and each
  // wanted item to be paired with at most one offered item.
  void BuildConstraints();

 private:
  // Adds a self trade on an item, representing the item not being traded.
  void AddSelfTrade(std::string_view item);

  // Creates a pair of item indexes, given the actual item ids. The first
  // element represents the offered item, the second element represents the
  // wanted item.
  std::pair<int32_t, int32_t> MakeItemPair(std::string_view offered,
                                           std::string_view wanted) const;

  // The actual CP Model that represents that math trade.
  operations_research::sat::CpModelBuilder cp_model_;

  // The linear expression that represents the total cost of the trade.
  operations_research::sat::LinearExpr total_cost_;

  // Maps each item to a 1:1 index. This avoids hashing entire strings.
  util::StrIndexer indexer_;

  // Allowed assignments, indexed by an <offered, wanted> pair. Each assignment
  // is represented by a boolean variable. A boolean variable <offered, wanted>
  // is true if item "offered" is assigned to item "wanted. If offered==wanted,
  // it represents a self-trade.
  absl::flat_hash_map<std::pair<int32_t, int32_t>,
                      operations_research::sat::BoolVar>
      allowed_assignments_;
};
}  // namespace mathtrader::solver::internal
#endif  // MATHTRADER_SOLVER_INTERNAL_TRADE_MODEL_H_
