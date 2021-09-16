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

// Module that solves the math trade.

#ifndef MATHTRADER_SOLVER_SOLVER_H_
#define MATHTRADER_SOLVER_SOLVER_H_

#include <cstdint>
#include <string>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "ortools/sat/sat_parameters.pb.h"

#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/trade_response.pb.h"
#include "mathtrader/solver/internal/trade_model.h"

namespace mathtrader::solver {
class Solver {
 public:
  Solver() = default;

  // Disables copy constructor and assignment operator.
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  ~Solver() = default;

  // Specifies whether the trade graph should be decomposed to strongly
  // connected components in order to speed up the solution. Must be called
  // before `BuildModel()`.
  void set_decompose_components(bool decompose_components) {
    decompose_components_ = decompose_components;
  }

  // Builds the model that models the math trade from the given OLWLG-parser
  // generated input.
  void BuildModel(const common::TradeRequest& trade_request);

  // Trivial setter for the SAT parameters of the solver. You must set any
  // parameters before calling `Solve()`.
  void set_parameters(
      const operations_research::sat::SatParameters& sat_parameters) {
    sat_parameters_ = sat_parameters;
  }

  // Trivial setter for the SAT parameters of the solver, which consumes the
  // `sat_parameters` argument. You must set any parameters before calling
  // `Solve()`.
  void set_parameters(
      operations_research::sat::SatParameters&& sat_parameters) {
    sat_parameters_ = std::move(sat_parameters);
  }

  // Solves the CP model. Return `OkStatus` if an optimal or feasible solution
  // has been found.
  ABSL_MUST_USE_RESULT absl::Status SolveModel();

  // Returns the solution to the math trade.
  ABSL_MUST_USE_RESULT const common::TradeResponse& response() const {
    return response_;
  }

 private:
  // Solves an individual CP model representing a strongly connected component
  // of the trade graph. Returns `OkStatus` if an optimal or feasible solution
  // has been found.
  ABSL_MUST_USE_RESULT absl::Status SolveIndividualModel(
      internal::TradeModel& trade_model);

  // Specifies whether the trade graph should be decomposed to strongly
  // connected components.
  bool decompose_components_{};

  // The trade models that represent the math trade, populated by
  // `BuildModel()`. If `decompose_components_` has been specified, this will
  // contain multiple trade models, each representing a strongly connected
  // component. Otherwise, this will contain exactly one trade model.
  std::vector<internal::TradeModel> trade_models_;

  // Parameters to configure the solver.
  operations_research::sat::SatParameters sat_parameters_;

  // Stores the users who have not yet traded any item. Maps each user to a
  // vector with all owned items. This is used to introduce username constraints
  // in strongly connected components for usernames which traded items in a
  // previous strongly connected component.
  absl::flat_hash_map<std::string, std::vector<std::string>> nontrading_users_;

  // The response with the solved trade.
  common::TradeResponse response_;
};
}  // namespace mathtrader::solver
#endif  // MATHTRADER_SOLVER_SOLVER_H_
