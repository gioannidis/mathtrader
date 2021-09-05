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

// This is a subset of the internal parser tests, as MathParser is merely a
// stateless public API of the internal parser.

#include "mathtrader/parser/math_parser.h"

#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/wantlist.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::Wantlist;
using ::mathtrader::parser::MathParser;
using ::testing::_;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::MatchesRegex;
using ::testing::Pair;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrCaseEq;
using ::testing::UnorderedElementsAre;

using RemovedItem = ::mathtrader::common::TradeRequest::RemovedItem;

// Base test case with official item names and wantlists. Each wantlist defines
// three (3) non-dummy items.
TEST(MathParserTest, TestOfficialItemsAndWantlists) {
  static constexpr std::string_view kInputData = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from Abcd1)
0002-SOC ==> "Shadows over Camelot" (from Abcd2)
0003-AGMI ==> "Adventure Games: Monochrome Inc." (from Abcd3)
0004-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from Abcd3)
0005-TIMSTO ==> "T.I.M.E Stories" (from Abcd4)
0006-AN6P-COPY1 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from Abcd5) [copy 1 of 3]
0007-AN6P-COPY2 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from Abcd5) [copy 2 of 3]
!END-OFFICIAL-NAMES

(Abcd1) 0001-20GIFT : 0002-SOC 0003-AGMI 0004-AN7WCS
(Abcd2) 0002-SOC : 0001-20GIFT 0006-AN6P-COPY1 0003-AGMI
(Abcd3) 0003-AGMI : 0001-20GIFT 0005-TIMSTO 0002-SOC
(Abcd3) 0004-AN7WCS : 0006-AN6P-COPY1 0007-AN6P-COPY2 0001-20GIFT
(Abcd4) 0005-TIMSTO : 0001-20GIFT 0003-AGMI 0002-SOC
)";

  const auto result = MathParser::ParseText(kInputData);
  ASSERT_TRUE(result.ok()) << result.status().message();

  // Verifies that:
  // * All usernames match "Abcd[0-9]", ignoring case.
  // * No offered item is dummy.
  EXPECT_THAT(
      result->items(),
      Each(Pair(_, AllOf(Property(&Item::username,
                                  MatchesRegex(R"([Aa][Bb][Cc][Dd][0-9])")),
                         Property("dummy", &Item::is_dummy, IsFalse())))));

  // Verifies that all wantlists have 3 wanted items.
  EXPECT_THAT(result->wantlists(),
              Each(Property(&Wantlist::wanted, SizeIs(3))));

  // Checks that there are no missing or duplicate items.
  EXPECT_EQ(result->missing_items_size(), 0);
  EXPECT_EQ(result->duplicate_items_size(), 0);
}

// Tests that missing items are properly removed from wantlists. These are
// wanted items without an official name.
TEST(MathParserTest, TestMissingItems) {
  static constexpr std::string_view kInputData = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from SomeUsername1)
0002-SOC ==> "Shadows over Camelot" (from SomeUsername2)
0003-AGMI ==> "Adventure Games: Monochrome Inc." (from SomeUsername3)
0004-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from SomeUsername3)
0005-TIMSTO ==> "T.I.M.E Stories" (from SomeUsername4)
0006-AN6P-COPY1 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from SomeUsername5) [copy 1 of 3]
0007-AN6P-COPY2 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from SomeUsername5) [copy 2 of 3]
!END-OFFICIAL-NAMES

(SomeUsername1) 0001-20GIFT : 0002-SOC 0003-AGMI 0004-AN7WCS
(SomeUsername2) 0002-SOC : 0001-20GIFT 0006-AN6P-COPY1 0003-AGMI
(SomeUsername3) 0003-AGMI : 0001-20GIFT 0005-TIMSTO missing-item 0002-SOC
(SomeUsername3) 0004-AN7WCS : missing-item 0006-AN6P-COPY1 0007-AN6P-COPY2 0001-20GIFT
(SomeUsername4) 0005-TIMSTO : 0001-20GIFT missing-item 0003-AGMI 0002-SOC missing-item
)";

  const auto result = MathParser::ParseText(kInputData);
  ASSERT_TRUE(result.ok()) << result.status().message();

  // Verifies that all wantlists have 3 wanted items. The 'missing-item' should
  // have been removed.
  EXPECT_THAT(result->wantlists(),
              Each(Property(&Wantlist::wanted, SizeIs(3))));

  // Checks that there is a 'missing-item' that has appeared 4 times.
  EXPECT_THAT(result->missing_items(),
              ElementsAre(AllOf(Property(&RemovedItem::wanted_item_id,
                                         StrCaseEq("missing-item")),
                                Property(&RemovedItem::frequency, Eq(4)))));

  // Checks that there are no duplicate items. 'missing-item' should be deleted
  // first, so no errors are generated, even if it appears 2+ times in a list.
  EXPECT_EQ(result->duplicate_items_size(), 0);
}

// Tests that missing items are properly removed from wantlists. These are
// wanted items without an official name.
TEST(MathParserTest, TestMissingItemsWithoutOfficialNames) {
  static constexpr std::string_view kInputData = R"(
(SomeUsername1) 0001-20GIFT : 0002-SOC 0003-AGMI 0004-AN7WCS
(SomeUsername2) 0002-SOC : 0001-20GIFT 0006-AN6P-COPY1 0003-AGMI
(SomeUsername3) 0003-AGMI : 0001-20GIFT 0005-TIMSTO %DUMMY 0002-SOC
(SomeUsername3) %DUMMY : 0006-AN6P-COPY1 0007-AN6P-COPY2 0001-20GIFT
(SomeUsername4) 0004-AN7WCS : 0001-20GIFT %DUMMY 0003-AGMI 0002-SOC
)";

  const auto result = MathParser::ParseText(kInputData);
  ASSERT_TRUE(result.ok()) << result.status().message();

  // Verifies the wantlists size after removing the missing wanted items.
  EXPECT_THAT(
      result->wantlists(),
      ElementsAre(Property("wanted items", &Wantlist::wanted, SizeIs(3)),
                  Property("wanted items", &Wantlist::wanted,
                           SizeIs(2)),  // removed: 0006
                  Property("wanted items", &Wantlist::wanted,
                           SizeIs(3)),  // removed: 0005
                  Property("wanted items", &Wantlist::wanted,
                           SizeIs(1)),  // removed: 0006, 0007
                  Property("wanted items", &Wantlist::wanted,
                           SizeIs(3))));  // removed: %DUMMY

  EXPECT_THAT(result->missing_items(),
              UnorderedElementsAre(Property("id", &RemovedItem::wanted_item_id,
                                            StrCaseEq("0005-TIMSTO")),
                                   Property("id", &RemovedItem::wanted_item_id,
                                            StrCaseEq("0006-AN6P-COPY1")),
                                   Property("id", &RemovedItem::wanted_item_id,
                                            StrCaseEq("0007-AN6P-COPY2")),
                                   Property("id", &RemovedItem::wanted_item_id,
                                            StartsWith(R"(%DUMMY)"))));

  // Checks that there are no duplicate items.
  EXPECT_EQ(result->duplicate_items_size(), 0);
}
}  // namespace
