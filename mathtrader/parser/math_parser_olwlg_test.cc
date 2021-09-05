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
//  --test_filter=*CountryTest.* --> for country-specific tests
//  --test_filter=*WorldTest.* --> for worldwide tests

#include "mathtrader/parser/math_parser.h"

#include <ostream>
#include <string_view>

#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/trade_request.pb.h"
#include "mathtrader/common/wantlist.pb.h"

namespace {
using ::mathtrader::common::Item;
using ::mathtrader::common::TradeRequest;
using ::mathtrader::common::Wantlist;
using ::mathtrader::parser::MathParser;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::FieldsAre;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::MatchesRegex;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrCaseEq;
using ::testing::StrEq;

using RemovedItem = ::mathtrader::common::TradeRequest::RemovedItem;
}  // namespace

// Opens the same namespace that defines the protobuf Map in order to specialize
// the PrintTo function. See: https://google.github.io/googletest/advanced.html
namespace google::protobuf {
void PrintTo(
    const Map<std::basic_string<char>, mathtrader::common::Item>& item_map,
    std::ostream* os) {
  // The maximum number of items to print.
  static constexpr int64_t kMaxPrintItems = 20;

  int64_t item_count = 0;
  for (const auto& [id, item] : item_map) {
    // Skips printing items if we are over the threshold.
    if (item_count > kMaxPrintItems) {
      *os << "<... skipping over " << (item_map.size() - item_count)
          << " items ...>" << std::endl;
      break;
    }
    *os << "---" << std::endl
        << "Key #" << item_count << ": \"" << id << "\"" << std::endl
        << item.DebugString();
    ++item_count;
  }
}
}  // namespace google::protobuf

