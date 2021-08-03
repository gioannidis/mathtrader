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

#include "mathtrader/network/internal/arc_builder.h"

#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/arc.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wanted_item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace {
using ::mathtrader::network::internal::ArcBuilder;
using ::mathtrader::Arc;
using ::mathtrader::Item;
using ::mathtrader::ParserResult;
using ::mathtrader::WantedItem;
using ::mathtrader::Wantlist;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::StartsWith;

// Defines the test wantlists as a vector of wantlists. In each wantlist, the
// first element is the offered item, the rest are the wanted items.
using WantlistVector = std::vector<std::vector<std::string>>;

// Counts the number of elements in a 2D-vector.
template<typename T>
int64_t count2d(std::vector<std::vector<T>> vector_2d) {
  int64_t num_elements = 0;
  for (const std::vector<T>& vec : vector_2d) {
    num_elements += vec.size();
  }
  return num_elements;
}

// Builds the ParserResult proto out of the wantlist vector.
ParserResult BuildParserResult(const WantlistVector& wantlists) {
  ParserResult parser_result;
  for (const auto& wantlist_vector : wantlists) {
    Wantlist* const wantlist = parser_result.add_wantlists();

    // Sets the offered item as the first element of the vector.
    wantlist->mutable_offered_item()->set_id(wantlist_vector[0]);

    // Sets each wanted item and its priority.
    for (unsigned i = 1; i < wantlist_vector.size(); ++i) {
      Item* const wanted_item = wantlist->add_wanted_item();
      wanted_item->set_id(wantlist_vector[i]);
      wanted_item->SetExtension(WantedItem::priority, i);
    }
  }
  return parser_result;
}

// Encodes the expected arc frequency of a given item.
struct ItemArcFrequency {
  const std::string item_id;
  const int64_t head_count;  // number of arcs where item is the arc head.
  const int64_t tail_count;  // number of arcs where item is the arc tail.
};

// Verifies the number of Arcs for a given item.
void ExpectArcFrequencies(const ArcBuilder::ArcContainer& arcs,
                          const ItemArcFrequency& frequencies) {
  const std::string_view id = frequencies.item_id;
  const int64_t head_count = frequencies.head_count;
  const int64_t tail_count = frequencies.tail_count;
  EXPECT_THAT(
      arcs,
      Contains(Property(&Arc::head, StartsWith(id))).Times(head_count));
  EXPECT_THAT(
      arcs,
      Contains(Property(&Arc::tail, StartsWith(id))).Times(tail_count));
}

// As above, but for multiple items.
void ExpectArcFrequencies(const ArcBuilder::ArcContainer& arcs,
                          const std::vector<ItemArcFrequency>& frequencies) {
  for (const ItemArcFrequency& item_frequencies : frequencies) {
    ExpectArcFrequencies(arcs, item_frequencies);
  }
}

// Test suite: ArcBuilderTest

// Wantlists where all items are valid candidates, i.e., have their own offered
// wantlist and are wanted in another wantlist.
TEST(ArcBuilderTest, AllValidItems) {
  const WantlistVector wantlists = {
      {"A", "B", "C", "D"},
      {"B", "A", "E"},
      {"C", "B", "A"},
      {"D", "A"},
      {"E", "C", "A", "D"}};

  const auto arcs = ArcBuilder::BuildArcs(BuildParserResult(wantlists));

  // Verifies the total number of arcs: one arc for each item, since all items
  // are valid candidates.
  EXPECT_THAT(arcs, SizeIs(count2d(wantlists)));

  // Verifies the self-trading arcs.
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Gt(10'000))).Times(5));

  // Number of trading arcs with specific cost (priority).
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(1))).Times(5));
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(2))).Times(4));
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(3))).Times(2));
  EXPECT_THAT(
      arcs, Contains(Property(&Arc::cost, AllOf(Gt(4), Lt(10'000)))).Times(0));

  // Verifies the arc frequency for each item.
  ExpectArcFrequencies(arcs, {
      {"A", /*head_count=*/5, /*tail_count=*/4},
      {"B", /*head_count=*/3, /*tail_count=*/3},
      {"C", /*head_count=*/3, /*tail_count=*/3},
      {"D", /*head_count=*/3, /*tail_count=*/2},
      {"E", /*head_count=*/2, /*tail_count=*/4}});

  // All arcs have unit capacity.
  EXPECT_THAT(arcs, Each(Property(&Arc::capacity, Eq(1))));
}
}  // namespace
