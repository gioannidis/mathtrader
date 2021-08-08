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

#include "mathtrader/assignment/internal/arc_builder.h"
#include "mathtrader/common/assignment.pb.h"

namespace mathtrader::assignment {
// Processes the OLWLG-generated wantlists and builds the Assignment.
Assignment AssignmentBuilder::BuildNetwork(const ParserResult& parser_result) {
  // The Assignment to return.
  Assignment assignment;

  // Builds all the arcs representing the offered-wanted item relationships.
  internal::ArcBuilder::BuildArcs(parser_result, &assignment);

  return assignment;
}
}  // namespace mathtrader::assignment
