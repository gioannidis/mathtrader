# This file is part of mathtrader.
#
# Copyright (C) 2021 George Ioannidis
#
# mathtrader is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# mathtrader is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with mathtrader. If not, see <http://www.gnu.org/licenses/>.

# Defines internal BUILD rules for the "officialwants" file parser.

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

# Parser output.
proto_library(
    name = "parser_result_proto",
    srcs = ["parser_result.proto"],
    deps = ["//mathtrader/common:wantlist_proto"],
)

cc_proto_library(
    name = "parser_result_cc_proto",
    deps = [":parser_result_proto"],
)
