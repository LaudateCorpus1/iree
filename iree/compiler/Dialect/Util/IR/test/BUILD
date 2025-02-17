# Copyright 2019 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

load("//build_tools/bazel:iree_lit_test.bzl", "iree_lit_test_suite")
load("//build_tools/bazel:enforce_glob.bzl", "enforce_glob")

package(
    default_visibility = ["//visibility:public"],
    features = ["layering_check"],
    licenses = ["notice"],  # Apache 2.0
)

iree_lit_test_suite(
    name = "lit",
    srcs = enforce_glob(
        [
            "alignment_folding.mlir",
            "alignment_ops.mlir",
            "attributes.mlir",
            "byte_buffer_ops.mlir",
            "global_folding.mlir",
            "global_ops.mlir",
            "hint_folding.mlir",
            "hint_ops.mlir",
            "list_ops.mlir",
            "numeric_ops.mlir",
            "range_folding.mlir",
            "range_ops.mlir",
            "structural_folding.mlir",
            "structural_ops.mlir",
        ],
        include = ["*.mlir"],
    ),
    tools = [
        "//iree/tools:iree-opt",
        "@llvm-project//llvm:FileCheck",
    ],
)
