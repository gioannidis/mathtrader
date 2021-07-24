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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"

#ifndef EXPECT_OK
#define EXPECT_OK(status) \
  EXPECT_TRUE((status).ok()) << (status).message();
#endif

namespace {

using ::mathtrader::internal_parser::InternalParser;
using ::mathtrader::Item;
using ::mathtrader::Wantlist;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsTrue;
using ::testing::Property;
using ::testing::ResultOf;
using ::testing::SizeIs;
using ::testing::StrEq;

// Tests a basic use-case.
TEST(InternalParser, TestOnlyComments) {
  const std::string input_data = R"(# Comment line. Next line is empty.

#Comment line without leading whitespace. Next two lines are also empty.

)";

  InternalParser parser;
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_EQ(parser.get_line_count(), 5);
  EXPECT_THAT(parser.get_parser_result().wantlist(), IsEmpty());
}

TEST(InternalParser, TestSingleWantlist) {
  const std::string input_data = R"(1-A : 2-B 3-C 4-D)";

  InternalParser parser;
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_EQ(parser.get_line_count(), 1);
  EXPECT_THAT(parser.get_parser_result().wantlist(),
              ElementsAre(AllOf(Property(&Wantlist::offered_item,
                                         Property(&Item::id, StrEq("1-A"))),
                                Property(&Wantlist::wanted_item,
                                         SizeIs(3)))));
}

TEST(InternalParser, TestMultipleWantlists) {
  const std::string input_data = R"(
  1-A : 2-B 3-C 4-D
  2-B : 1-A 4-D
  3-C : 5-E
  4-D : 2-B 1-A
  )";

  InternalParser parser;
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_GE(parser.get_line_count(), 4);
  EXPECT_THAT(parser.get_parser_result().wantlist(), SizeIs(4));

  // Tests the mutable interface.
  EXPECT_THAT(parser.mutable_parser_result()->wantlist(), SizeIs(4));
}

// Test suite: official items

TEST(InternalParserItemsTest, TestOfficialItems) {
  const std::string input_data = R"(
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
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_EQ(parser.get_item_count(), 7);
  EXPECT_THAT(parser.get_parser_result().users(), SizeIs(5));
}

// Includes empty lines with all spaces. Note that although the ItemParser
// returns an error when an empty line is given, the InternalParser filters out
// empty lines before passing them to the ItemParser.
TEST(InternalParserItemsTest, TestColonsSpaces) {
  const std::string input_data = R"(
!BEGIN-OFFICIAL-NAMES

0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
  0002-SOC : "Shadows over Camelot" (from username2)

      0024-AGMI: "Adventure Games: Monochrome Inc." (from username3)
  0025-AN7WCS :"Alt Name: 7 Wonders coaster set" (from username3)
      0026-TIMSTO:"T.I.M.E Stories" (from username4)
                       
!END-OFFICIAL-NAMES)";

  InternalParser parser;
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_EQ(parser.get_item_count(), 5);
  EXPECT_THAT(parser.get_parser_result().users(), SizeIs(4));
}

// One of the items is a dummy item and should be ignored.
// TODO(gioannidis) move to negative tests.
TEST(InternalParserItemsTest, DISABLED_TestDummyNames) {
  const std::string input_data = R"(
!BEGIN-OFFICIAL-NAMES
0001-20GIFT ==> "Alt Name: $20 PayPal GC" (from username1)
0002-SOC ==> "Shadows over Camelot" (from username2)
0024-AGMI ==> "Adventure Games: Monochrome Inc." (from username3)
%0025-AN7WCS ==> "Alt Name: 7 Wonders coaster set" (from username3)
0026-TIMSTO ==> "T.I.M.E Stories" (from username4)
!END-OFFICIAL-NAMES)";

  InternalParser parser;
  EXPECT_OK(parser.ParseText(input_data));
  EXPECT_EQ(parser.get_item_count(), 4);
  EXPECT_THAT(parser.get_parser_result().users(), SizeIs(4));
}

// Tests that we can specify new usernames in wantlists, even if they have not
// been declared in the official names.
TEST(InternalParserItemsTest, TestExtraUsernameInWantlist) {
  const std::string input_data = R"(
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
  EXPECT_TRUE(parser.ParseText(input_data).ok());
  EXPECT_EQ(parser.get_item_count(), 6);
  EXPECT_THAT(parser.get_parser_result().users(), SizeIs(5));
}

// Test suite: negative tests

// Offered items must have an official name, if official names have been given.
TEST(InternalParserNegativeTest, TestMissingOfficialOfferedItemName) {
  const std::string input_data = R"(
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
  absl::Status status = parser.ParseText(input_data);

  // Verifies that 3 official items were read.
  EXPECT_EQ(parser.get_item_count(), 3);
  EXPECT_THAT(parser.get_parser_result().users(), IsEmpty());
  EXPECT_THAT(status,
              AllOf(ResultOf(absl::IsInvalidArgument, IsTrue()),
                    Property(&absl::Status::message, HasSubstr("0004-D"))));
}

// Checks against declaring a double wantlist for an item.
TEST(InternalParserNegativeTest, TestDoubleWantlist) {
  const std::string input_data = R"(
      0001-A : 0002-B 0003-C
      0002-B : 0003-C 0001-A
      0004-D : 0001-A
      0003-C : 0001-A
      0005-E : 0001-A 0004-D 0002-B
      0002-B : 0001-E
      0006-F : 0001-A)";

  InternalParser parser;
  absl::Status status = parser.ParseText(input_data);
  EXPECT_THAT(status,
              AllOf(ResultOf(absl::IsInvalidArgument, IsTrue()),
                    Property(&absl::Status::message, HasSubstr("0002-B"))));
}

// Checks against declaring a double wantlist for a dummy item.
TEST(InternalParserNegativeTest, TestDoubleWantlistOfDummyItem) {
  const std::string input_data = R"(
      0001-A : 0002-B 0003-C
      0002-B : 0003-C 0001-A
      0004-D : 0001-A
      (abcd) %003-C : 0001-A
      0005-E : 0001-A 0004-D 0002-B
      0006-F : 0001-E
      (abcd) %003-C : 0001-A)";

  InternalParser parser;
  absl::Status status = parser.ParseText(input_data);
  EXPECT_THAT(status,
              AllOf(ResultOf(absl::IsInvalidArgument, IsTrue()),
                    Property(&absl::Status::message, HasSubstr("%003-C"))));
}

}  // namespace
