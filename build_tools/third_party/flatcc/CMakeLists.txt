# Copyright 2020 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(FLATCC_ROOT "${IREE_ROOT_DIR}/third_party/flatcc/")

external_cc_library(
  PACKAGE
    flatcc
  NAME
    runtime
  ROOT
    ${FLATCC_ROOT}
  INCLUDES
    "${FLATCC_ROOT}/include"
  SRCS
    "src/runtime/builder.c"
    "src/runtime/emitter.c"
    "src/runtime/json_parser.c"
    "src/runtime/json_printer.c"
    "src/runtime/refmap.c"
  HDRS
    "include/flatcc/flatcc_accessors.h"
    "include/flatcc/flatcc_alloc.h"
    "include/flatcc/flatcc_assert.h"
    "include/flatcc/flatcc_builder.h"
    "include/flatcc/flatcc_emitter.h"
    "include/flatcc/flatcc_endian.h"
    "include/flatcc/flatcc_epilogue.h"
    "include/flatcc/flatcc_flatbuffers.h"
    "include/flatcc/flatcc_identifier.h"
    "include/flatcc/flatcc_iov.h"
    "include/flatcc/flatcc_json_parser.h"
    "include/flatcc/flatcc_json_printer.h"
    "include/flatcc/flatcc_portable.h"
    "include/flatcc/flatcc_prologue.h"
    "include/flatcc/flatcc_refmap.h"
    "include/flatcc/flatcc_rtconfig.h"
    "include/flatcc/flatcc_types.h"
    "include/flatcc/flatcc_unaligned.h"
    "include/flatcc/flatcc_verifier.h"
    "include/flatcc/reflection/flatbuffers_common_builder.h"
    "include/flatcc/reflection/flatbuffers_common_reader.h"
  DEPS
    flatcc::parsing
  PUBLIC
)

# A limited version of :runtime with only enough to parse and verify.
external_cc_library(
  PACKAGE
    flatcc
  NAME
    parsing
  ROOT
    ${FLATCC_ROOT}
  INCLUDES
    "${FLATCC_ROOT}/include"
  SRCS
    "config/config.h"
    "src/runtime/verifier.c"
  HDRS
    "include/flatcc/flatcc_accessors.h"
    "include/flatcc/flatcc_alloc.h"
    "include/flatcc/flatcc_assert.h"
    "include/flatcc/flatcc_endian.h"
    "include/flatcc/flatcc_epilogue.h"
    "include/flatcc/flatcc_flatbuffers.h"
    "include/flatcc/flatcc_identifier.h"
    "include/flatcc/flatcc_portable.h"
    "include/flatcc/flatcc_prologue.h"
    "include/flatcc/flatcc_rtconfig.h"
    "include/flatcc/flatcc_types.h"
    "include/flatcc/flatcc_unaligned.h"
    "include/flatcc/flatcc_verifier.h"
    "include/flatcc/reflection/flatbuffers_common_reader.h"
  PUBLIC
)

# Define our own binary target for the CLI, for host builds only.
# flatcc's `flatcc_cli` target renames itself to `flatcc` or `flatcc_d`
# depending on the build configuration, so we prefer to just set our own name.
if(NOT CMAKE_CROSSCOMPILING)
  add_executable(iree-flatcc-cli
    "${FLATCC_ROOT}/src/cli/flatcc_cli.c"
    "${FLATCC_ROOT}/external/hash/cmetrohash64.c"
    "${FLATCC_ROOT}/external/hash/str_set.c"
    "${FLATCC_ROOT}/external/hash/ptr_set.c"
    "${FLATCC_ROOT}/src/compiler/hash_tables/symbol_table.c"
    "${FLATCC_ROOT}/src/compiler/hash_tables/scope_table.c"
    "${FLATCC_ROOT}/src/compiler/hash_tables/name_table.c"
    "${FLATCC_ROOT}/src/compiler/hash_tables/schema_table.c"
    "${FLATCC_ROOT}/src/compiler/hash_tables/value_set.c"
    "${FLATCC_ROOT}/src/compiler/fileio.c"
    "${FLATCC_ROOT}/src/compiler/parser.c"
    "${FLATCC_ROOT}/src/compiler/semantics.c"
    "${FLATCC_ROOT}/src/compiler/coerce.c"
    "${FLATCC_ROOT}/src/compiler/codegen_schema.c"
    "${FLATCC_ROOT}/src/compiler/flatcc.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_reader.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_sort.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_builder.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_verifier.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_sorter.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_json_parser.c"
    "${FLATCC_ROOT}/src/compiler/codegen_c_json_printer.c"
    "${FLATCC_ROOT}/src/runtime/builder.c"
    "${FLATCC_ROOT}/src/runtime/emitter.c"
    "${FLATCC_ROOT}/src/runtime/refmap.c"
  )
  target_include_directories(iree-flatcc-cli SYSTEM
    PUBLIC
      "${FLATCC_ROOT}/external"
      "${FLATCC_ROOT}/include"
      "${FLATCC_ROOT}/config"
  )
endif()
