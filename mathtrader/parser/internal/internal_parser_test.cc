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

#include "mathtrader/parser/internal/internal_parser.h"

#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

#ifndef ASSERT_OK
#define ASSERT_OK(status) ASSERT_TRUE((status).ok()) << (status).message();
#endif

namespace {
using ::mathtrader::common::Wantlist;
using ::mathtrader::parser::ParserResult;
using ::mathtrader::parser::internal::InternalParser;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsTrue;
using ::testing::Key;
using ::testing::Property;
using ::testing::ResultOf;
using ::testing::SizeIs;
using ::testing::StrCaseEq;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

using RemovedItem = ::mathtrader::parser::ParserResult::RemovedItem;

// Tests a basic use-case.
TEST(InternalParser, TestOnlyComments) {
  static constexpr std::string_view input_data =
      R"(# Comment line. Next line is empty.

#Comment line without leading whitespace. Next two lines are also empty.

)";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));
  EXPECT_EQ(parser.get_line_count(), 5);
  EXPECT_THAT(parser.parser_result().wantlists(), IsEmpty());
}

// Tests a single wantlist without official names. All wanted items are missing,
// because they have no respective wantlists.
TEST(InternalParser, TestSingleWantlist) {
  static constexpr std::string_view input_data = R"(1-A : 2-B 3-C 4-D)";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));
  EXPECT_EQ(parser.get_line_count(), 1);

  const ParserResult& result = parser.parser_result();

  // Only non-missing items are registered in the item map. In this case, it is
  // the offered item.
  EXPECT_THAT(result.items(), ElementsAre(Key("1-A")));

  // There is a single empty wantlist.
  EXPECT_THAT(result.wantlists(),
              ElementsAre(AllOf(Property(&Wantlist::offered, StrEq("1-A")),
                                Property(&Wantlist::wanted, IsEmpty()))));
}

TEST(InternalParser, TestMultipleWantlists) {
  static constexpr std::string_view input_data = R"(
  1-A : 2-B 3-C 4-D
  2-B : 1-A 4-D
  3-C : 5-E
  4-D : 2-B 1-A
  )";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));
  EXPECT_GE(parser.get_line_count(), 4);
  EXPECT_THAT(parser.parser_result().wantlists(), SizeIs(4));

  // Tests the mutable interface.
  EXPECT_THAT(parser.mutable_parser_result()->wantlists(), SizeIs(4));
}

// Test suite: official items

TEST(InternalParserItemsTest, TestOfficialItems) {
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
0002-SOC ==> "Shadows over Camelot" (from username2)
0024-AGMI ==> "Adventure Games: Monochrome Inc." (from username3)
0025-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from username3)
0026-TIMSTO ==> "T.I.M.E Stories" (from username4)
0038-AN6P-COPY1 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from username5) [copy 1 of 3]
0038-AN6P-COPY2 ==> "Alt Name: $62 PayPal/Zelle/Amazon" (from username5) [copy 2 of 3]
!END-OFFICIAL-NAMES)";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));

  const auto& result = parser.parser_result();
  EXPECT_EQ(result.item_count(), 7);  // Non-dummy items.
  EXPECT_EQ(result.items_size(), 7);  // All items.
  EXPECT_THAT(result.users(), SizeIs(5));
}

// Includes empty lines with all spaces. Note that although the ItemParser
// returns an error when an empty line is given, the InternalParser filters out
// empty lines before passing them to the ItemParser.
TEST(InternalParserItemsTest, TestColonsSpaces) {
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES

0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
  0002-SOC : "Shadows over Camelot" (from username2)

      0024-AGMI: "Adventure Games: Monochrome Inc." (from username3)
  0025-AN7WCS :"Alt Name: 7 Wonders coaster set" (from username3)
      0026-TIMSTO:"T.I.M.E Stories" (from username4)
                       
!END-OFFICIAL-NAMES)";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));

  const auto& result = parser.parser_result();
  EXPECT_EQ(result.item_count(), 5);  // Non-dummy items.
  EXPECT_EQ(result.items_size(), 5);  // All items.
  EXPECT_THAT(result.users(), SizeIs(4));
}

// Tests that we can specify new usernames in wantlists, even if they have not
// been declared in the official names.
TEST(InternalParserItemsTest, TestExtraUsernameInWantlist) {
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
0002-SOC ==> "Shadows over Camelot" (from username2)
0024-AGMI ==> "Adventure Games: Monochrome Inc." (from username3)
0025-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from username3)
0026-TIMSTO
0027-MKBG
!END-OFFICIAL-NAMES

(username1) 0001-20GIFT : 0002-SOC
(username4) 0026-TIMSTO : 0024-AGMI
(username5) 0027-MKBG : 0001-20GIFT
(username3) 0024-AGMI : 0002-SOC)";

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));

  const auto& result = parser.parser_result();
  EXPECT_EQ(result.item_count(), 6);  // Non-dummy items.
  EXPECT_EQ(result.items_size(), 6);  // All items.
  EXPECT_THAT(result.users(), SizeIs(5));
}

