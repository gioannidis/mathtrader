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

#include <cstdint>
#include <string>
#include <string_view>

#include "absl/types/span.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "ortools/base/file.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_response.pb.h"
#include "mathtrader/parser/trade_request.pb.h"
#include "mathtrader/solver/internal/trade_request_extensions.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::TradePair;
using ::mathtrader::common::TradeResponse;
using ::mathtrader::parser::TradeRequest;
using ::mathtrader::solver::Solver;
using ::mathtrader::solver::internal::TradeRequestExtensions;
using ::testing::AllOf;
using ::testing::IsEmpty;
using ::testing::Property;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

// Maximum buffer length in bytes when reading an entire file to string.
static constexpr int64_t kReadToStringMaxLength = 2'000;

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

// Solves the math trade defined by the input textproto, verifies that we have
// found a solution and returns the response. If `has_usernames` has been given,
// builds also an items map from the internal `TradeRequestExtensions`.
TradeResponse SolveTrade(std::string_view textproto,
                         bool has_usernames = false) {
  // Builds the trade request.
  TradeRequest trade_request = BuildTradeRequest(textproto);

  // Creates usernames if specified.
  if (has_usernames) {
    // Iterates each individual owner; they may own multiple items.
    for (const auto& owners :
         trade_request.GetRepeatedExtension(TradeRequestExtensions::owners)) {
      // Iterates the items owned by the given owner.
      for (const std::string& item_id : owners.items()) {
        // Creates a new item indexed by the given id.
        Item& item = (*trade_request.mutable_items())[item_id];

        // Sets the item id (for consistency) and the username (for the test).
        item.set_id(item_id);
        item.set_username(owners.username());
      }
    }
  }
  // Solves the math trade problem.
  Solver solver;
  solver.BuildModel(trade_request);

  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();
  return solver.response();
}

// As above, but reads the textproto from a file.
TradeResponse SolveTradeFromFile(std::string_view textproto_pathname,
                                 bool has_usernames = false) {
  // Opens the textproto file with default options.
  File* textproto_file =
      file::OpenOrDie(/*filename=*/textproto_pathname, /*mode=*/"r",
                      /*flags=*/file::Defaults());

  // Reads the textproto file to string.
  std::string textproto;
  textproto_file->ReadToString(&textproto, kReadToStringMaxLength);

  return SolveTrade(textproto, has_usernames);
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

// Test suite: disconnected item chains with multiple users. Use case:
// - Alice (U1) and Bob (U2) trade with each other their respective G1 items.
// U1G1 -> U2G1
// U2G1 -> U1G1
//
// - Charlie (U3) and Daniel (U4) trade with each other their respective G1
//   items.
// U3G1 -> U4G1
// U3G1 -> U3G1
//
// - Alice can either trade with Eve (U5) or form a longer chain with Bob,
//   Charlie and Daniel.
// U5G1 -> U1G2
// U1G2 -> U2G2, U5G1
// U2G2 -> U3G2
// U3G2 -> U4G2
// U4G2 -> U1G2
//
// See the `./test_data` subfolder for a visualization.
static constexpr std::string_view kDisconnectedChainsPathname =
    "mathtrader/solver/test_data/disconnected_chains.textproto";

// Baseline: no usernames are given.
TEST(DisconnectedChainsTest, NoUsernames) {
  const TradeResponse& response =
      SolveTradeFromFile(kDisconnectedChainsPathname);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U1G1", "U2G1"), TradePairIs("U2G1", "U1G1"),
                  TradePairIs("U3G1", "U4G1"), TradePairIs("U4G1", "U3G1"),
                  TradePairIs("U1G2", "U2G2"), TradePairIs("U2G2", "U3G2"),
                  TradePairIs("U3G2", "U4G2"), TradePairIs("U4G2", "U1G2")));
}

// Defines usernames for all items.
TEST(DisconnectedChainsTest, WithUsernames) {
  const TradeResponse& response =
      SolveTradeFromFile(kDisconnectedChainsPathname, /*has_usernames=*/true);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U1G1", "U2G1"), TradePairIs("U2G1", "U1G1"),
                  TradePairIs("U3G1", "U4G1"), TradePairIs("U4G1", "U3G1"),
                  TradePairIs("U1G2", "U5G1"), TradePairIs("U5G1", "U1G2")));
}

// Test suite: five small item chains or one bigger chain. Use case:
// - Alice (U1) and Bob (U2) trade with each other their respective G1-G5 items.
// U1GN -> U2GN -> U1GN
// for N = 1..5
//
// - Charlie (U3) offers one item, creating the following bigger chain:
// U3G1 -> U2G1 -> U1G2 -> U2G3 -> U1G4 -> U2G5 -> U3G1
//
// If Charlie's bigger chain trades, then 5 items do not trade, but all users
// trade at least one item. Otherwise, Alice and Bob trade all their items,
// while Charlie's one item does not trade.
//
// See the `./test_data` subfolder for a visualization.
static constexpr std::string_view kMultipleSmallAndOneBigChainPathname =
    "mathtrader/solver/test_data/multiple_small_and_one_big_chain.textproto";

// Baseline: no usernames are given. Alice and Bob trade all their items.
// Charlie does not trade.
TEST(MultipleSmallAndOneBigChainTest, NoUsernames) {
  const TradeResponse& response =
      SolveTradeFromFile(kMultipleSmallAndOneBigChainPathname);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U1G1", "U2G1"), TradePairIs("U2G1", "U1G1"),
                  TradePairIs("U1G2", "U2G2"), TradePairIs("U2G2", "U1G2"),
                  TradePairIs("U1G3", "U2G3"), TradePairIs("U2G3", "U1G3"),
                  TradePairIs("U1G4", "U2G4"), TradePairIs("U2G4", "U1G4"),
                  TradePairIs("U1G5", "U2G5"), TradePairIs("U2G5", "U1G5")));
}

// Defines usernames for all items. Alice trades 3 items, Bob trades 2 items,
// Charlie trades their single item. All users trade at least one item.
TEST(MultipleSmallAndOneBigChainTest, WithUsernames) {
  const TradeResponse& response = SolveTradeFromFile(
      kMultipleSmallAndOneBigChainPathname, /*has_usernames=*/true);
  EXPECT_THAT(response.trade_pairs(),
              UnorderedElementsAre(
                  TradePairIs("U2G1", "U1G2"), TradePairIs("U1G2", "U2G3"),
                  TradePairIs("U2G3", "U1G4"), TradePairIs("U1G4", "U2G5"),
                  TradePairIs("U2G5", "U3G1"), TradePairIs("U3G1", "U2G1")));
}
}  // namespace
