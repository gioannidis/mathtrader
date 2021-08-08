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

#include "mathtrader/assignment/network_builder.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/assignment.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"

namespace {
using ::mathtrader::Assignment;
using ::mathtrader::Item;
using ::mathtrader::ParserResult;
using ::mathtrader::Wantlist;
using ::mathtrader::network::NetworkBuilder;
using ::testing::Key;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

// Defines the test wantlists as a vector of wantlists. In each wantlist, the
// first element is the offered item, the rest are the wanted items.
using WantlistVector = std::vector<std::vector<std::string>>;

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

TEST(NetworkBuilderTest, UnwantedItemsAndEmptyWantlists) {
  // Defines the test wantlists; first item is offered, the rest are wanted.
  const WantlistVector wantlists = {
      {"A", "B", "C", "non-offered_1", "empty_wantlist_1"},
      {"B", "A", "empty-wantlist_1", "E"},
      {"unwanted_1", "A", "B", "C", "empty_wantlist_1", "empty_wanlist_2", "F"},
      {"C", "B", "non_offered_2", "A"},
      {"unwanted_2", "empty_wantlist_1", "A"},
      {"empty_wantlist_1"},
      {"E", "C", "A", "empty_wantlist_2"},
      {"empty_wantlist_2"}};

  // Number of offered items, excluding offered items and items that are never
  // wanted or have empty wantlists.
  constexpr static int32_t kValidWantedItems = 8;

  Assignment const assignment =
      NetworkBuilder::BuildNetwork(BuildParserResult(wantlists));

  EXPECT_THAT(assignment.arcs(), SizeIs(kValidWantedItems));
  EXPECT_THAT(assignment.items(),
              UnorderedElementsAre(Key("A"), Key("B"), Key("C"), Key("E")));
}
}  // namespace
