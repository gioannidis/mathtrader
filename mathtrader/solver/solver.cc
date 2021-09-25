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

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "ortools/graph/strongly_connected_components.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/solver/internal/trade_model.h"
#include "mathtrader/util/str_indexer.h"

namespace mathtrader::solver {
namespace {
using ::mathtrader::common::TradeRequest;
using ::mathtrader::solver::internal::TradeModel;

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

// Converts the wantlists of the given `trade_request` to a graph, where integer
// indexes represent each item. Returns the result. The returned graph is a
// vector of adjacency lists, where `graph[i]` provides the adjacent vertices of
// vertex `i`.
auto ConvertWantlistsToGraph(const TradeRequest& trade_request,
                             const util::StrIndexer& indexer) {
  // Initializes one adjacency list for each item (vertex).
  std::vector<std::vector<int32_t>> graph(trade_request.items_size());

  for (const common::Wantlist& wantlist : trade_request.wantlists()) {
    // Converts the offered item to its vertex index.
    const int32_t offered = indexer.IndexOrDie(wantlist.offered());

    for (const auto& wanted_item : wantlist.wanted()) {
      // Converts the wanted item to its vertex index.
      const int32_t wanted = indexer.IndexOrDie(wanted_item.id());
      graph[offered].emplace_back(wanted);
    }
  }
  return graph;
}

// Builds the item constraints and costs in a trade model.
void BuildItemConstraintsAndCosts(TradeModel& trade_model) {
  // Builds the item constraints, mandating 1-1 matching between offered and
  // wanted items.
  trade_model.BuildConstraints();

  // Builds the cost of trading items.
  trade_model.BuildItemTradingCost();
}

// Builds a TradeModel from the given trade request. This represents the entire
// trade graph with a single TradeModel, without decomposing it to strongly
// connected components.
TradeModel BuildModelFromTradeRequest(const TradeRequest& trade_request) {
  // The TradeModel to return.
  TradeModel trade_model;

  // Adds the allowed assignment for each offered->wanted relationship.
  for (const common::Wantlist& wantlist : trade_request.wantlists()) {
    for (const auto& wanted_item : wantlist.wanted()) {
      trade_model.AddAssignment(wantlist.offered(), wanted_item.id(),
                                wanted_item.priority());
    }
  }
  BuildItemConstraintsAndCosts(trade_model);
  return trade_model;
}

auto BuildStronglyConnectedComponents(const TradeRequest& trade_request) {
  // Builds an index of the N item ids. Index range: [0, N)
  const util::StrIndexer indexer(GetItemIds(trade_request));

  // Converts the wantlists to adjacency lists.
  const auto graph = ConvertWantlistsToGraph(trade_request, indexer);

  // Finds the strongly connected components. Runs in `O(nodes + edges)`.
  std::vector<std::vector<int32_t>> components;
  FindStronglyConnectedComponents(graph.size(), graph, &components);

  // Sorts the strongly connected components by size in ascending order. This
  // heuristic is applied to process smaller components first, so that trading
  // users can be excluded from bigger components in favor of users who have not
  // traded yet.
  std::sort(components.begin(), components.end(),
            [](const auto& a, const auto& b) { return a.size() < b.size(); });

  // Builds a map: item -> component id. Component #0 is the smallest because
  // they have been sorted by size. The trade_request must outlive the map.
  absl::flat_hash_map<std::string_view, int32_t> item_to_component;
  for (int32_t i = 0; i < static_cast<int32_t>(components.size()); ++i) {
    for (const auto& item_index : components[i]) {
      // Converts the `item_index` back to the string item and maps it to the
      // component index.
      const auto [unused_it, was_inserted] =
          item_to_component.try_emplace(indexer.ValueOrDie(item_index), i);

      // `FindStronglyConnectedComponents` should list each item once.
      CHECK(was_inserted);
    }
  }

  // The trade models to return. Each TradeModel represents a strongly connected
  // component.
  std::vector<TradeModel> trade_models(components.size());

  // Adds item assignments to each TradeModel, ignoring item assignments between
  // different strongly connected components.
  for (const common::Wantlist& wantlist : trade_request.wantlists()) {
    const std::string_view offered = wantlist.offered();
    const int32_t component_id = item_to_component.at(offered);

    // Processes the wanted items that belong to the same strongly connected
    // component.
    for (const auto& wanted_item : wantlist.wanted()) {
      const std::string_view wanted = wanted_item.id();
      if (item_to_component.at(wanted) == component_id) {
        trade_models[component_id].AddAssignment(
            offered, wanted, /*cost=*/wanted_item.priority());
      }
    }
  }

  // Finally, builds the item constraints and costs for each strongly connected
  // component.
  for (TradeModel& trade_model : trade_models) {
    BuildItemConstraintsAndCosts(trade_model);
  }
  return trade_models;
}
}  // namespace

void Solver::BuildModel(const TradeRequest& trade_request) {
  // Builds a map of users -> owned items. All users are included in the map,
  // because none has traded any item yet.
  nontrading_users_ = BuildUserMap(trade_request);

  // Builds exactly one model if we do not decompose the trade graph to strongly
  // connected components.
  if (!decompose_components_) {
    trade_models_.emplace_back(BuildModelFromTradeRequest(trade_request));
  } else {
    trade_models_ = BuildStronglyConnectedComponents(trade_request);

    // Also populates the TradeResponse with the sizes of the strongly connected
    // components.
    for (const TradeModel& trade_model : trade_models_) {
      response_.add_component_sizes(trade_model.items_size());
    }
  }
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

absl::Status Solver::SolveIndividualModel(TradeModel& trade_model) {
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
