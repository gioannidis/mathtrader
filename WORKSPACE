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

# Defines external repository dependencies for common types.

workspace(name = "gioannidis_mathtrader")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Integration of clang-tidy with bazel.
http_archive(
    name = "bazel_clang_tidy",
    sha256 = "8e1f43cd7c7c55cddeaedf94edc52ed41dd1b60c591eaa4d9ff977575b81cf6b",
    strip_prefix = "bazel_clang_tidy-1c3a983c054a41be73ac128e3d842a940850b5fe",
    urls = ["https://github.com/erenon/bazel_clang_tidy/archive/1c3a983c054a41be73ac128e3d842a940850b5fe.tar.gz"],
)

http_archive(
    name = "rules_proto",
    sha256 = "e0cab008a9cdc2400a1d6572167bf9c5afc72e19ee2b862d18581051efab42c9",
    strip_prefix = "rules_proto-c0b62f2f46c85c16cb3b5e9e921f0d00e3101934",
    urls = ["https://github.com/bazelbuild/rules_proto/archive/c0b62f2f46c85c16cb3b5e9e921f0d00e3101934.tar.gz"],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

# TODO(gioannidis) remove when `rules_proto` pulls `rules_cc` dependencies at
# any point after 2019-08-21.
# See: https://github.com/erenon/bazel_clang_tidy/issues/8
http_archive(
    name = "rules_cc",
    sha256 = "3cde212ccda3ba152897e7fd354c42eba275878b6d98fe4f2125c684a73f3842",
    strip_prefix = "rules_cc-d66a13e2a01630afcafc4ba411d83e291ecf02bd",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/d66a13e2a01630afcafc4ba411d83e291ecf02bd.tar.gz"],
)

http_archive(
    name = "com_google_absl",
    sha256 = "321de4d5582cda06b78880e7ce0580f38355b33f67041bf7779b7f66029d4f06",
    strip_prefix = "abseil-cpp-a2419e63b8ae3b924152822f3c9f9da67ff6e215",
    urls = ["https://github.com/abseil/abseil-cpp/archive/a2419e63b8ae3b924152822f3c9f9da67ff6e215.zip"],
)

http_archive(
    name = "com_google_googletest",
    sha256 = "1791ff1f4db4f8b4ae42ea9b401173816b9cdedc79f92819fd477778ae31000a",
    strip_prefix = "googletest-4ec4cd23f486bf70efcc5d2caa40f24368f752e3",
    urls = ["https://github.com/google/googletest/archive/4ec4cd23f486bf70efcc5d2caa40f24368f752e3.tar.gz"],
)

http_archive(
    name = "com_google_or_tools",
    sha256 = "40d87541981a7793d4b6ea47f737fed2799a144db64aca430fef8841e12c2518",
    strip_prefix = "or-tools-b37d9c786b69128f3505f15beca09e89bf078a89",
    urls = ["https://github.com/google/or-tools/archive/b37d9c786b69128f3505f15beca09e89bf078a89.tar.gz"],
)

http_archive(
    name = "com_google_re2",
    sha256 = "02fcb0b30819a08ab33966bbfb5c11686deffca634a0096574ccf1aca804bccd",
    strip_prefix = "re2-5aec8b553863350e32d13bbf745eae81adcdce93",
    urls = ["https://github.com/google/re2/archive/5aec8b553863350e32d13bbf745eae81adcdce93.tar.gz"],
)
