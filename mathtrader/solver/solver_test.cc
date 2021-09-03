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

#include <string>
#include <string_view>

#include "absl/types/span.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/trade_request.pb.h"
#include "mathtrader/solver/trade_response.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::parser::TradeRequest;
using ::mathtrader::solver::Solver;
using ::mathtrader::solver::TradePair;
using ::mathtrader::solver::TradeResponse;
using ::testing::AllOf;
using ::testing::IsEmpty;
using ::testing::Property;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

// Matches a TradePair message, encapsulating an offered and a wanted item.
MATCHER_P2(TradePairIs, offered_id, wanted_id, "") {
  return ExplainMatchResult(AllOf(Property(&TradePair::offered, offered_id),
                                  Property(&TradePair::wanted, wanted_id)),
                            arg, result_listener);
}

// Builds the item map from the TradeRequest::wantlists. This allows us to
// define test protos in text format and auto-populate the map. No `Item` fields
// are populated.
void BuildItemMap(TradeRequest& trade_request) {
  for (const auto& wantlist : trade_request.wantlists()) {
    // Inserts the offered item.
    gtl::InsertIfNotPresent(trade_request.mutable_items(), wantlist.offered(),
                            Item());
    // Inserts all wanted items.
    for (const auto& wanted : wantlist.wanted()) {
      gtl::InsertIfNotPresent(trade_request.mutable_items(), wanted.id(),
                              Item());
    }
  }
}

// Constructs a TradeRequest proto from the given text input. Builds the
// wantlists from the text input and updates the item map with all listed items.
TradeRequest BuildTradeRequest(std::string_view text_proto) {
  TradeRequest parser_proto;
  CHECK(::google::protobuf::TextFormat::ParseFromString(std::string(text_proto),
                                                        &parser_proto));
  BuildItemMap(parser_proto);
  return parser_proto;
}

// Solves the math trade defined by the input, verifies that we have found a
// solution and returns the response. Optionally takes a span of the wantlist
// owners.
const TradeResponse SolveTrade(std::string_view input,
                               absl::Span<const std::string_view> owners = {}) {
  CHECK_GE(input.size(), owners.size());

  Solver solver;
  TradeRequest trade_request = BuildTradeRequest(input);

  // Index corresponding to owner[i] of wantlist[i].
  int index = 0;
  for (const std::string_view owner : owners) {
    const std::string_view offered = trade_request.wantlists(index).offered();
    auto* items = trade_request.mutable_items();
    auto it = items->find(offered);
    CHECK(it != items->end());
    it->second.set_username(std::string(owner));
    ++index;
  }
  solver.BuildModel(trade_request);

  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();
  return solver.response();
}

// Tests that no items trade with empty wantlists.
TEST(SolverTest, EmptyWantlists) {
  static constexpr std::string_view input = R"pb(
    wantlists { offered: "Pandemic" }
    wantlists { offered: "MageKnight" }
    wantlists { offered: "ThroughTheAges" }
    wantlists { offered: "Carcassonne" }
  )pb";

  const TradeResponse& response = SolveTrade(input);
  EXPECT_THAT(response.trade_pairs(), IsEmpty());
}

// Tests two items that trade with each other.
TEST(SolverTest, TwoItems) {
  static constexpr std::string_view input = R"pb(
    wantlists {
      offered: "Pandemic"
      wanted { id: "MageKnight" }
    }
    wantlists {
      offered: "MageKnight"
      wanted { id: "Pandemic" }
    }
  )pb";

  const TradeResponse& response = SolveTrade(input);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(TradePairIs("Pandemic", "MageKnight"),
                                   TradePairIs("MageKnight", "Pandemic")));
}

// Tests five items that trade with each other.
TEST(SolverTest, FiveItemsOneChain) {
  static constexpr std::string_view input = R"pb(
    wantlists {
      offered: "Pandemic"
      wanted { id: "MageKnight" }
    }
    wantlists {
      offered: "MageKnight"
      wanted { id: "Carcassonne" }
    }
    wantlists {
      offered: "Carcassonne"
      wanted { id: "ThroughTheAges" }
    }
    wantlists {
      offered: "ThroughTheAges"
      wanted { id: "TwilightStruggle" }
    }
    wantlists {
      offered: "TwilightStruggle"
      wanted { id: "Pandemic" }
    }
  )pb";

  const TradeResponse& response = SolveTrade(input);
  EXPECT_THAT(
      response.trade_pairs(),
      UnorderedElementsAre(TradePairIs("Pandemic", "MageKnight"),
                           TradePairIs("MageKnight", "Carcassonne"),
                           TradePairIs("Carcassonne", "ThroughTheAges"),
                           TradePairIs("ThroughTheAges", "TwilightStruggle"),
                           TradePairIs("TwilightStruggle", "Pandemic")));
}

