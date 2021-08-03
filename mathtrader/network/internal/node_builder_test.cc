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

#include "mathtrader/network/internal/node_builder.h"

#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mathtrader/common/flow_network.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/node.pb.h"
#include "mathtrader/common/offered_item.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace {
using ::mathtrader::network::internal::NodeBuilder;
using ::mathtrader::FlowNetwork;
using ::mathtrader::Item;
using ::mathtrader::Node;
using ::mathtrader::OfferedItem;
using ::mathtrader::ParserResult;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::Property;
using ::testing::StartsWith;
using ::testing::StrCaseEq;

// Matches a node where both the node id and the symmetric node id start with
// the original item id.
MATCHER(NodeIdsStartWithItemId, "") {
  return ExplainMatchResult(
      AllOf(Property(&Node::id, StartsWith(arg.item_id())),
            Property(&Node::symmetric_node, StartsWith(arg.item_id()))),
      arg, result_listener);
}

// Test suite: fundamental NodeBuilder tests.

TEST(NodeBuilderTest, Base) {
  // The offered items and usernames at the input.
  const std::vector<std::pair<std::string, std::string>> items_users = {
    {"abcd", "User1"}, {"0001-", "fooBarUser"}, {"0042-MKBG", "owner42"},
    {"Qwerty0123", "qwertzUser"}};

  // Propagates the offered items and usernames to the input.
  ParserResult input;
  for (const auto& pair : items_users) {
    Item* const item = input.add_wantlists()->mutable_offered_item();
    item->set_id(pair.first);
    item->SetExtension(OfferedItem::username, pair.second);
  }

  FlowNetwork flow_network;
  NodeBuilder::BuildNodes(input, &flow_network);

  // Two nodes were generated for each item.
  EXPECT_EQ(flow_network.nodes_size(), 2 * items_users.size());

  // Verifies that each item id is contained twice in the Nodes.
  EXPECT_THAT(flow_network.nodes(), AllOf(
      Contains(Property(&Node::id, StartsWith("abcd"))).Times(2),
      Contains(Property(&Node::id, StartsWith("0001-"))).Times(2),
      Contains(Property(&Node::id, StartsWith("0042-MKBG"))).Times(2),
      Contains(Property(&Node::id, StartsWith("Qwerty0123"))).Times(2)));

  // Verifies that no node is a source or sink.
  EXPECT_THAT(flow_network.nodes(),
              Each(Property(&Node::has_production, IsFalse())));

  // Verifies that half the nodes are offered and half are wanted items.
  EXPECT_THAT(flow_network.nodes(), AllOf(
      Contains(Property(&Node::item_type, Eq(Node::kOffered))).Times(4),
      Contains(Property(&Node::item_type, Eq(Node::kWanted))).Times(4)));

  // Verifies that the item id is a prefix of both the node id and the symmetric
  // node id.
  EXPECT_THAT(flow_network.nodes(), Each(NodeIdsStartWithItemId()));

  // Verifies that there are two nodes with each username.
  EXPECT_THAT(flow_network.nodes(), AllOf(
      Contains(Property(&Node::username, StrCaseEq("User1"))).Times(2),
      Contains(Property(&Node::username, StrCaseEq("fooBarUser"))).Times(2),
      Contains(Property(&Node::username, StrCaseEq("owner42"))).Times(2),
      Contains(Property(&Node::username, StrCaseEq("qwertzUser"))).Times(2)));
}

// Test suite: Death Tests

// Verifies that NodeBuilder dies if duplicate items are given.
TEST(NodeBuilderDeathTest, DuplicateItems) {
  static constexpr char item_id[] = "AnItemId";
  ParserResult input;
  for (int i = 0; i < 2; ++i) {
    input.add_wantlists()->mutable_offered_item()->set_id(item_id);
  }
  FlowNetwork flow_network;
  EXPECT_DEATH(NodeBuilder::BuildNodes(input, &flow_network), item_id);
}

}  // namespace
