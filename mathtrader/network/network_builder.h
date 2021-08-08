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

// Entry point of the network builder, which transforms a `ParserResult`
// message, representing wantlists, to a `Assignment` graph.

#ifndef MATHTRADER_NETWORK_NETWORK_BUILDER_H_
#define MATHTRADER_NETWORK_NETWORK_BUILDER_H_

#include "mathtrader/common/assignment.pb.h"
#include "mathtrader/parser/parser_result.pb.h"

namespace mathtrader::network {

class NetworkBuilder {
 public:
  NetworkBuilder() = default;

  // Disables copy constructor and move assignment.
  NetworkBuilder(const NetworkBuilder&) = delete;
  NetworkBuilder& operator=(const NetworkBuilder&) = delete;

  // Processes the OLWLG-generated wantlists and builds the Assignment.
  static Assignment BuildNetwork(const ParserResult& parser_result);
};
}  // namespace mathtrader::network
#endif  // MATHTRADER_NETWORK_NETWORK_BUILDER_H_