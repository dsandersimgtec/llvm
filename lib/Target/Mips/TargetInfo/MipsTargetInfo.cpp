//===-- MipsTargetInfo.cpp - Mips Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Mips.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::TheMipsTarget, llvm::TheMipselTarget;
Target llvm::TheMips64Target, llvm::TheMips64elTarget;

extern "C" void LLVMInitializeMipsTargetInfo() {
  RegisterTarget<TargetTuple::mips,
                 /*HasJIT=*/true> X(TheMipsTarget, "mips", "Mips");

  RegisterTarget<TargetTuple::mipsel,
                 /*HasJIT=*/true> Y(TheMipselTarget, "mipsel", "Mipsel");

  RegisterTarget<TargetTuple::mips64,
                 /*HasJIT=*/true> A(TheMips64Target, "mips64",
                                    "Mips64 [experimental]");

  RegisterTarget<TargetTuple::mips64el,
                 /*HasJIT=*/true> B(TheMips64elTarget, "mips64el",
                                    "Mips64el [experimental]");
}
