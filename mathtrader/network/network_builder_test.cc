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

#include "mathtrader/network/network_builder.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/flow_network.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wantlist.pb.h"

namespace {
using ::mathtrader::FlowNetwork;
using ::mathtrader::Item;
using ::mathtrader::ParserResult;
using ::mathtrader::Wantlist;
using ::mathtrader::network::NetworkBuilder;

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
  const WantlistVector wantlists = {{"A", "B", "C", "non-offered", "D"},
                                    {"B", "A", "empty-wantlist", "E"},
                                    {"unwanted", "A", "B", "C", "D", "E", "F"},
                                    {"C", "B", "also-non-offered", "A"},
                                    {"also-unwanted", "D", "A"},
                                    {"empty-wantlist"},
                                    {"E", "C", "A", "D"}};

  // Number of wantlists, excluding offered items that are never wanted or have
  // empty wantlists.
  constexpr static int32_t kValidOfferedItems = 4;

  FlowNetwork const flow_network =
      NetworkBuilder::BuildNetwork(BuildParserResult(wantlists));

  EXPECT_EQ(flow_network.nodes_size(), 2 * kValidOfferedItems);
}
}  // namespace
