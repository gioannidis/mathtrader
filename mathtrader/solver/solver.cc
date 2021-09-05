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
#include "ortools/sat/sat_parameters.pb.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/wantlist.pb.h"

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
}  // namespace

void Solver::BuildModel(const TradeRequest& trade_request) {
  // Retrieves the item ids and indexes them in the TradeModel.
  const auto item_ids = GetItemIds(trade_request);
  trade_model_.IndexItems(item_ids);

  // Adds the allowed assignment for each offered->wanted relationship.
  for (const common::Wantlist& wantlist : trade_request.wantlists()) {
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
  for (const auto& [id, item] : trade_request.items()) {
    if (item.has_username()) {
      trade_model_.AddOwner(/*owner=*/item.username(), /*item=*/id);
    }
  }

  // Builds the cost of non-trading users.
  trade_model_.BuildNonTradingUserCosts();
}

absl::Status Solver::SolveModel() {
  using ::operations_research::sat::CpSolverResponse;
  using ::operations_research::sat::CpSolverStatus;

  // First, commits the objective function.
  trade_model_.CommitObjectiveFunction();

  // Defines SAT parameters to pass to the solver.
  operations_research::sat::SatParameters parameters;

  // Sets a time limit, if specified.
  if (max_time_in_seconds_ > 0) {
    parameters.set_max_time_in_seconds(max_time_in_seconds_);
  }

  // Sets the number of parallel search workers.
  if (num_search_workers_ > 0) {
    parameters.set_num_search_workers(num_search_workers_);
  }

  // Sets whether the solver should stop after the first solution.
  parameters.set_stop_after_first_solution(stop_after_first_solution_);

  // Declares a generic wrapper to wire the solver and the parameters.
  operations_research::sat::Model model;
  model.Add(operations_research::sat::NewSatParameters(parameters));

  // Solves the CP model objective.
  const auto& cp_model = trade_model_.cp_model();
  const CpSolverResponse cp_solver_response =
      operations_research::sat::SolveCpModel(cp_model.Build(), &model);

  // Propagates the statistics.
  response_.set_cp_model_stats(
      operations_research::sat::CpModelStats(cp_model.Proto()));
  response_.set_solution_info(cp_solver_response.solution_info());

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
      return absl::UnknownError(
          "Solver reached a search limit before a solution could be "
          "determined.");
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
  trade_model_.PopulateResponse(cp_solver_response, response_);

  return absl::OkStatus();
}
}  // namespace mathtrader::solver
