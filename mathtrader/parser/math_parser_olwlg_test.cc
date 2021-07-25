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

// Tests the MathParser on real OLWLG testcases. Note that worldwide tests tend
// to be longer than country-specfic tests, so you may want to use the following
// filters:
//  --test_filter=*WorldTest.* --> for worldwide tests
//  --test_filter=*CountryTest.* --> for country-specific tests

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
    R"(()"  // opens item id group
      // Item id format 1: exactly 4 leading digits, separating '-",
      // followed by any alphanumeric, including zero.
      R"(([[:digit:]]{4}-[[:alnum:]]*))"
      R"(|)"
      // Item id format 2: digits only.
      R"(([[:digit:]]+))"
    R"())"  // closes item id group
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

void ExpectWantlist(absl::string_view filename, int32_t user_count,
                    int32_t item_count, int32_t wantlist_count,
                    int32_t longest_wantlist) {
  MathParser parser;
  const auto parser_result = parser.ParseFile(filename);
  ASSERT_TRUE(parser_result.ok()) << parser_result.status().message();

  // Verifies the number of users with items.
  for (const auto& user : parser_result->users()) {
    std::cout << user << std::endl;
  }
  EXPECT_THAT(parser_result->users(), SizeIs(user_count));

  // Verifies the number of items.
  EXPECT_EQ(parser_result->item_count(), item_count);

  const auto& wantlists = parser_result->wantlist();

  // Verifies the number of wantlists.
  EXPECT_EQ(wantlists.size(), wantlist_count);

  // Verifies the id format of offered items.
  EXPECT_THAT(wantlists,
              Each(Property(&Wantlist::offered_item, IsValidItemId())));

  // Verifies the longest wantlist.
  EXPECT_THAT(wantlists,
              Contains(Property(&Wantlist::wanted_item,
                       SizeIs(longest_wantlist))));

  // Verifies the id format of wanted items.
  EXPECT_THAT(
      wantlists,
      Each(Property(&Wantlist::wanted_item, Each(IsValidItemId()))));
}

TEST(MathParserOlwlgWorldTest, TestMarch2021Worldwide) {
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  ExpectWantlist("mathtrader/parser/test_data/283180-officialwants.txt",
                 /*user_count=*/139, /*item_count=*/9056,
                 /*wantlist_count=*/13035, /*longest_wantlist=*/1000);
}

TEST(MathParserOlwlgCountryTest, TestJune2021US) {
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  ExpectWantlist("mathtrader/parser/test_data/286101-officialwants.txt",
                 /*user_count=*/335, /*item_count=*/5896,
                 /*wantlist_count=*/14860, /*longest_wantlist=*/695);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Norway) {
  // Longest wantlist: line 517: "(nils777) 8339221 ..."
  ExpectWantlist("mathtrader/parser/test_data/286103-officialwants.txt",
                 /*user_count=*/18, /*item_count=*/143,
                 /*wantlist_count=*/142, /*longest_wantlist=*/35);
}

TEST(MathParserOlwlgCountryTest, TestJune2021UK) {
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  ExpectWantlist("mathtrader/parser/test_data/286149-officialwants.txt",
                 /*user_count=*/223, /*item_count=*/2990,
                 /*wantlist_count=*/9549, /*longest_wantlist=*/785);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Canada) {
  // Longest wantlist: line 2149: "(Dragoon6542) 8351711 ..."
  ExpectWantlist("mathtrader/parser/test_data/286870-officialwants.txt",
                 /*user_count=*/121, /*item_count=*/1147,
                 /*wantlist_count=*/1266, /*longest_wantlist=*/289);
}

TEST(MathParserOlwlgCountryTest, TestJuly2021Greece) {
  // Longest wantlist: line 3417: "(Rico13mpatsoni) 8366279 ..."
  ExpectWantlist("mathtrader/parser/test_data/286928-officialwants.txt",
                 /*user_count=*/66, /*item_count=*/687,
                 /*wantlist_count=*/1003, /*longest_wantlist=*/235);
}

}  // namespace