namespace {
// ID of non-dummy items: NNNN-AAAA or NNNN-AAAA-COPYNN.
// Examples:
//    "0001-MKGB", "0002-20GIFT"
static constexpr std::string_view kItemRegex =
    R"(()"  // opens item id group
            // Item id format 1: exactly 4 leading digits, separating '-",
            // followed by any alphanumeric, including zero.
    R"(([[:digit:]]{4}-[[:alnum:]]*))"
    R"(|)"
    // Item id format 2: digits only.
    R"(([[:digit:]]+))"
    R"())"                      // closes item id group
    R"((-COPY[[:digit:]]+)?)";  // e.g., "-COPY42"; optional

// Mapped(inner_matcher) matches an std::pair whose `second` field matches
// inner_matcher. For example, Contains(Mapped(Ge(5))) can be used to match an
// std::map that contains at least one element whose value is >= 5.
MATCHER_P(Mapped, inner_matcher, "") {
  return ExplainMatchResult(inner_matcher, arg.second, result_listener);
}

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

// Matches a (id_string, Item) pair, where id_string = Item::id. Essentially
// matches a map pair where the index is the item's id.
MATCHER(HasIdAsKey, "") {
  *result_listener << "whose property `map key` is \"" << arg.first << "\", ";
  return ExplainMatchResult(
      Property("actual item id", &Item::id, Eq(arg.first)), arg.second,
      result_listener);
}

// Matches a `RemovedItem` arg whose owner is `username`, ignoring case. The
// item is retrieved from item_map.
MATCHER_P2(WantlistOwnerOfRemovedStrCaseEq, username, item_map, "") {
  const Item* item = gtl::FindOrNull(item_map, arg.offered_item_id());
  if (!item) {
    return ExplainMatchResult(testing::NotNull(), item, result_listener);
  }
  return ExplainMatchResult(StrCaseEq(username), item->username(),
                            result_listener);
}

// Runs a number of checks against the given TradeRequest.
void ExpectWantlist(const absl::StatusOr<TradeRequest>& trade_request,
                    int32_t user_count, int32_t item_count,
                    int32_t wantlist_count, int32_t longest_wantlist,
                    int32_t missing_item_count = 0) {
  ASSERT_TRUE(trade_request.ok()) << trade_request.status().message();

  // Verifies the number of users with items.
  EXPECT_THAT(trade_request->users(), SizeIs(Eq(user_count)));

  // Verifies the number of items.
  EXPECT_EQ(trade_request->item_count(), item_count);

  // Verifies that the item map is well-formed.
  EXPECT_THAT(trade_request->items(), Each(HasIdAsKey()));

  // Verifies that every item has a username.
  EXPECT_THAT(
      trade_request->items(),
      Each(Mapped(Property("username", &Item::has_username, IsTrue()))));

  const auto& wantlists = trade_request->wantlists();

  // Verifies the number of wantlists.
  EXPECT_EQ(wantlists.size(), wantlist_count);

  // Verifies the id format of all items.
  EXPECT_THAT(trade_request->items(), Each(Mapped(IsValidItemId())));

  // Verifies the longest wantlist.
  EXPECT_THAT(trade_request->wantlists(),
              Contains(Property(&Wantlist::wanted_size, Eq(longest_wantlist))));

  // Verifies the number of missing items.
  if (missing_item_count) {
    EXPECT_THAT(trade_request->missing_items(),
                ElementsAre(AllOf(Property(&RemovedItem::wanted_item_id,
                                           StrCaseEq("missing-official")),
                                  Property(&RemovedItem::frequency,
                                           Eq(missing_item_count)))));
  }
}

// Parses `filename` and runs a number of checks against the result.
void ExpectWantlist(std::string_view filename, int32_t user_count,
                    int32_t item_count, int32_t wantlist_count,
                    int32_t longest_wantlist, int32_t missing_item_count = 0) {
  MathParser parser;
  const auto result = parser.ParseFile(filename);
  ExpectWantlist(result, user_count, item_count, wantlist_count,
                 longest_wantlist, missing_item_count);

  // Verifies that there are no repeated items. If a file does so, use the
  // overloaded ExpectWantlist(const absl::StatusOr<...> ...) and manually
  // check the repeated items afterwards.
  EXPECT_EQ(result->duplicate_items_size(), 0);
}

// Test suites: OLWLG trades.

TEST(MathParserOlwlgWorldTest, TestMarch2021Worldwide) {
  static constexpr int64_t kUserCount = 139;
  static constexpr int64_t kItemCount = 9056;
  static constexpr int64_t kWantlistCount = 13035;
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  static constexpr int64_t kLongestWantlistSize = 1000;
  static constexpr int64_t kMissingItemCount = 184;

  MathParser parser;
  const auto result = parser.ParseFile("olwlg_data/283180-officialwants.txt");

  ExpectWantlist(result, kUserCount, kItemCount, kWantlistCount,
                 kLongestWantlistSize, kMissingItemCount);

  // Number of duplicate items can be retrieved from the result file
  // "https://bgg.activityclub.org/olwlg/283180-results-official.txt"
  // as follows:
  //    grep "is repeated" ${result_file} | uniq | wc -l
  EXPECT_EQ(result->duplicate_items_size(), 28247);

  // Verifies that all duplicate items originate from the same user.
  const auto& duplicates = result->duplicate_items();
  EXPECT_THAT(duplicates, Each(WantlistOwnerOfRemovedStrCaseEq(
                              "tigersareawesome", result->items())));

  // Checks one duplicate item.
  EXPECT_THAT(
      duplicates,
      Contains(
          AllOf(Property(&RemovedItem::wanted_item_id, StartsWith("%8153405")),
                Property(&RemovedItem::offered_item_id, StrEq("8177732")),
                Property(&RemovedItem::frequency, Eq(3)))));
}

TEST(MathParserOlwlgCountryTest, TestJune2021US) {
  static constexpr int64_t kUserCount = 335;
  static constexpr int64_t kItemCount = 5896;
  static constexpr int64_t kWantlistCount = 14860;
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  static constexpr int64_t kLongestWantlistSize = 695;
  static constexpr int64_t kMissingItemCount = 134;

  ExpectWantlist("olwlg_data/286101-officialwants.txt", kUserCount, kItemCount,
                 kWantlistCount, kLongestWantlistSize, kMissingItemCount);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Norway) {
  static constexpr int64_t kUserCount = 18;
  static constexpr int64_t kItemCount = 143;
  static constexpr int64_t kWantlistCount = 142;
  // Longest wantlist: line 517: "(nils777) 8339221 ..."
  static constexpr int64_t kLongestWantlistSize = 35;

  ExpectWantlist("olwlg_data/286103-officialwants.txt", kUserCount, kItemCount,
                 kWantlistCount, kLongestWantlistSize);
}

TEST(MathParserOlwlgCountryTest, TestJune2021UK) {
  static constexpr int64_t kUserCount = 223;
  static constexpr int64_t kItemCount = 2990;
  static constexpr int64_t kWantlistCount = 9549;
  static constexpr int64_t kLongestWantlistSize = 785;
  // Longest wantlist: line 19783: "(jgoyes) 1109-3GIFT ..."
  static constexpr int64_t kMissingItemCount = 150;

  MathParser parser;
  const auto result = parser.ParseFile("olwlg_data/286149-officialwants.txt");

  ExpectWantlist(result, kUserCount, kItemCount, kWantlistCount,
                 kLongestWantlistSize, kMissingItemCount);

  // User who lists owned items as wanted.
  static constexpr std::string_view kOwner = "MLBath";

  // Verifies that the following items have been removed as they have been
  // listed as wanted by their owner.
  EXPECT_THAT(
      result->owned_items(),
      UnorderedElementsAre(
          Property(&RemovedItem::wanted_item_id, StrCaseEq("8320574-COPY1")),
          Property(&RemovedItem::wanted_item_id, StrCaseEq("8320574-COPY2"))));

  // Verifies the offered item and the frequency of the removed items.
  EXPECT_THAT(result->owned_items(),
              Each(AllOf(Property(&RemovedItem::offered_item_id,
                                  StartsWith("%8320574")),
                         Property(&RemovedItem::frequency, Eq(1)))));

  // Verifies that the above items are all owned by kOwner.
  EXPECT_THAT(
      result->items(),
      Contains(Mapped(AllOf(
                   Property(&Item::username, StrCaseEq(kOwner)),
                   Property(&Item::id,
                            MatchesRegex("(8320574-COPY[12])|(%8320574.*)")))))
          .Times(3));

  // No duplicate items.
  EXPECT_EQ(result->duplicate_items_size(), 0);
}

TEST(MathParserOlwlgCountryTest, TestJune2021Canada) {
  static constexpr int64_t kUserCount = 121;
  static constexpr int64_t kItemCount = 1147;
  static constexpr int64_t kWantlistCount = 1266;
  // Longest wantlist: line 2149: "(Dragoon6542) 8351711 ..."
  static constexpr int64_t kLongestWantlistSize = 289;
  static constexpr int64_t kMissingItemCount = 87;

  ExpectWantlist("olwlg_data/286870-officialwants.txt", kUserCount, kItemCount,
                 kWantlistCount, kLongestWantlistSize, kMissingItemCount);
}

TEST(MathParserOlwlgCountryTest, TestJuly2021Greece) {
  static constexpr int64_t kUserCount = 66;
  static constexpr int64_t kItemCount = 687;
  static constexpr int64_t kWantlistCount = 1003;
  // Longest wantlist: line 3417: "(Rico13mpatsoni) 8366279 ..."
  static constexpr int64_t kLongestWantlistSize = 235;

  ExpectWantlist("olwlg_data/286928-officialwants.txt", kUserCount, kItemCount,
                 kWantlistCount, kLongestWantlistSize);
}
}  // namespace
