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

#include "mathtrader/solver/solver.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::solver {
namespace {
using ::mathtrader::parser::ParserResult;

// Extracts the item ids from `parser_result`. Returns a vector with the
// extracted item ids.
std::vector<std::string_view> GetItemIds(const ParserResult& parser_result) {
  std::vector<std::string_view> items;
  std::transform(parser_result.items().begin(), parser_result.items().end(),
                 items.begin(),
                 [](std::pair<std::string_view, const common::Item&> map_pair) {
                   return map_pair.first;
                 });
  return items;
}
}  // namespace

void Solver::BuildModel(const ParserResult& parser_result) {
  // Retrieves the item ids and indexes them in the TradeModel.
  const auto item_ids = GetItemIds(parser_result);
  trade_model_.IndexItems(item_ids);

  // Adds the allowed assignment for each offered->wanted relationship.
  for (const common::Wantlist& wantlist : parser_result.wantlists()) {
    for (const auto& wanted_item : wantlist.wanted()) {
      trade_model_.AddAssignment(wantlist.offered(), wanted_item.id(),
                                 wanted_item.priority());
    }
  }

  // Builds the item constraints, mandating 1-1 matching between offered and
  // wanted items.
  trade_model_.BuildConstraints();

  // Builds the cost of trading items.
  trade_model_.BuildItemTradingCost();

  // Adds the usernames.
  for (const auto& [id, item] : parser_result.items()) {
    trade_model_.AddOwner(/*owner=*/item.username(), /*item=*/item.id());
  }

  // Builds the cost of non-trading users.
  trade_model_.BuildNonTradingUserCosts();
}
}  // namespace mathtrader::solver
