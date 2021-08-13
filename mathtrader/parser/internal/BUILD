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

# Rules to build the stateless module that parses official item names.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

# Rules to build the stateless module that parses official item names.
cc_library(
    name = "item_parser",
    srcs = ["item_parser.cc"],
    hdrs = ["item_parser.h"],
    deps = [
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:offered_item_cc_proto",
        "//mathtrader/parser/util:item_util",
        "//mathtrader/util:strings",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_re2//:re2",
    ],
)

cc_test(
    name = "item_parser_test",
    size = "small",
    srcs = ["item_parser_test.cc"],
    deps = [
        ":item_parser",
        "//mathtrader/common:offered_item_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Rules to build the internal wantlist extensions.
proto_library(
    name = "internal_wantlist_proto",
    srcs = ["internal_wantlist.proto"],
    deps = ["//mathtrader/common:wantlist_proto"],
)

cc_proto_library(
    name = "internal_wantlist_cc_proto",
    deps = [":internal_wantlist_proto"],
)

# Rules to build the stateless module that parses wantlists.
cc_library(
    name = "wantlist_parser",
    srcs = ["wantlist_parser.cc"],
    hdrs = ["wantlist_parser.h"],
    deps = [
        ":internal_wantlist_cc_proto",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/parser/util:item_util",
        "//mathtrader/util:strings",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_re2//:re2",
    ],
)

cc_test(
    name = "wantlist_parser_test",
    size = "small",
    srcs = ["wantlist_parser_test.cc"],
    deps = [
        ":internal_wantlist_cc_proto",
        ":wantlist_parser",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/util:strings",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Rules to build the internal, stateful parser.
cc_library(
    name = "internal_parser",
    srcs = [
        "internal_parser.cc",
        "internal_parser_api.cc",
        "internal_parser_directives.cc",
    ],
    hdrs = ["internal_parser.h"],
    visibility = ["//mathtrader/parser:__pkg__"],
    deps = [
        ":item_parser",
        ":wantlist_parser",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "//mathtrader/parser:parser_result_cc_proto",
        "//mathtrader/parser/util:item_util",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_or_tools//ortools/base:filelineiter",
        "@com_google_or_tools//ortools/base:map_util",
        "@com_google_re2//:re2",
    ],
)

cc_test(
    name = "internal_parser_test",
    size = "small",
    srcs = ["internal_parser_test.cc"],
    deps = [
        ":internal_parser",
        "//mathtrader/common:item_cc_proto",
        "//mathtrader/common:wantlist_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
