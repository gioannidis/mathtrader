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
#include <string_view>

#include "absl/container/flat_hash_set.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/flow_network.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/node.pb.h"
#include "mathtrader/common/offered_item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/network/internal/node_util.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network::internal {

// Generates Nodes from the parser result, adding them to the `flow_network`.
void NodeBuilder::BuildNodes(const ParserResult& parser_result,
                             FlowNetwork* flow_network) {
  // Verifies that no duplicate offered items are encountered.
  absl::flat_hash_set<std::string> offered_items;

  // Processes the offered item from each wantlist.
  for (const Wantlist& wantlist : parser_result.wantlists()) {
    const Item& item = wantlist.offered_item();
    const std::string& item_id = item.id();

    // Verifies that the offered item is unique.
    gtl::InsertOrDie(&offered_items, item_id);

    // Creates the offered/wanted nodes.
    Node* const offered_node = flow_network->add_nodes();
    Node* const wanted_node = flow_network->add_nodes();

    // Creates unique ids.
    offered_node->set_id(GetOfferedNodeId(item_id));
    wanted_node->set_id(GetWantedNodeId(item_id));

    // Sets the item type.
    offered_node->set_item_type(Node::kOffered);
    wanted_node->set_item_type(Node::kWanted);

    // Sets the symmetric nodes.
    offered_node->set_symmetric_node(wanted_node->id());
    wanted_node->set_symmetric_node(offered_node->id());

    // Sets the original item_id.
    offered_node->set_item_id(item_id);
    wanted_node->set_item_id(item_id);

    // Sets the username.
    {
      const std::string& username = item.GetExtension(OfferedItem::username);
      offered_node->set_username(username);
      wanted_node->set_username(username);
    }
  }
}

}  // namespace mathtrader::network::internal
