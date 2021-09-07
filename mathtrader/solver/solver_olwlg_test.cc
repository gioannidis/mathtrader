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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

#include "mathtrader/solver/solver.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_set.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ortools/base/logging.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/trade_response.pb.h"
#include "mathtrader/parser/math_parser.h"
#include "ortools/sat/sat_parameters.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::TradePair;
using ::mathtrader::common::TradeRequest;
using ::mathtrader::common::TradeResponse;
using ::mathtrader::solver::Solver;

// Maximum time in seconds to run the solution.
static constexpr double kMaxTimeInSeconds = 3600;

// Number of parallel search workers.
static constexpr int32_t kNumSearchWorkers = 4;

// Builds the TradeRequest from the given OLWLG data.
void BuildTradeRequest(std::string_view filepath, TradeRequest& request) {
  mathtrader::parser::MathParser parser;
  auto request_or = parser.ParseFile(filepath);

  // Verifies the TradeRequest.
  ASSERT_TRUE(request_or.ok()) << request_or.status().message();
  request = std::move(*request_or);
}

// Builds and configures the solver from the given TradeRequest.
void BuildSolver(const TradeRequest& request, Solver& solver) {
  solver.BuildModel(request);
  operations_research::sat::SatParameters sat_parameters;

  // Sets a maximum allowed time in seconds to solve the problem.
  sat_parameters.set_max_time_in_seconds(kMaxTimeInSeconds);

  // Sets the number of parallel workers.
  sat_parameters.set_num_search_workers(kNumSearchWorkers);

  // solver.set_stop_after_first_solution(true);
  solver.set_parameters(std::move(sat_parameters));
}

// Verifies the trading non-dummy items.
void VerifyTradingItems(const TradeRequest& request,
                        const TradeResponse& response,
                        int64_t expected_trading_items) {
  const auto& trading_pairs = response.trade_pairs();

  // Counts non-dummy offered items. No need to examine wanted items, because
  // they also appear as offered.
  const int64_t trading_items =
      std::count_if(trading_pairs.begin(), trading_pairs.end(),
                    // Lambda that finds non-dummy items.
                    [&items = request.items()](const TradePair& trade_pair) {
                      return !items.at(trade_pair.offered()).is_dummy();
                    });
  EXPECT_EQ(trading_items, expected_trading_items);
}

// Solves the trade from the OLWLG data and verifies the expected results.
void ExpectTrade(std::string_view filepath, int64_t expected_trading_items,
                 int64_t expected_trading_users) {
  // Builds the TradeRequest.
  TradeRequest request;
  BuildTradeRequest(filepath, request);

  // Builds and configures the solver.
  Solver solver;
  BuildSolver(request, solver);

  // Solves the model and verifies the solver status.
  const absl::Status solver_status = solver.SolveModel();
  LOG(INFO) << "Model stats: " << solver.response().cp_model_stats();
  LOG(INFO) << "Solution info: " << solver.response().solution_info();
  LOG(INFO) << "Wall time: " << solver.response().wall_time();
  LOG(INFO) << "User time: " << solver.response().user_time();
  ASSERT_TRUE(solver_status.ok()) << solver_status.message();

  // Verifies the expected metrics.
  const TradeResponse& response = solver.response();
  VerifyTradingItems(request, response, expected_trading_items);
  EXPECT_EQ(response.trading_users(), expected_trading_users);

  // Outputs statistics.
  LOG(INFO) << "Optimal: " << response.is_optimal();
}

// Test suite: OLWLG worldwide tests.

TEST(MathParserOlwlgWorldTest, TestMarch2021Worldwide) {
  // TradeMaximizer: 592 items, 235 users. Time: 1137226ms
  static constexpr int64_t kItemCount = 0;
  static constexpr int64_t kUserCount = 0;
  ExpectTrade("olwlg_data/283180-officialwants.txt", kItemCount, kUserCount);
}

// Test suite: OLWLG country-specific tests.

TEST(MathParserOlwlgCountryTest, TestJune2021US) {
  // TradeMaximizer: 592 items, 235 users. Time: 1137226ms
  static constexpr int64_t kItemCount = 390;
  static constexpr int64_t kUserCount = 180;
  ExpectTrade("olwlg_data/286101-officialwants.txt", kItemCount, kUserCount);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Norway) {
  // TradeMaximizer: 10 items, 9 users. Time: 2ms
  static constexpr int64_t kItemCount = 10;
  static constexpr int64_t kUserCount = 9;
  ExpectTrade("olwlg_data/286103-officialwants.txt", kItemCount, kUserCount);
}

TEST(MathParserOlwlgCountryTest, TestJune2021UK) {
  // TradeMaximizer: 638 items, 180 users. Time: 2036655ms
  static constexpr int64_t kItemCount = 586;
  static constexpr int64_t kUserCount = 202;
  ExpectTrade("olwlg_data/286149-officialwants.txt", kItemCount, kUserCount);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Canada) {
  // TradeMaximizer: 166 items, 73 users. Time: 111ms
  static constexpr int64_t kItemCount = 150;
  static constexpr int64_t kUserCount = 84;
  ExpectTrade("olwlg_data/286870-officialwants.txt", kItemCount, kUserCount);
}

TEST(MathParserOlwlgCountryTest, TestJuly2021Greece) {
  // TradeMaximizer: 105 items, 43 users. Time: 405ms
  static constexpr int64_t kItemCount = 101;
  static constexpr int64_t kUserCount = 49;
  ExpectTrade("olwlg_data/286928-officialwants.txt", kItemCount, kUserCount);
}
}  // namespace
