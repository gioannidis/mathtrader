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

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/solver/internal/trade_model.h"

namespace mathtrader::solver {
namespace {
using ::mathtrader::common::TradeRequest;

// Extracts the item ids from `trade_request`. Returns a vector with the
// extracted item ids.
std::vector<std::string_view> GetItemIds(const TradeRequest& trade_request) {
  const auto& item_map = trade_request.items();

  // The item id container to return.
  std::vector<std::string_view> items(item_map.size());
  std::transform(trade_request.items().begin(), trade_request.items().end(),
                 items.begin(),
                 [](std::pair<std::string_view, const common::Item&> map_pair) {
                   return map_pair.first;
                 });
  return items;
}

// Builds a map: users -> items they own, from the given trade request.
auto BuildUserMap(const TradeRequest& trade_request) {
  absl::flat_hash_map<std::string, std::vector<std::string>> user_map;
  for (const auto& [id, item] : trade_request.items()) {
    if (item.has_username()) {
      auto& owned_items = user_map[item.username()];
      owned_items.push_back(item.id());
    }
  }
  return user_map;
}
}  // namespace

void Solver::BuildModel(const TradeRequest& trade_request) {
  // Builds exactly one model.
  trade_models_.emplace_back();
  auto& trade_model = trade_models_.front();

  // Adds the allowed assignment for each offered->wanted relationship.
  for (const common::Wantlist& wantlist : trade_request.wantlists()) {
    for (const auto& wanted_item : wantlist.wanted()) {
      trade_model.AddAssignment(wantlist.offered(), wanted_item.id(),
                                wanted_item.priority());
    }
  }

  // Builds the item constraints, mandating 1-1 matching between offered and
  // wanted items.
  trade_model.BuildConstraints();

  // Builds the cost of trading items.
  trade_model.BuildItemTradingCost();

  // Builds a map of users -> owned items. All users are included in the map,
  // because none has traded any item yet.
  nontrading_users_ = BuildUserMap(trade_request);
}

absl::Status Solver::SolveModel() {
  // Solves each individual strongly connected component. Returns an error if
  // the solver fails on any individual component.
  for (auto& trade_model : trade_models_) {
    if (absl::Status status = SolveIndividualModel(trade_model); !status.ok()) {
      return status;
    }
  }

  // Denotes that a solution was found for all individual components.
  return absl::OkStatus();
}

absl::Status Solver::SolveIndividualModel(internal::TradeModel& trade_model) {
  using ::operations_research::sat::CpSolverResponse;
  using ::operations_research::sat::CpSolverStatus;

  // Before solving, builds the cost of non-trading users. Users who have traded
  // at least one item in a previous TradeModel have already been removed from
  // `nontrading_users_`.
  for (const auto& [owner, items] : nontrading_users_) {
    for (const std::string_view item : items) {
      trade_model.AddOwner(owner, item);
    }
  }
  trade_model.BuildNonTradingUserCosts();

  // Commits the objective function which includes the costs of non-trading
  // users and non-trading items.
  trade_model.CommitObjectiveFunction();

  // Declares a generic wrapper to wire the solver and the sat parameters.
  operations_research::sat::Model model;
  model.Add(operations_research::sat::NewSatParameters(sat_parameters_));

  // Solves the CP model objective.
  const auto& cp_model = trade_model.cp_model();
  const CpSolverResponse cp_solver_response =
      operations_research::sat::SolveCpModel(cp_model.Build(), &model);

  // Propagates the statistics.
  response_.set_cp_model_stats(
      operations_research::sat::CpModelStats(cp_model.Proto()));
  response_.set_solution_info(cp_solver_response.solution_info());
  response_.set_user_time(cp_solver_response.user_time());
  response_.set_wall_time(cp_solver_response.wall_time());

  // Handles the CpSolverResponse status.
  switch (cp_solver_response.status()) {
    case CpSolverStatus::OPTIMAL: {
      response_.set_is_optimal(true);
      break;
    }
    case CpSolverStatus::FEASIBLE: {
      break;
    }
    case CpSolverStatus::UNKNOWN: {
      return absl::UnknownError(absl::StrFormat(
          "Solver reached a search limit before a solution could be "
          "determined. Tip: consider raising the imposed time limit of %lfs. "
          "Solver wall time: %lfs. Solver user time: %lfs.",
          sat_parameters_.max_time_in_seconds(), response_.wall_time(),
          response_.user_time()));
    }
    case CpSolverStatus::MODEL_INVALID: {
      return absl::InternalError("The generated CpModel is invalid.");
    }
    case CpSolverStatus::INFEASIBLE: {
      return absl::NotFoundError(
          "No solution was found. This indicates an issue with the generated "
          "CpModel, because a solution where no item trades can always be "
          "found.");
    }
    default: {
      return absl::UnimplementedError(absl::StrFormat(
          "No implementation to handle CpSolverResponse status: %d",
          cp_solver_response.status()));
      break;
    }
  }

  // Populates the trade_response with the trading items.
  trade_model.PopulateResponse(cp_solver_response, response_);

  // Removes users who have traded at least one item and populates the
  // TradeResponse.
  for (const std::string_view user :
       trade_model.GetTradingOwners(cp_solver_response)) {
    nontrading_users_.erase(user);
    *response_.add_trading_users() = user;
  }
  return absl::OkStatus();
}
}  // namespace mathtrader::solver