TEST(InternalParser, TestDuplicateItems) {
  // Defines official names for all items to avoid reporting any missing items.
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES
A
B
C
D
E
F
G
I
K
O
P
Q
R
X
Z
!END-OFFICIAL-NAMES
(user1) A : B C D E F G Z
(user2) B : A F Q F R A Z F C
(user3) C : O F K Z E K K P F I K X K
  )";
  // Number of wanted items, excluding duplicates.
  static constexpr int32_t kWantedItems[] = {7, 6, 8};

  // Number of all unique items: offered + unique
  static constexpr int32_t kAllWantedItems = 15;

  InternalParser parser;
  ASSERT_OK(parser.ParseText(input_data));

  const auto& result = parser.parser_result();
  EXPECT_EQ(result.item_count(), kAllWantedItems);  // Non-dummy items.
  EXPECT_EQ(result.items_size(), kAllWantedItems);  // All items.
  EXPECT_THAT(result.users(),
              UnorderedElementsAre(StrCaseEq("user1"), StrCaseEq("user2"),
                                   StrCaseEq("user3")));

  EXPECT_THAT(
      result.wantlists(),
      ElementsAre(
          Property("wanted items", &Wantlist::wanted, SizeIs(kWantedItems[0])),
          Property("wanted items", &Wantlist::wanted, SizeIs(kWantedItems[1])),
          Property("wanted items", &Wantlist::wanted,
                   SizeIs(kWantedItems[2]))));

  // Verifies all missing items and their frequencies.
  // Note that the order of duplicate items is irrelevant.
  EXPECT_THAT(
      result.duplicate_items(),
      UnorderedElementsAre(
          AllOf(Property("wanted", &RemovedItem::wanted_item_id, StrEq("F")),
                Property("offered", &RemovedItem::offered_item_id, StrEq("B")),
                Property("frequency", &RemovedItem::frequency, Eq(3))),

          AllOf(Property("wanted", &RemovedItem::wanted_item_id, StrEq("A")),
                Property("offered", &RemovedItem::offered_item_id, StrEq("B")),
                Property("frequency", &RemovedItem::frequency, Eq(2))),

          AllOf(Property("wanted", &RemovedItem::wanted_item_id, StrEq("F")),
                Property("offered", &RemovedItem::offered_item_id, StrEq("C")),
                Property("frequency", &RemovedItem::frequency, Eq(2))),

          AllOf(Property("wanted", &RemovedItem::wanted_item_id, StrEq("K")),
                Property("offered", &RemovedItem::offered_item_id, StrEq("C")),
                Property("frequency", &RemovedItem::frequency, Eq(5)))));
}

// Test suite: negative tests

// Offered items must have an official name, if official names have been given.
TEST(InternalParserNegativeTest, TestMissingOfficialOfferedItemName) {
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES
0001-A
0002-B
0003-C
!END-OFFICIAL-NAMES

0001-A : 0002-B 0003-C
0002-B : 0003-C 0001-A
0004-D : 0001-A
0003-C : 0001-A)";

  InternalParser parser;
  const absl::Status status = parser.ParseText(input_data);
  EXPECT_THAT(status, AllOf(ResultOf(absl::IsNotFound, IsTrue()),
                            Property("error message", &absl::Status::message,
                                     HasSubstr("0004-D"))));
}

// Checks against declaring a double wantlist for an item.
TEST(InternalParserNegativeTest, TestDoubleWantlist) {
  static constexpr std::string_view input_data = R"(
      0001-A : 0002-B 0003-C
      0002-B : 0003-C 0001-A
      0004-D : 0001-A
      0003-C : 0001-A
      0005-E : 0001-A 0004-D 0002-B
      0002-B : 0001-E
      0006-F : 0001-A)";

  InternalParser parser;
  const absl::Status status = parser.ParseText(input_data);
  EXPECT_THAT(status, AllOf(ResultOf(absl::IsAlreadyExists, IsTrue()),
                            Property("error message", &absl::Status::message,
                                     HasSubstr("0002-B"))));
}

// Checks against declaring a double wantlist for a dummy item.
TEST(InternalParserNegativeTest, TestDoubleWantlistOfDummyItem) {
  static constexpr std::string_view input_data = R"(
      0001-A : 0002-B 0003-C
      0002-B : 0003-C 0001-A
      0004-D : 0001-A
      (abcd) %003-C : 0001-A
      0005-E : 0001-A 0004-D 0002-B
      0006-F : 0001-E
      (abcd) %003-C : 0001-A)";

  InternalParser parser;
  const absl::Status status = parser.ParseText(input_data);
  EXPECT_THAT(status, AllOf(ResultOf(absl::IsAlreadyExists, IsTrue()),
                            Property("error message", &absl::Status::message,
                                     HasSubstr("%003-C"))));
}

// One of the items is a dummy item and should be ignored.
TEST(InternalParserNegativeTest, TestDummyNames) {
  static constexpr std::string_view input_data = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
0002-SOC ==> "Shadows over Camelot" (from username2)
0024-AGMI ==> "Adventure Games: Monochrome Inc." (from username3)
%0025-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from username3)
0026-TIMSTO ==> "T.I.M.E Stories" (from username4)
!END-OFFICIAL-NAMES)";

  InternalParser parser;
  EXPECT_TRUE(absl::IsInvalidArgument(parser.ParseText(input_data)));
}
}  // namespace
