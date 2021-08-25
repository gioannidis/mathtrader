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

#include "mathtrader/assignment/assignment.pb.h"
#include "mathtrader/solver/solver_result.pb.h"

namespace mathtrader::solver {
class Solver {
 public:
  Solver() = default;

  // Disables copy constructor and assignment operator.
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  ~Solver() = default;

  void set_assignment(const assignment::Assignment& assignment);
  void set_assignment(assignment::Assignment&& assignment);

  const SolverResult& result() const { return result_; }

 private:
  // The input assignment to run the solver on.
  assignment::Assignment assignment_;

  // The result with the solved trade.
  SolverResult result_;
};
}  // namespace mathtrader::solver
#endif  // MATHTRADER_SOLVER_SOLVER_H_
