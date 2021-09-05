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

#include "absl/base/attributes.h"
#include "absl/status/status.h"

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

  // Solves the CP model. Return `OkStatus` if an optimal or feasible solution
  // has been found.
  ABSL_MUST_USE_RESULT absl::Status SolveModel();

  // Returns the solution to the math trade.
  ABSL_MUST_USE_RESULT const common::TradeResponse& response() const {
    return response_;
  }

  // Sets the maximum time in seconds to run the Solver when `SolveModel()` is
  // called. If the Solver has not found the optimal solution when the time
  // limit is reached, it will return a feasible solution, if it has found any.
  void set_max_time_in_seconds(double max_time_in_seconds) {
    max_time_in_seconds_ = max_time_in_seconds;
  }

 private:
  // The model that represents the math trade.
  internal::TradeModel trade_model_;

  // The response with the solved trade.
  common::TradeResponse response_;

  // Maximum time in seconds to run the Solver.
  double max_time_in_seconds_{};
};
}  // namespace mathtrader::solver
#endif  // MATHTRADER_SOLVER_SOLVER_H_
