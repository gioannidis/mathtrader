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

#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/parser/parser_result.pb.h"
#include "mathtrader/solver/solver_result.pb.h"

namespace {
using ::google::protobuf::TextFormat;
using ::mathtrader::common::Item;
using ::mathtrader::parser::ParserResult;
using ::mathtrader::solver::Solver;
using ::mathtrader::solver::TradePair;
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

// Builds the item map from the ParserResult::wantlists. This allows us to
// define test protos in text format and auto-populate the map. No `Item` fields
// are populated.
void BuildItemMap(ParserResult& parser_result) {
  for (const auto& wantlist : parser_result.wantlists()) {
    // Inserts the offered item.
    gtl::InsertIfNotPresent(parser_result.mutable_items(), wantlist.offered(),
                            Item());
    // Inserts all wanted items.
    for (const auto& wanted : wantlist.wanted()) {
      gtl::InsertIfNotPresent(parser_result.mutable_items(), wanted.id(),
                              Item());
    }
  }
}

// Constructs a ParserResult proto from the given text input. Builds the
// wantlists from the text input and updates the item map with all listed items.
ParserResult BuildParserResult(std::string_view text_proto) {
  ParserResult parser_proto;
  CHECK(TextFormat::ParseFromString(std::string(text_proto), &parser_proto));
  BuildItemMap(parser_proto);
  return parser_proto;
}

// Tests that no items trade with empty wantlists.
TEST(SolverTest, EmptyWantlists) {
  static constexpr std::string_view input = R"pb(
    wantlists { offered: "Pandemic" }
    wantlists { offered: "MageKnight" }
    wantlists { offered: "ThroughTheAges" }
    wantlists { offered: "Carcassonne" }
  )pb";

  Solver solver;
  solver.BuildModel(BuildParserResult(input));

  // Solves and verifies that we have found a solution.
  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();

  EXPECT_THAT(solver.result().trade_pairs(), IsEmpty());
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

  Solver solver;
  solver.BuildModel(BuildParserResult(input));

  // Solves and verifies that we have found a solution.
  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();

  EXPECT_THAT(solver.result().trade_pairs(),
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

  Solver solver;
  solver.BuildModel(BuildParserResult(input));

  // Solves and verifies that we have found a solution.
  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();

  EXPECT_THAT(
      solver.result().trade_pairs(),
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

  Solver solver;
  solver.BuildModel(BuildParserResult(input));

  // Solves and verifies that we have found a solution.
  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();

  EXPECT_THAT(
      solver.result().trade_pairs(),
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

  Solver solver;
  solver.BuildModel(BuildParserResult(input));

  // Solves and verifies that we have found a solution.
  const auto status = solver.SolveModel();
  CHECK(status.ok()) << status.message();

  EXPECT_THAT(solver.result().trade_pairs(),
              UnorderedElementsAre(TradePairIs("Pandemic", "Carcassonne"),
                                   TradePairIs("Carcassonne", "Pandemic")));
}
}  // namespace
