// RUN: iree-run-mlir -iree-hal-target-backends=vmvx %s | FileCheck %s
// RUN: iree-run-mlir -iree-hal-target-backends=dylib-llvm-aot %s | FileCheck %s
// RUN: [[ $IREE_VULKAN_DISABLE == 1 ]] || (iree-run-mlir -iree-hal-target-backends=vulkan-spirv %s | FileCheck %s)

// CHECK-LABEL: EXEC @scalar
func.func @scalar() -> i32 {
  %result = util.unfoldable_constant 42 : i32
  return %result : i32
}
// CHECK: i32=42
