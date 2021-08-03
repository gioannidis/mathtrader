
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

#ifndef MATHTRADER_NETWORK_INTERNAL_ARC_BUILDER_H_
#define MATHTRADER_NETWORK_INTERNAL_ARC_BUILDER_H_

#include "mathtrader/common/arc.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network::internal {
// Generates Arcs from the given wantlists items. Each wantlist generates an arc
// from the offered item to each wanted item. The arc's cost is taken from the
// wanted item's priority. A self-trading arc is also added for each offered
// item with a very high cost.
//
// As an example, consider the wantlist "A : B %C D" with default options. Item
// "C" is a dummy item. The ArcBuilder generates the following arcs:
//
//    A+ ->  A- (self-trading, cost >> 1)
//    A+ ->  B- (cost = 1)
//    A+ -> %C- (cost = 2)
//    A+ ->  D- (cost = 3)
//
// Note that ArcBuilder aggressively prunes:
// * Wanted items that do not have their own wantlist, i.e., that are never
//   offered.
// * Offered items with empty wantlists.
// * Offered items that are never wanted.
//
// No Arcs are generated when an offered or wanted item is pruned.
// As a pruning example, consider the wantlists:
//
//    A : B C E
//    B : D E
//    C :
//    F : B C
//
// Items D and E are pruned because they have no respective wantlist, so they
// are never traded. Item C is pruned because its wantlist is empty. Item F is
// not wanted by anyone, so it is also pruned. The arcs that are generated are:
// A->A, A-B, B->B
//
// Usage:
//
//     ParserResult parser_result;
//     // ... populates parser_result
//     auto arcs = ArcBuilder::BuildArcs(parser_result);
class ArcBuilder {
 public:
  // Map type that indexes Arcs by Arc::id.
  using ArcContainer = google::protobuf::RepeatedField<Arc>;

  ArcBuilder() = default;

  // Disables copy constructor and move assignment.
  ArcBuilder(const ArcBuilder&) = delete;
  ArcBuilder& operator=(const ArcBuilder&) = delete;

  // Generates Arcs from the parser result. Prunes items as follows:
  // 1. Detects all items that have a valid wantlist, i.e., being offered.
  // 2. Detects all items that are valid trade candidates. These are all items
  //    that have a valid wantlist as an offered item and appear in at least one
  //    other wantlist.
  static ArcContainer BuildArcs(const ParserResult& parser_result);
};
}  // namespace mathtrader::network::internal
#endif  // MATHTRADER_NETWORK_INTERNAL_ARC_BUILDER_H_