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

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "ortools/sat/sat_parameters.pb.h"

#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/trade_response.pb.h"
#include "mathtrader/solver/internal/trade_model.h"
#include "mathtrader/util/str_indexer.h"

namespace mathtrader::solver {
class Solver {
 public:
  Solver() = default;

  // Disables copy constructor and assignment operator.
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  ~Solver() = default;

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
  // The model that represents the math trade.
  internal::TradeModel trade_model_;

  // Parameters to configure the solver.
  operations_research::sat::SatParameters sat_parameters_;

  // The response with the solved trade.
  common::TradeResponse response_;
};
}  // namespace mathtrader::solver
#endif  // MATHTRADER_SOLVER_SOLVER_H_
