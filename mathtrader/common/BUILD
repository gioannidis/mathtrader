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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
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

cc_library(
    name = "item_attributes",
    hdrs = ["item_attributes.h"],
    deps = [
        ":item_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "item_attributes_test",
    size = "small",
    srcs = ["item_attributes_test.cc"],
    deps = [
        ":item_attributes",
        ":item_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Build rules for offered items.
proto_library(
    name = "offered_item_proto",
    srcs = ["offered_item.proto"],
    deps = [":item_proto"],
)

cc_proto_library(
    name = "offered_item_cc_proto",
    deps = [":offered_item_proto"],
)

# Build rules for wanted items.
proto_library(
    name = "wanted_item_proto",
    srcs = ["wanted_item.proto"],
    deps = [":item_proto"],
)

cc_proto_library(
    name = "wanted_item_cc_proto",
    deps = [":wanted_item_proto"],
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

# Build rules for FlowNetwork.
proto_library(
    name = "arc_proto",
    srcs = ["arc.proto"],
)

cc_proto_library(
    name = "arc_cc_proto",
    deps = [":arc_proto"],
)

proto_library(
    name = "node_proto",
    srcs = ["node.proto"],
)

cc_proto_library(
    name = "node_cc_proto",
    deps = [":node_proto"],
)
