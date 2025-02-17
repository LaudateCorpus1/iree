// Copyright 2020 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <memory>
#include <utility>

#include "iree/compiler/Codegen/PassDetail.h"
#include "iree/compiler/Codegen/Passes.h"
#include "iree/compiler/Dialect/Util/IR/UtilOps.h"
#include "iree/compiler/Dialect/Util/IR/UtilTypes.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

namespace mlir {
namespace iree_compiler {
namespace {

/// Any fp32 derived type is illegal.
static bool isIllegalType(Type type) {
  if (type.isF32()) return true;
  if (auto ptrType = type.dyn_cast<IREE::Util::PtrType>()) {
    return isIllegalType(ptrType.getTargetType());
  }
  if (auto shapedType = type.dyn_cast<ShapedType>()) {
    return isIllegalType(shapedType.getElementType());
  }
  return false;
}

class FloatTypeConverter : public TypeConverter {
 public:
  static Type convertTensor(RankedTensorType type) {
    if (!type.getElementType().isF32()) return type;
    auto newType = RankedTensorType::get(type.getShape(),
                                         Float16Type::get(type.getContext()));
    return newType;
  }
  explicit FloatTypeConverter() {
    addConversion([](Type type) { return type; });
    addConversion([&](FloatType type) {
      if (type.isF32()) return FloatType::getF16(type.getContext());
      return type;
    });
    addConversion(convertTensor);
    addConversion([&](IREE::Util::PtrType ptrType) {
      if (auto tensorType =
              ptrType.getTargetType().dyn_cast<RankedTensorType>()) {
        return IREE::Util::PtrType::get(convertTensor(tensorType));
      }
      return ptrType;
    });
  }
};

// Generic pattern to convert FP32 values and attributes to FP16.
class GenericTypeConvert : public ConversionPattern {
 public:
  GenericTypeConvert(MLIRContext *context, TypeConverter &converter)
      : ConversionPattern(converter, MatchAnyOpTypeTag(), 0, context) {}
  LogicalResult matchAndRewrite(
      Operation *op, ArrayRef<Value> operands,
      ConversionPatternRewriter &rewriter) const override {
    llvm::SmallVector<NamedAttribute, 4> newAttr;
    convertAttributes(op->getAttrs(), rewriter, newAttr);
    llvm::SmallVector<Type, 4> newResults;
    (void)getTypeConverter()->convertTypes(op->getResultTypes(), newResults);
    OperationState state(op->getLoc(), op->getName().getStringRef(), operands,
                         newResults, newAttr, op->getSuccessors());
    for (Region &r : op->getRegions()) {
      Region *newRegion = state.addRegion();
      rewriter.inlineRegionBefore(r, *newRegion, newRegion->begin());
      TypeConverter::SignatureConversion result(newRegion->getNumArguments());
      (void)getTypeConverter()->convertSignatureArgs(
          newRegion->getArgumentTypes(), result);
      rewriter.applySignatureConversion(newRegion, result);
    }
    Operation *newOp = rewriter.create(state);
    rewriter.replaceOp(op, newOp->getResults());
    return success();
  }

 protected:
  static void convertAttributes(ArrayRef<NamedAttribute> attrs,
                                ConversionPatternRewriter &rewriter,
                                SmallVectorImpl<NamedAttribute> &newAttrs) {
    for (auto attr : attrs) {
      if (auto fpAttr = attr.getValue().dyn_cast<DenseFPElementsAttr>()) {
        std::vector<llvm::APFloat> args;
        if (!fpAttr.getType().getElementType().isF32()) continue;
        for (llvm::APFloat f : fpAttr.getValues<APFloat>()) {
          bool losesInfo;
          f.convert(APFloat::IEEEhalf(), APFloat::rmTowardZero, &losesInfo);
          args.push_back(f);
        }
        auto tensorType = RankedTensorType::get(fpAttr.getType().getShape(),
                                                rewriter.getF16Type());
        newAttrs.emplace_back(attr.getName(),
                              DenseElementsAttr::get(tensorType, args));
      } else if (auto typeAttr = attr.getValue().dyn_cast<TypeAttr>()) {
        if (isIllegalType(typeAttr.getValue())) {
          if (auto tensorType =
                  typeAttr.getValue().dyn_cast<RankedTensorType>()) {
            Type newType = RankedTensorType::get(tensorType.getShape(),
                                                 rewriter.getF16Type());
            newAttrs.emplace_back(attr.getName(), TypeAttr::get(newType));
          }
        }
      } else {
        newAttrs.push_back(attr);
      }
    }
  }
};

struct DemoteF32ToF16Pass : public DemoteF32ToF16Base<DemoteF32ToF16Pass> {
  void runOnOperation() override {
    MLIRContext *context = &getContext();
    ModuleOp moduleOp = getOperation();

    FloatTypeConverter converter;
    RewritePatternSet patterns(&getContext());
    patterns.insert<GenericTypeConvert>(context, converter);
    populateFunctionOpInterfaceTypeConversionPattern<func::FuncOp>(patterns,
                                                                   converter);
    ConversionTarget target(*context);
    // Operations are legal if they don't contain any illegal type.
    target.markUnknownOpDynamicallyLegal([](Operation *op) {
      if (auto globalOp = dyn_cast<IREE::Util::GlobalOp>(op)) {
        return !isIllegalType(globalOp.type());
      }
      if (auto funcOp = dyn_cast<func::FuncOp>(op)) {
        for (Type type : funcOp.getFunctionType().getInputs()) {
          if (isIllegalType(type)) return false;
        }
        for (Type type : funcOp.getFunctionType().getResults()) {
          if (isIllegalType(type)) return false;
        }
      }
      for (Type type : op->getResultTypes()) {
        if (isIllegalType(type)) return false;
      }
      for (Type type : op->getOperandTypes()) {
        if (isIllegalType(type)) return false;
      }
      return true;
    });
    if (failed(applyFullConversion(moduleOp, target, std::move(patterns)))) {
      return signalPassFailure();
    }
  }
};

}  // namespace

std::unique_ptr<OperationPass<ModuleOp>> createDemoteF32ToF16Pass() {
  return std::make_unique<DemoteF32ToF16Pass>();
}

}  // namespace iree_compiler
}  // namespace mlir
