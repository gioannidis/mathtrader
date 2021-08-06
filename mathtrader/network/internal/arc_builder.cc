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

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "ortools/base/logging.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/flow_network.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wanted_item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/network/internal/node_util.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network::internal {
namespace {
// Generic set of item ids.
using ItemSet = absl::flat_hash_set<std::string>;

// Map of arcs indexed by "arc_id", defined as StrCat(tail_id, head_id).
using ArcMap = absl::flat_hash_map<std::string, Arc>;

// The cost of an Arc that represents an item not being traded. This is called
// a "self-trading" arc. Defined as a big number, so that the solver will try
// to maximize the traded items.
static constexpr int32_t kSelfTradingArcCost = (1 << 14);

// Adds an arc between two Nodes to the "arcs" set. Dies if the arc already
// exists.
void AddArc(std::string_view tail, std::string_view head, int64_t capacity,
            int64_t cost, ArcMap* arcs) {
  CHECK_NOTNULL(arcs);

  // Creates and initializes an arc.
  Arc arc;
  arc.set_tail(std::string(tail));
  arc.set_head(std::string(head));
  arc.set_capacity(capacity);
  arc.set_cost(cost);

  // Generates a unique arc_id to index the arc in the "arcs" set.
  std::string arc_id = absl::StrCat(tail, head);

  gtl::InsertOrDie(arcs, std::move(arc_id), std::move(arc));
}

// Same as above, but adds an arc with a unit capacity.
void AddArc(std::string_view tail, std::string_view head, int64_t cost,
            ArcMap* arcs) {
  AddArc(tail, head, /*capacity=*/1, cost, arcs);
}

// Same as above, but operates on the item ids, which are converted to the
// respective Offered/Wanted node ids. Arcs between items have unit capacity.
void AddArc(const Item& offered, const Item& wanted, int64_t cost,
            ArcMap* arcs) {
  AddArc(GetOfferedNodeId(offered.id()), GetWantedNodeId(wanted.id()), cost,
         arcs);
}

// Same as above, but takes the arc "cost" from the wanted item's priority.
// Dies if the `wanted` item has no priority extension.
// The cost is set to zero if the offered item is dummy.
void AddArc(const Item& offered, const Item& wanted, ArcMap* arcs) {
  CHECK(wanted.HasExtension(WantedItem::priority));
  const int64_t cost =
      (!offered.is_dummy() ? wanted.GetExtension(WantedItem::priority) : 0);
  AddArc(offered, wanted, cost, arcs);
}

// Same as above, but adds a self-trading arc on a single item with unit
// capacity.
void AddArc(const Item& item, ArcMap* arcs) {
  AddArc(/*offered=*/item, /*wanted=*/item, /*cost=*/kSelfTradingArcCost, arcs);
}

// Generates an ItemSet with all offered items that have a non-empty wantlist.
// Dies if any duplicate offered items are detected.
ItemSet GetOfferedItems(const ParserResult& input) {
  ItemSet item_set;
  for (const Wantlist& wantlist : input.wantlists()) {
    if (wantlist.wanted_item_size()) {
      const Item& offered_item = wantlist.offered_item();
      gtl::InsertOrDie(&item_set, offered_item.id());
    }
  }
  return item_set;
}

// Generates an ItemSet with all trading candidate items. These are defined as
// items that appear both as an offered item in a wantlist and as a wanted item
// in a different wantlist.
ItemSet GetCandidateItems(const ParserResult& input) {
  // The item set to return.
  ItemSet candidate_items;

  // First, gets the set of offered items.
  const ItemSet offered_items = GetOfferedItems(input);

  // Then, finds the intersection between offered and wanted items.
  for (const Wantlist& wantlist : input.wantlists()) {
    for (const Item& wanted_item : wantlist.wanted_item()) {
      const std::string_view id = wanted_item.id();
      if (offered_items.contains(id)) {
        // This is a valid candidate item, i.e., is an offered and wanted item
        // (in different wantlists).
        gtl::InsertIfNotPresent(&candidate_items, std::string(id));
      }
    }
  }
  return candidate_items;
}

// Removes all nodes from the flow network that are definitely not trading,
// i.e., not in the `candidate_items` set.
void RemoveNonTradingNodes(const ItemSet& candidate_items,
                           FlowNetwork* flow_network) {
  auto* nodes = flow_network->mutable_nodes();
  nodes->erase(
      std::remove_if(nodes->begin(), nodes->end(),
                     // Lambda that erases the node if the item is not a
                     // candidate item. The original item_id is looked up, which
                     // is a substring of the node id.
                     [&candidate_items](const Node& node) {
                       return !candidate_items.contains(node.item_id());
                     }),
      nodes->end());  // second argument of nodes->erase().
}
}  // namespace

// Generates Arcs from the parser result, adding them to `flow_network`.
// Prunes items as follows:
// 1. Detects all items that have a valid wantlist, i.e., being offered.
// 2. Detects all items that are valid trade candidates. These are all items
//    that have a valid wantlist as an offered item and appear in at least one
//    other wantlist.
//
// If both source and sink have been defined in `flow_network`, also adds an
// arc betwen the source/sink and each item, updating the source and sink
// production fields.
void ArcBuilder::BuildArcs(const ParserResult& parser_result,
                           FlowNetwork* flow_network) {
  // Tracks duplicate arcs.
  ArcMap arc_map;

  // Builds a set of candidate items that are both wanted and offered.
  const ItemSet candidate_items = GetCandidateItems(parser_result);

  // Generates self-trading arcs for each offered item and trading arcs between
  // the offered item and every wanted item.
  for (const Wantlist& wantlist : parser_result.wantlists()) {
    const Item& offered_item = wantlist.offered_item();

    // Skips offered items that are never wanted or with an empty wantlist.
    if (!candidate_items.contains(offered_item.id())) {
      continue;
    }

    // Adds a self-trading arc.
    AddArc(offered_item, &arc_map);

    // Adds an Arc for wanted items that are also being offered.
    for (const Item& wanted_item : wantlist.wanted_item()) {
      if (candidate_items.contains(wanted_item.id())) {
        AddArc(offered_item, wanted_item, &arc_map);
      }
    }
  }

  // Applies source/sink operations, if both have been defined.
  if (flow_network->has_source() && flow_network->has_sink()) {
    CHECK_NE(flow_network->source().id(), "") << "Empty source id not allowed";
    CHECK_NE(flow_network->sink().id(), "") << "Empty sink id not allowed";

    // Updates the source/sink productions equal to the actual number o
    // candidate items, as some items may have been pruned.
    flow_network->mutable_source()->set_production(candidate_items.size());
    flow_network->mutable_sink()->set_production(-1 * candidate_items.size());

    // Adds an arc between the source/sink and each candidate item with unit
    // capacity and zero cost.
    for (const std::string_view item_id : candidate_items) {
      AddArc(/*tail=*/flow_network->source().id(), /*head=*/item_id,
             /*capacity=*/1, /*cost=*/0, &arc_map);
      AddArc(/*tail=*/item_id, /*head=*/flow_network->sink().id(),
             /*capacity=*/1, /*cost=*/0, &arc_map);
    }
  }

  // Finally, removes non-trading nodes.
  RemoveNonTradingNodes(candidate_items, flow_network);

  // Moves the arcs to the FlowNetwork.
  while (!arc_map.empty()) {
    auto internal_node = arc_map.extract(arc_map.begin());
    *flow_network->add_arcs() = std::move(internal_node.mapped());
  }
}
}  // namespace mathtrader::network::internal
