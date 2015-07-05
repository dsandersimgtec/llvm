//===-- BPFTargetInfo.cpp - BPF Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "BPF.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

namespace llvm {
Target TheBPFleTarget;
Target TheBPFbeTarget;
Target TheBPFTarget;
}

extern "C" void LLVMInitializeBPFTargetInfo() {
  TargetRegistry::RegisterTarget(TheBPFTarget, "bpf", "BPF (host endian)",
                                 [](TargetTuple::ArchType) { return false; },
                                 true);
  RegisterTarget<TargetTuple::bpfel, /*HasJIT=*/true> X(TheBPFleTarget, "bpfel",
                                                        "BPF (little endian)");
  RegisterTarget<TargetTuple::bpfeb, /*HasJIT=*/true> Y(TheBPFbeTarget, "bpfeb",
                                                        "BPF (big endian)");
}