// Tests five items that form two chains.
TEST(SolverTest, FiveItemsTwoChains) {
  static constexpr std::string_view input = R"pb(
    wantlists {
      offered: "Pandemic"
      wanted { id: "MageKnight" }
    }
    wantlists {
      offered: "MageKnight"
      wanted { id: "Pandemic" }
    }
    wantlists {
      offered: "Carcassonne"
      wanted { id: "ThroughTheAges" }
    }
    wantlists {
      offered: "ThroughTheAges"
      wanted { id: "TwilightStruggle" }
    }
    wantlists {
      offered: "TwilightStruggle"
      wanted { id: "Carcassonne" }
    }
  )pb";

  const TradeResponse& response = SolveTrade(input);
  EXPECT_THAT(
      response.trade_pairs(),
      UnorderedElementsAre(TradePairIs("Pandemic", "MageKnight"),
                           TradePairIs("MageKnight", "Pandemic"),
                           TradePairIs("Carcassonne", "ThroughTheAges"),
                           TradePairIs("ThroughTheAges", "TwilightStruggle"),
                           TradePairIs("TwilightStruggle", "Carcassonne")));
}

// Tests three items with priorities. Pandemic trades with Carcassonne because
// it has a higher priority (lower value).
TEST(SolverTest, ThreeItemsWithPriorities) {
  static constexpr std::string_view input = R"pb(
    wantlists {
      offered: "Pandemic"
      wanted { id: "Carcassonne" priority: 1 }
      wanted { id: "MageKnight" priority: 2 }
    }
    wantlists {
      offered: "MageKnight"
      wanted { id: "Pandemic" }
    }
    wantlists {
      offered: "Carcassonne"
      wanted { id: "Pandemic" }
    }
  )pb";

  const TradeResponse& response = SolveTrade(input);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(TradePairIs("Pandemic", "Carcassonne"),
                                   TradePairIs("Carcassonne", "Pandemic")));
}

// Test suite: solving with usernames. The use case is as follows:
// - Users U1 and U2 trade with each other their respective G1 items.
// U1G1 -> U2G1
// U2G1 -> U1G1
//
// - Users U3 and U4 trade with each other their respective G1 items.
//
// U3G1 -> U4G1
// U3G1 -> U3G1
//
// - User U1 can either trade with U5 or form a longer chain with U2, U3, U4.
// U5G1 -> U1G2
// U1G2 -> U2G2, U5G1
// U2G2 -> U3G2
// U3G2 -> U4G2
// U4G2 -> U1G2

static constexpr std::string_view kSolverWithUsernamesTestUseCase = R"pb(
  wantlists {
    offered: "U1G1"
    wanted { id: "U2G1" }
  }
  wantlists {
    offered: "U2G1"
    wanted { id: "U1G1" }
  }

  wantlists {
    offered: "U3G1"
    wanted { id: "U4G1" }
  }
  wantlists {
    offered: "U4G1"
    wanted { id: "U3G1" }
  }

  wantlists {
    offered: "U5G1"
    wanted { id: "U1G2" }
  }
  wantlists {
    offered: "U1G2"
    wanted { id: "U2G2" }
    wanted { id: "U5G1" }
  }
  wantlists {
    offered: "U2G2"
    wanted { id: "U3G2" }
  }
  wantlists {
    offered: "U3G2"
    wanted { id: "U4G2" }
  }
  wantlists {
    offered: "U4G2"
    wanted { id: "U1G2" }
  }
)pb";

// Baseline: no usernames are given.
TEST(SolverWithUsernamesTest, NoUsernames) {
  const TradeResponse& response = SolveTrade(kSolverWithUsernamesTestUseCase);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U1G1", "U2G1"), TradePairIs("U2G1", "U1G1"),
                  TradePairIs("U3G1", "U4G1"), TradePairIs("U4G1", "U3G1"),
                  TradePairIs("U1G2", "U2G2"), TradePairIs("U2G2", "U3G2"),
                  TradePairIs("U3G2", "U4G2"), TradePairIs("U4G2", "U1G2")));
}

// Defines usernames for all items.
TEST(SolverWithUsernamesTest, WithUsernames) {
  const TradeResponse& response =
      SolveTrade(kSolverWithUsernamesTestUseCase,
                 {"user1", "user2", "user3", "user4", "user5", "user1", "user2",
                  "user3", "user4"});
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U1G1", "U2G1"), TradePairIs("U2G1", "U1G1"),
                  TradePairIs("U3G1", "U4G1"), TradePairIs("U4G1", "U3G1"),
                  TradePairIs("U1G2", "U5G1"), TradePairIs("U5G1", "U1G2")));
}
}  // namespace
