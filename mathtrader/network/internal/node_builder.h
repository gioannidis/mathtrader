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

// Builds the trade network nodes.

#ifndef MATHTRADER_NETWORK_INTERNAL_NODE_BUILDER_H_
#define MATHTRADER_NETWORK_INTERNAL_NODE_BUILDER_H_

#include "mathtrader/common/flow_network.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network::internal {
// Generates Nodes from the wantlists' offered items. Each item generates two
// nodes, representing an "offered" and a wanted item.
//
// As an example, consider an offered item with id "0001-ITEM". It generates two
// nodes: "0001-ITEM+" (offered) and "0001-ITEM-" (wanted).
//
// Note that NodeBuilder aggressively prunes any wanted items that do not have
// their own wantlist, i.e., that are never offered. No Nodes are generated for
// these items. This reduces the size of the resulting FlowNetwork by removing
// the items that will never be traded.
//
// Usage:
//
//     ParserResult parser_result;
//     // ... populates parser_result
//     FlowNetwork flow_network;
//     NodeBuilder::BuildNodes(parser_result, &flow_network);
class NodeBuilder {
 public:
  NodeBuilder() = default;

  // Disables copy constructor and move assignment.
  NodeBuilder(const NodeBuilder&) = delete;
  NodeBuilder& operator=(const NodeBuilder&) = delete;

  // Generates Nodes from the parser result, adding them to the `flow_network`.
  // Adds two nodes for each item, a source and a sink.
  static void BuildNodes(const ParserResult& parser_result,
                         FlowNetwork* flow_network);
};
}  // namespace mathtrader::network::internal
#endif  // MATHTRADER_NETWORK_INTERNAL_NODE_BUILDER_H_
