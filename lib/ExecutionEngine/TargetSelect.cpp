//===-- TargetSelect.cpp - Target Chooser Code ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This just asks the TargetRegistry for the appropriate target to use, and
// allows the user to specify a specific one on the commandline with -march=x,
// -mcpu=y, and -mattr=a,-b,+c. Clients should initialize targets prior to
// calling selectTarget().
//
//===----------------------------------------------------------------------===//

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

TargetMachine *EngineBuilder::selectTarget() {
  TargetTuple TT;

  // MCJIT can generate code for remote targets, but the old JIT and Interpreter
  // must use the host architecture.
  if (WhichEngine != EngineKind::Interpreter && M)
    TT = M->getTargetTuple();

  return selectTarget(TT, MArch, MCPU, MAttrs);
}

/// selectTarget - Pick a target either via -march or by guessing the native
/// arch.  Add any CPU features specified via -mcpu or -mattr.
TargetMachine *
EngineBuilder::selectTarget(const TargetTuple &TT, StringRef MArch,
                            StringRef MCPU,
                            const SmallVectorImpl<std::string> &MAttrs) {
  TargetTuple TheTT(TT);
  if (TheTT.getTargetTriple().str().empty())
    TheTT = TargetTuple(Triple(sys::getProcessTriple()));

  // Adjust the tuple to match what the user requested.
  const Target *TheTarget = nullptr;
  if (!MArch.empty()) {
    auto I = std::find_if(
        TargetRegistry::targets().begin(), TargetRegistry::targets().end(),
        [&](const Target &T) { return MArch == T.getName(); });

    if (I == TargetRegistry::targets().end()) {
      if (ErrorStr)
        *ErrorStr = "No available targets are compatible with this -march, "
                    "see -version for the available targets.\n";
      return nullptr;
    }

    TheTarget = &*I;

    // Adjust the tuple to match (if known), otherwise stick with the
    // requested/host tuple.
    TargetTuple::ArchType Type = TargetTuple::getArchTypeForLLVMName(MArch);
    if (Type != TargetTuple::UnknownArch)
      TheTT.setArch(Type);
  } else {
    std::string Error;
    TheTarget =
        TargetRegistry::lookupTarget(TheTT.getTargetTriple().str(), Error);
    if (!TheTarget) {
      if (ErrorStr)
        *ErrorStr = Error;
      return nullptr;
    }
  }

  // Package up features to be passed to target/subtarget
  std::string FeaturesStr;
  if (!MAttrs.empty()) {
    SubtargetFeatures Features;
    for (unsigned i = 0; i != MAttrs.size(); ++i)
      Features.AddFeature(MAttrs[i]);
    FeaturesStr = Features.getString();
  }

  // FIXME: non-iOS ARM FastISel is broken with MCJIT.
  if (TheTT.getArch() == TargetTuple::arm && !TheTT.isiOS() &&
      OptLevel == CodeGenOpt::None) {
    OptLevel = CodeGenOpt::Less;
  }

  // Allocate a target...
  TargetMachine *Target = TheTarget->createTargetMachine(
      TheTT.getTargetTriple().str(), MCPU, FeaturesStr, Options, RelocModel,
      CMModel, OptLevel);
  assert(Target && "Could not allocate target machine!");
  return Target;
}
