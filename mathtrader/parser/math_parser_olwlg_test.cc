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

// Tests the OLWLG on real OLWLG testcases.

#include "mathtrader/parser/math_parser.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"

namespace {

using ::mathtrader::Item;
using ::mathtrader::MathParser;
using ::mathtrader::Wantlist;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::Each;
using ::testing::ExplainMatchResult;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::MatchesRegex;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::StartsWith;

// ID of non-dummy items: NNNN-AAAA or NNNN-AAAA-COPYNN.
// Examples:
//    "0001-MKGB", "0002-20GIFT"
static constexpr char kItemRegex[] =
    R"([[:digit:]]{4}-[[:alnum:]]*)"  // exactly 4 leading digits
                                      // separating '-"
                                      // any alphanumeric, including zero.
    R"((-COPY[[:digit:]]+)?)";  // e.g., "-COPY42"; optional

// Matches an item that:
// * Non-dummy and adhere to a specific format.
// * Dummy and begin with "%".
MATCHER(IsValidItemId, "") {
  return ExplainMatchResult(
      AnyOf(AllOf(Property(&Item::id, MatchesRegex(kItemRegex)),
                  Property(&Item::is_dummy, IsFalse())),
            AllOf(Property(&Item::id, StartsWith("%")),
                  Property(&Item::is_dummy, IsTrue()))),
      arg, result_listener);
}

TEST(MathParserOlwlgTest, TestJune2021US) {
  MathParser parser;
  const auto parser_result =
      parser.ParseFile("mathtrader/parser/test_data/286101-officialwants.txt");
  ASSERT_TRUE(parser_result.ok()) << parser_result.status().message();

  // Verifies the number of users with items.
  EXPECT_EQ(parser_result->user_count(), 335);

  const auto& wantlists = parser_result->wantlist();

  // Verifies the number of wantlists.
  EXPECT_THAT(wantlists, SizeIs(14860));

  // Verifies the id format of offered items.
  EXPECT_THAT(wantlists,
              Each(Property(&Wantlist::offered_item, IsValidItemId())));

  // Verifies the longest wantlist:
  //   line 19783: "(jgoyes) 1109-3GIFT ..."
  EXPECT_THAT(wantlists,
              Contains(Property(&Wantlist::wanted_item, SizeIs(695))));

  // Verifies the id format of wanted items.
  EXPECT_THAT(
      wantlists,
      Each(Property(&Wantlist::wanted_item, Each(IsValidItemId()))));
}

}  // namespace
