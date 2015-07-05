//===-- ARMTargetInfo.cpp - ARM Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheARMLETarget,   llvm::TheARMBETarget;
Target llvm::TheThumbLETarget, llvm::TheThumbBETarget;

extern "C" void LLVMInitializeARMTargetInfo() {
  RegisterTarget<TargetTuple::arm, /*HasJIT=*/true> X(TheARMLETarget, "arm",
                                                      "ARM");
  RegisterTarget<TargetTuple::armeb, /*HasJIT=*/true> Y(TheARMBETarget, "armeb",
                                                        "ARM (big endian)");

  RegisterTarget<TargetTuple::thumb, /*HasJIT=*/true> A(TheThumbLETarget,
                                                        "thumb", "Thumb");
  RegisterTarget<TargetTuple::thumbeb, /*HasJIT=*/true> B(
      TheThumbBETarget, "thumbeb", "Thumb (big endian)");
}
