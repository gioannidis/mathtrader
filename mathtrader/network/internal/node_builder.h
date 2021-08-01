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

#include "mathtrader/common/node.pb.h"
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
//     auto node = NodeBuilder::BuildNodes(parser_result);
class NodeBuilder {
 public:
  // Map type that indexes Nodes by Node::id.
  using NodeContainer = google::protobuf::RepeatedField<Node>;

  NodeBuilder() = default;

  // Disables copy constructor and move assignment.
  NodeBuilder(const NodeBuilder&) = delete;
  NodeBuilder& operator=(const NodeBuilder&) = delete;

  // Generates Nodes from the parser result.
  static NodeContainer BuildNodes(const ParserResult& parser_result);
};
}  // namespace mathtrader::network::internal
#endif  // MATHTRADER_NETWORK_INTERNAL_NODE_BUILDER_H_
