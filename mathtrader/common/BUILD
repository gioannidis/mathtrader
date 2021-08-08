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

# Build rules for common types.

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(
    default_visibility = ["//mathtrader:__subpackages__"],
)

# Build rules for generic items.
proto_library(
    name = "item_proto",
    srcs = ["item.proto"],
)

cc_proto_library(
    name = "item_cc_proto",
    deps = [":item_proto"],
)

# Build rules for offered item extensions.
proto_library(
    name = "offered_item_proto",
    srcs = ["offered_item.proto"],
    deps = [":item_proto"],
)

cc_proto_library(
    name = "offered_item_cc_proto",
    deps = [":offered_item_proto"],
)

# Build rules for wantlists.
proto_library(
    name = "wantlist_proto",
    srcs = ["wantlist.proto"],
    deps = [":item_proto"],
)

cc_proto_library(
    name = "wantlist_cc_proto",
    deps = [":wantlist_proto"],
)
