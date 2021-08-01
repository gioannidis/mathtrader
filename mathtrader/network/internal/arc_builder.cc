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
#include "absl/strings/str_cat.h"
#include "ortools/base/logging.h"
#include "ortools/base/map_util.h"

#include "mathtrader/common/arc.pb.h"
#include "mathtrader/common/item.pb.h"
#include "mathtrader/common/wanted_item.pb.h"
#include "mathtrader/common/wantlist.pb.h"
#include "mathtrader/network/internal/node_util.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network::internal {
namespace {
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
  AddArc(GetOfferedNodeId(offered.id()), GetWantedNodeId(wanted.id()),
         cost, arcs);
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
}  // namespace

// Generates Arcs from the parser result.
// TODO(gioannidis) add self-trading arc for offered items without a wantlist.
ArcBuilder::ArcContainer ArcBuilder::BuildArcs(
    const ParserResult& parser_result) {
  // Tracks duplicate arcs.
  ArcMap arc_map;

  for (const Wantlist& wantlist : parser_result.wantlists()) {
    // Adds a self-trading arc on the offered item, representing the item not
    // getting traded.
    const Item& offered_item = wantlist.offered_item();
    AddArc(offered_item, &arc_map);

    // Adds an arc between the offered item and all wanted items.
    for (const Item& wanted_item : wantlist.wanted_item()) {
      AddArc(offered_item, wanted_item, &arc_map);
    }
  }

  // The Arc Container to return.
  ArcContainer arcs;

  // Moves the arcs to the Arc Container.
  while (!arc_map.empty()) {
    auto internal_node = arc_map.extract(arc_map.begin());
    *arcs.Add() = std::move(internal_node.mapped());
  }
  return arcs;
}
}  // namespace mathtrader::network::internal
