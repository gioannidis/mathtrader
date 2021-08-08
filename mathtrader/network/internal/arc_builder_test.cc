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

#include "mathtrader/network/internal/arc_builder.h"

#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/assignment.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace {
using ::mathtrader::Arc;
using ::mathtrader::Assignment;
using ::mathtrader::Item;
using ::mathtrader::ParserResult;
using ::mathtrader::Wantlist;
using ::mathtrader::network::internal::ArcBuilder;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::Property;
using ::testing::SizeIs;
using ::testing::StrEq;

// Defines the test wantlists as a vector of wantlists. In each wantlist, the
// first element is the offered item, the rest are the wanted items.
using WantlistVector = std::vector<std::vector<std::string>>;

// Counts the number of elements in a 2D-vector.
template <typename T>
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
      wanted_item->set_priority(i);
    }
  }
  return parser_result;
}

// Encodes the expected arc frequency of a given item.
struct ItemArcFrequency {
  const std::string item_id;
  const int64_t wanted_count;   // Number of arcs where item is the wanted item
  const int64_t offered_count;  // Number of arcs where item is the offered item
};

// Verifies the number of Arcs for a given item.
void ExpectArcFrequencies(const Assignment& assignment,
                          const ItemArcFrequency& frequencies) {
  const std::string_view id = frequencies.item_id;
  const int64_t wanted_count = frequencies.wanted_count;
  const int64_t offered_count = frequencies.offered_count;
  EXPECT_THAT(assignment.arcs(),
              Contains(Property(&Arc::wanted, StrEq(id))).Times(wanted_count));
  EXPECT_THAT(
      assignment.arcs(),
      Contains(Property(&Arc::offered, StrEq(id))).Times(offered_count));
}

// As above, but for multiple items.
void ExpectArcFrequencies(const Assignment& assignment,
                          const std::vector<ItemArcFrequency>& frequencies) {
  for (const ItemArcFrequency& item_frequencies : frequencies) {
    ExpectArcFrequencies(assignment, item_frequencies);
  }
}

// Test suite: ArcBuilderTest

// Wantlists where all items are valid candidates, i.e., have their own offered
// wantlist and are wanted in another wantlist.
TEST(ArcBuilderTest, AllValidItems) {
  const WantlistVector wantlists = {{"A", "B", "C", "D"},
                                    {"B", "A", "E"},
                                    {"C", "B", "A"},
                                    {"D", "A"},
                                    {"E", "C", "A", "D"}};

  Assignment assignment;
  ArcBuilder::BuildArcs(BuildParserResult(wantlists), &assignment);

  // Verifies the total number of arcs: one arc for wanted item. This is
  // computed by counting all elements of `wantlists`, minus the offered item,
  // i.e., the number of individual wantlists.
  const auto& arcs = assignment.arcs();
  EXPECT_THAT(arcs, SizeIs(count2d(wantlists) - wantlists.size()));

  // Number of trading arcs with specific cost (priority).
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(1))).Times(5));
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(2))).Times(4));
  EXPECT_THAT(arcs, Contains(Property(&Arc::cost, Eq(3))).Times(2));

  // Verifies the arc frequency for each item.
  ExpectArcFrequencies(assignment,
                       {{"A", /*wanted_count=*/4, /*offered_count=*/3},
                        {"B", /*wanted_count=*/2, /*offered_count=*/2},
                        {"C", /*wanted_count=*/2, /*offered_count=*/2},
                        {"D", /*wanted_count=*/2, /*offered_count=*/1},
                        {"E", /*wanted_count=*/1, /*offered_count=*/3}});

  // All arcs have unit capacity.
  EXPECT_THAT(arcs, Each(Property(&Arc::capacity, Eq(1))));
}
}  // namespace
