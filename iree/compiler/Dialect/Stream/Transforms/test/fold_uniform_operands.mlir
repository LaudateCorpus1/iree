// RUN: iree-opt -split-input-file -iree-stream-fold-uniform-operands %s | FileCheck %s

// Tests that duplicate primitive operands are folded if they are uniform at all
// dispatch sites.
//
// In this test (%a, %a) -> (%a0, %a1) at both dispatch sites and can be
// folded, while (%b, %b) -> (%b0, %b1) and (%c20, %b) -> (%b0, %b1) prevents
// the folding due to divergent values. The binding and %c should be untouched.

// CHECK-LABEL: @deduplicateOperandsEx
stream.executable private @deduplicateOperandsEx {
  stream.executable.export public @dispatch
  builtin.module  {
    // CHECK: func @dispatch(%[[BINDING:.+]]: !stream.binding, %[[A01:.+]]: i32, %[[B0:.+]]: index, %[[C:.+]]: i1, %[[B1:.+]]: index)
    func.func @dispatch(%binding: !stream.binding, %a0: i32, %b0: index, %c: i1, %a1: i32, %b1: index) {
      // CHECK-NEXT: util.do_not_optimize(%[[BINDING]]) : !stream.binding
      util.do_not_optimize(%binding) : !stream.binding
      // CHECK-NEXT: util.do_not_optimize(%[[A01]]) : i32
      util.do_not_optimize(%a0) : i32
      // CHECK-NEXT: util.do_not_optimize(%[[A01]]) : i32
      util.do_not_optimize(%a1) : i32
      // CHECK-NEXT: util.do_not_optimize(%[[B0]]) : index
      util.do_not_optimize(%b0) : index
      // CHECK-NEXT: util.do_not_optimize(%[[B1]]) : index
      util.do_not_optimize(%b1) : index
      // CHECK-NEXT: util.do_not_optimize(%[[C]]) : i1
      util.do_not_optimize(%c) : i1
      return
    }
  }
}
// CHECK: func @deduplicateOperands(%[[A:.+]]: i32, %[[B:.+]]: index, %[[C:.+]]: i1)
func.func @deduplicateOperands(%a: i32, %b: index, %c: i1) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c20 = arith.constant 20 : index
  %alloc = stream.resource.alloc uninitialized : !stream.resource<transient>{%c20}
  %result_timepoint = stream.cmd.execute with(%alloc as %capture: !stream.resource<transient>{%c20}) {
    // CHECK: stream.cmd.dispatch {{.+}}(%[[A]], %[[B]], %[[C]], %[[B]] : i32, index, i1, index)
    stream.cmd.dispatch @deduplicateOperandsEx::@dispatch[%c1, %c1, %c1](%a, %b, %c, %a, %b : i32, index, i1, i32, index) {
      rw %capture[%c0 for %c20] : !stream.resource<transient>{%c20}
    }
    // CHECK: stream.cmd.dispatch {{.+}}(%[[A]], %c20, %[[C]], %[[B]] : i32, index, i1, index)
    stream.cmd.dispatch @deduplicateOperandsEx::@dispatch[%c1, %c1, %c1](%a, %c20, %c, %a, %b : i32, index, i1, i32, index) {
      rw %capture[%c0 for %c20] : !stream.resource<transient>{%c20}
    }
  } => !stream.timepoint
  return
}

// -----

// Tests that operands that are uniformly constant at all dispatch sites are
// inlined into the dispatch regions.
//
// In this test %a is dynamic and ignored, %b is uniformly %c20 and inlined, and
// %c is divergent (%false/%true) and skipped.

// CHECK-LABEL: @inlineConstantOperandsEx
stream.executable private @inlineConstantOperandsEx {
  stream.executable.export public @dispatch
  builtin.module  {
    // CHECK: func @dispatch(%[[BINDING:.+]]: !stream.binding, %[[A:.+]]: i32, %[[C:.+]]: i1)
    func.func @dispatch(%binding: !stream.binding, %a: i32, %b: index, %c: i1) {
      // CHECK: %[[B:.+]] = arith.constant 20 : index
      // CHECK-NEXT: util.do_not_optimize(%[[BINDING]]) : !stream.binding
      util.do_not_optimize(%binding) : !stream.binding
      // CHECK-NEXT: util.do_not_optimize(%[[A]]) : i32
      util.do_not_optimize(%a) : i32
      // CHECK-NEXT: util.do_not_optimize(%[[B]]) : index
      util.do_not_optimize(%b) : index
      // CHECK-NEXT: util.do_not_optimize(%[[C]]) : i1
      util.do_not_optimize(%c) : i1
      return
    }
  }
}
// CHECK: func @inlineConstantOperands(%[[A:.+]]: i32)
func.func @inlineConstantOperands(%a: i32) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c20 = arith.constant 20 : index
  %false = arith.constant 0 : i1
  %true = arith.constant 1 : i1
  %alloc = stream.resource.alloc uninitialized : !stream.resource<transient>{%c20}
  %result_timepoint = stream.cmd.execute with(%alloc as %capture: !stream.resource<transient>{%c20}) {
    // CHECK: stream.cmd.dispatch {{.+}}(%[[A]], %false : i32, i1)
    stream.cmd.dispatch @inlineConstantOperandsEx::@dispatch[%c1, %c1, %c1](%a, %c20, %false : i32, index, i1) {
      rw %capture[%c0 for %c20] : !stream.resource<transient>{%c20}
    }
    // CHECK: stream.cmd.dispatch {{.+}}(%[[A]], %true : i32, i1)
    stream.cmd.dispatch @inlineConstantOperandsEx::@dispatch[%c1, %c1, %c1](%a, %c20, %true : i32, index, i1) {
      rw %capture[%c0 for %c20] : !stream.resource<transient>{%c20}
    }
  } => !stream.timepoint
  return
}
