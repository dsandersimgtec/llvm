//===-- TargetLibraryInfo.cpp - Runtime library information ----------------==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the TargetLibraryInfo class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

static cl::opt<TargetLibraryInfoImpl::VectorLibrary> ClVectorLibrary(
    "vector-library", cl::Hidden, cl::desc("Vector functions library"),
    cl::init(TargetLibraryInfoImpl::NoLibrary),
    cl::values(clEnumValN(TargetLibraryInfoImpl::NoLibrary, "none",
                          "No vector functions library"),
               clEnumValN(TargetLibraryInfoImpl::Accelerate, "Accelerate",
                          "Accelerate framework"),
               clEnumValEnd));

const char *const TargetLibraryInfoImpl::StandardNames[LibFunc::NumLibFuncs] = {
#define TLI_DEFINE_STRING
#include "llvm/Analysis/TargetLibraryInfo.def"
};

static bool hasSinCosPiStret(const TargetTuple &TT) {
  // Only Darwin variants have _stret versions of combined trig functions.
  if (!TT.isOSDarwin())
    return false;

  // The ABI is rather complicated on x86, so don't do anything special there.
  if (TT.getArch() == TargetTuple::x86)
    return false;

  if (TT.isMacOSX() && TT.isMacOSXVersionLT(10, 9))
    return false;

  if (TT.isiOS() && TT.isOSVersionLT(7, 0))
    return false;

  return true;
}

/// initialize - Initialize the set of available library functions based on the
/// specified target triple.  This should be carefully written so that a missing
/// target triple gets a sane set of defaults.
static void initialize(TargetLibraryInfoImpl &TLI, const TargetTuple &TT,
                       const char *const *StandardNames) {
#ifndef NDEBUG
  // Verify that the StandardNames array is in alphabetical order.
  for (unsigned F = 1; F < LibFunc::NumLibFuncs; ++F) {
    if (strcmp(StandardNames[F-1], StandardNames[F]) >= 0)
      llvm_unreachable("TargetLibraryInfoImpl function names must be sorted");
  }
#endif // !NDEBUG

  // There are no library implementations of mempcy and memset for AMD gpus and
  // these can be difficult to lower in the backend.
  if (TT.getArch() == TargetTuple::r600 ||
      TT.getArch() == TargetTuple::amdgcn) {
    TLI.setUnavailable(LibFunc::memcpy);
    TLI.setUnavailable(LibFunc::memset);
    TLI.setUnavailable(LibFunc::memset_pattern16);
    return;
  }

  // memset_pattern16 is only available on iOS 3.0 and Mac OS X 10.5 and later.
  if (TT.isMacOSX()) {
    if (TT.isMacOSXVersionLT(10, 5))
      TLI.setUnavailable(LibFunc::memset_pattern16);
  } else if (TT.isiOS()) {
    if (TT.isOSVersionLT(3, 0))
      TLI.setUnavailable(LibFunc::memset_pattern16);
  } else {
    TLI.setUnavailable(LibFunc::memset_pattern16);
  }

  if (!hasSinCosPiStret(TT)) {
    TLI.setUnavailable(LibFunc::sinpi);
    TLI.setUnavailable(LibFunc::sinpif);
    TLI.setUnavailable(LibFunc::cospi);
    TLI.setUnavailable(LibFunc::cospif);
    TLI.setUnavailable(LibFunc::sincospi_stret);
    TLI.setUnavailable(LibFunc::sincospif_stret);
  }

  if (TT.isMacOSX() && TT.getArch() == TargetTuple::x86 &&
      !TT.isMacOSXVersionLT(10, 7)) {
    // x86-32 OSX has a scheme where fwrite and fputs (and some other functions
    // we don't care about) have two versions; on recent OSX, the one we want
    // has a $UNIX2003 suffix. The two implementations are identical except
    // for the return value in some edge cases.  However, we don't want to
    // generate code that depends on the old symbols.
    TLI.setAvailableWithName(LibFunc::fwrite, "fwrite$UNIX2003");
    TLI.setAvailableWithName(LibFunc::fputs, "fputs$UNIX2003");
  }

  // iprintf and friends are only available on XCore and TCE.
  if (TT.getArch() != TargetTuple::xcore && TT.getArch() != TargetTuple::tce) {
    TLI.setUnavailable(LibFunc::iprintf);
    TLI.setUnavailable(LibFunc::siprintf);
    TLI.setUnavailable(LibFunc::fiprintf);
  }

  if (TT.isOSWindows() && !TT.isOSCygMing()) {
    // Win32 does not support long double
    TLI.setUnavailable(LibFunc::acosl);
    TLI.setUnavailable(LibFunc::asinl);
    TLI.setUnavailable(LibFunc::atanl);
    TLI.setUnavailable(LibFunc::atan2l);
    TLI.setUnavailable(LibFunc::ceill);
    TLI.setUnavailable(LibFunc::copysignl);
    TLI.setUnavailable(LibFunc::cosl);
    TLI.setUnavailable(LibFunc::coshl);
    TLI.setUnavailable(LibFunc::expl);
    TLI.setUnavailable(LibFunc::fabsf); // Win32 and Win64 both lack fabsf
    TLI.setUnavailable(LibFunc::fabsl);
    TLI.setUnavailable(LibFunc::floorl);
    TLI.setUnavailable(LibFunc::fmaxl);
    TLI.setUnavailable(LibFunc::fminl);
    TLI.setUnavailable(LibFunc::fmodl);
    TLI.setUnavailable(LibFunc::frexpl);
    TLI.setUnavailable(LibFunc::ldexpf);
    TLI.setUnavailable(LibFunc::ldexpl);
    TLI.setUnavailable(LibFunc::logl);
    TLI.setUnavailable(LibFunc::modfl);
    TLI.setUnavailable(LibFunc::powl);
    TLI.setUnavailable(LibFunc::sinl);
    TLI.setUnavailable(LibFunc::sinhl);
    TLI.setUnavailable(LibFunc::sqrtl);
    TLI.setUnavailable(LibFunc::tanl);
    TLI.setUnavailable(LibFunc::tanhl);

    // Win32 only has C89 math
    TLI.setUnavailable(LibFunc::acosh);
    TLI.setUnavailable(LibFunc::acoshf);
    TLI.setUnavailable(LibFunc::acoshl);
    TLI.setUnavailable(LibFunc::asinh);
    TLI.setUnavailable(LibFunc::asinhf);
    TLI.setUnavailable(LibFunc::asinhl);
    TLI.setUnavailable(LibFunc::atanh);
    TLI.setUnavailable(LibFunc::atanhf);
    TLI.setUnavailable(LibFunc::atanhl);
    TLI.setUnavailable(LibFunc::cbrt);
    TLI.setUnavailable(LibFunc::cbrtf);
    TLI.setUnavailable(LibFunc::cbrtl);
    TLI.setUnavailable(LibFunc::exp2);
    TLI.setUnavailable(LibFunc::exp2f);
    TLI.setUnavailable(LibFunc::exp2l);
    TLI.setUnavailable(LibFunc::expm1);
    TLI.setUnavailable(LibFunc::expm1f);
    TLI.setUnavailable(LibFunc::expm1l);
    TLI.setUnavailable(LibFunc::log2);
    TLI.setUnavailable(LibFunc::log2f);
    TLI.setUnavailable(LibFunc::log2l);
    TLI.setUnavailable(LibFunc::log1p);
    TLI.setUnavailable(LibFunc::log1pf);
    TLI.setUnavailable(LibFunc::log1pl);
    TLI.setUnavailable(LibFunc::logb);
    TLI.setUnavailable(LibFunc::logbf);
    TLI.setUnavailable(LibFunc::logbl);
    TLI.setUnavailable(LibFunc::nearbyint);
    TLI.setUnavailable(LibFunc::nearbyintf);
    TLI.setUnavailable(LibFunc::nearbyintl);
    TLI.setUnavailable(LibFunc::rint);
    TLI.setUnavailable(LibFunc::rintf);
    TLI.setUnavailable(LibFunc::rintl);
    TLI.setUnavailable(LibFunc::round);
    TLI.setUnavailable(LibFunc::roundf);
    TLI.setUnavailable(LibFunc::roundl);
    TLI.setUnavailable(LibFunc::trunc);
    TLI.setUnavailable(LibFunc::truncf);
    TLI.setUnavailable(LibFunc::truncl);

    // Win32 provides some C99 math with mangled names
    TLI.setAvailableWithName(LibFunc::copysign, "_copysign");

    if (TT.getArch() == TargetTuple::x86) {
      // Win32 on x86 implements single-precision math functions as macros
      TLI.setUnavailable(LibFunc::acosf);
      TLI.setUnavailable(LibFunc::asinf);
      TLI.setUnavailable(LibFunc::atanf);
      TLI.setUnavailable(LibFunc::atan2f);
      TLI.setUnavailable(LibFunc::ceilf);
      TLI.setUnavailable(LibFunc::copysignf);
      TLI.setUnavailable(LibFunc::cosf);
      TLI.setUnavailable(LibFunc::coshf);
      TLI.setUnavailable(LibFunc::expf);
      TLI.setUnavailable(LibFunc::floorf);
      TLI.setUnavailable(LibFunc::fminf);
      TLI.setUnavailable(LibFunc::fmaxf);
      TLI.setUnavailable(LibFunc::fmodf);
      TLI.setUnavailable(LibFunc::logf);
      TLI.setUnavailable(LibFunc::powf);
      TLI.setUnavailable(LibFunc::sinf);
      TLI.setUnavailable(LibFunc::sinhf);
      TLI.setUnavailable(LibFunc::sqrtf);
      TLI.setUnavailable(LibFunc::tanf);
      TLI.setUnavailable(LibFunc::tanhf);
    }

    // Win32 does *not* provide provide these functions, but they are
    // generally available on POSIX-compliant systems:
    TLI.setUnavailable(LibFunc::access);
    TLI.setUnavailable(LibFunc::bcmp);
    TLI.setUnavailable(LibFunc::bcopy);
    TLI.setUnavailable(LibFunc::bzero);
    TLI.setUnavailable(LibFunc::chmod);
    TLI.setUnavailable(LibFunc::chown);
    TLI.setUnavailable(LibFunc::closedir);
    TLI.setUnavailable(LibFunc::ctermid);
    TLI.setUnavailable(LibFunc::fdopen);
    TLI.setUnavailable(LibFunc::ffs);
    TLI.setUnavailable(LibFunc::fileno);
    TLI.setUnavailable(LibFunc::flockfile);
    TLI.setUnavailable(LibFunc::fseeko);
    TLI.setUnavailable(LibFunc::fstat);
    TLI.setUnavailable(LibFunc::fstatvfs);
    TLI.setUnavailable(LibFunc::ftello);
    TLI.setUnavailable(LibFunc::ftrylockfile);
    TLI.setUnavailable(LibFunc::funlockfile);
    TLI.setUnavailable(LibFunc::getc_unlocked);
    TLI.setUnavailable(LibFunc::getitimer);
    TLI.setUnavailable(LibFunc::getlogin_r);
    TLI.setUnavailable(LibFunc::getpwnam);
    TLI.setUnavailable(LibFunc::gettimeofday);
    TLI.setUnavailable(LibFunc::htonl);
    TLI.setUnavailable(LibFunc::htons);
    TLI.setUnavailable(LibFunc::lchown);
    TLI.setUnavailable(LibFunc::lstat);
    TLI.setUnavailable(LibFunc::memccpy);
    TLI.setUnavailable(LibFunc::mkdir);
    TLI.setUnavailable(LibFunc::ntohl);
    TLI.setUnavailable(LibFunc::ntohs);
    TLI.setUnavailable(LibFunc::open);
    TLI.setUnavailable(LibFunc::opendir);
    TLI.setUnavailable(LibFunc::pclose);
    TLI.setUnavailable(LibFunc::popen);
    TLI.setUnavailable(LibFunc::pread);
    TLI.setUnavailable(LibFunc::pwrite);
    TLI.setUnavailable(LibFunc::read);
    TLI.setUnavailable(LibFunc::readlink);
    TLI.setUnavailable(LibFunc::realpath);
    TLI.setUnavailable(LibFunc::rmdir);
    TLI.setUnavailable(LibFunc::setitimer);
    TLI.setUnavailable(LibFunc::stat);
    TLI.setUnavailable(LibFunc::statvfs);
    TLI.setUnavailable(LibFunc::stpcpy);
    TLI.setUnavailable(LibFunc::stpncpy);
    TLI.setUnavailable(LibFunc::strcasecmp);
    TLI.setUnavailable(LibFunc::strncasecmp);
    TLI.setUnavailable(LibFunc::times);
    TLI.setUnavailable(LibFunc::uname);
    TLI.setUnavailable(LibFunc::unlink);
    TLI.setUnavailable(LibFunc::unsetenv);
    TLI.setUnavailable(LibFunc::utime);
    TLI.setUnavailable(LibFunc::utimes);
    TLI.setUnavailable(LibFunc::write);

    // Win32 does *not* provide provide these functions, but they are
    // specified by C99:
    TLI.setUnavailable(LibFunc::atoll);
    TLI.setUnavailable(LibFunc::frexpf);
    TLI.setUnavailable(LibFunc::llabs);
  }

  switch (TT.getOS()) {
  case TargetTuple::MacOSX:
    // exp10 and exp10f are not available on OS X until 10.9 and iOS until 7.0
    // and their names are __exp10 and __exp10f. exp10l is not available on
    // OS X or iOS.
    TLI.setUnavailable(LibFunc::exp10l);
    if (TT.isMacOSXVersionLT(10, 9)) {
      TLI.setUnavailable(LibFunc::exp10);
      TLI.setUnavailable(LibFunc::exp10f);
    } else {
      TLI.setAvailableWithName(LibFunc::exp10, "__exp10");
      TLI.setAvailableWithName(LibFunc::exp10f, "__exp10f");
    }
    break;
  case TargetTuple::IOS:
    TLI.setUnavailable(LibFunc::exp10l);
    if (TT.isOSVersionLT(7, 0)) {
      TLI.setUnavailable(LibFunc::exp10);
      TLI.setUnavailable(LibFunc::exp10f);
    } else {
      TLI.setAvailableWithName(LibFunc::exp10, "__exp10");
      TLI.setAvailableWithName(LibFunc::exp10f, "__exp10f");
    }
    break;
  case TargetTuple::Linux:
    // exp10, exp10f, exp10l is available on Linux (GLIBC) but are extremely
    // buggy prior to glibc version 2.18. Until this version is widely deployed
    // or we have a reasonable detection strategy, we cannot use exp10 reliably
    // on Linux.
    //
    // Fall through to disable all of them.
  default:
    TLI.setUnavailable(LibFunc::exp10);
    TLI.setUnavailable(LibFunc::exp10f);
    TLI.setUnavailable(LibFunc::exp10l);
  }

  // ffsl is available on at least Darwin, Mac OS X, iOS, FreeBSD, and
  // Linux (GLIBC):
  // http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/ffsl.3.html
  // http://svn.freebsd.org/base/user/eri/pf45/head/lib/libc/string/ffsl.c
  // http://www.gnu.org/software/gnulib/manual/html_node/ffsl.html
  switch (TT.getOS()) {
  case TargetTuple::Darwin:
  case TargetTuple::MacOSX:
  case TargetTuple::IOS:
  case TargetTuple::FreeBSD:
  case TargetTuple::Linux:
    break;
  default:
    TLI.setUnavailable(LibFunc::ffsl);
  }

  // ffsll is available on at least FreeBSD and Linux (GLIBC):
  // http://svn.freebsd.org/base/user/eri/pf45/head/lib/libc/string/ffsll.c
  // http://www.gnu.org/software/gnulib/manual/html_node/ffsll.html
  switch (TT.getOS()) {
  case TargetTuple::FreeBSD:
  case TargetTuple::Linux:
    break;
  default:
    TLI.setUnavailable(LibFunc::ffsll);
  }

  // The following functions are available on at least Linux:
  if (!TT.isOSLinux()) {
    TLI.setUnavailable(LibFunc::dunder_strdup);
    TLI.setUnavailable(LibFunc::dunder_strtok_r);
    TLI.setUnavailable(LibFunc::dunder_isoc99_scanf);
    TLI.setUnavailable(LibFunc::dunder_isoc99_sscanf);
    TLI.setUnavailable(LibFunc::under_IO_getc);
    TLI.setUnavailable(LibFunc::under_IO_putc);
    TLI.setUnavailable(LibFunc::memalign);
    TLI.setUnavailable(LibFunc::fopen64);
    TLI.setUnavailable(LibFunc::fseeko64);
    TLI.setUnavailable(LibFunc::fstat64);
    TLI.setUnavailable(LibFunc::fstatvfs64);
    TLI.setUnavailable(LibFunc::ftello64);
    TLI.setUnavailable(LibFunc::lstat64);
    TLI.setUnavailable(LibFunc::open64);
    TLI.setUnavailable(LibFunc::stat64);
    TLI.setUnavailable(LibFunc::statvfs64);
    TLI.setUnavailable(LibFunc::tmpfile64);
  }

  TLI.addVectorizableFunctionsFromVecLib(ClVectorLibrary);
}

TargetLibraryInfoImpl::TargetLibraryInfoImpl() {
  // Default to everything being available.
  memset(AvailableArray, -1, sizeof(AvailableArray));

  initialize(*this, TargetTuple(), StandardNames);
}

TargetLibraryInfoImpl::TargetLibraryInfoImpl(const TargetTuple &TT) {
  // Default to everything being available.
  memset(AvailableArray, -1, sizeof(AvailableArray));

  initialize(*this, TT, StandardNames);
}

TargetLibraryInfoImpl::TargetLibraryInfoImpl(const TargetLibraryInfoImpl &TLI)
    : CustomNames(TLI.CustomNames) {
  memcpy(AvailableArray, TLI.AvailableArray, sizeof(AvailableArray));
  VectorDescs = TLI.VectorDescs;
  ScalarDescs = TLI.ScalarDescs;
}

TargetLibraryInfoImpl::TargetLibraryInfoImpl(TargetLibraryInfoImpl &&TLI)
    : CustomNames(std::move(TLI.CustomNames)) {
  std::move(std::begin(TLI.AvailableArray), std::end(TLI.AvailableArray),
            AvailableArray);
  VectorDescs = TLI.VectorDescs;
  ScalarDescs = TLI.ScalarDescs;
}

TargetLibraryInfoImpl &TargetLibraryInfoImpl::operator=(const TargetLibraryInfoImpl &TLI) {
  CustomNames = TLI.CustomNames;
  memcpy(AvailableArray, TLI.AvailableArray, sizeof(AvailableArray));
  return *this;
}

TargetLibraryInfoImpl &TargetLibraryInfoImpl::operator=(TargetLibraryInfoImpl &&TLI) {
  CustomNames = std::move(TLI.CustomNames);
  std::move(std::begin(TLI.AvailableArray), std::end(TLI.AvailableArray),
            AvailableArray);
  return *this;
}

static StringRef sanitizeFunctionName(StringRef funcName) {
  // Filter out empty names and names containing null bytes, those can't be in
  // our table.
  if (funcName.empty() || funcName.find('\0') != StringRef::npos)
    return StringRef();

  // Check for \01 prefix that is used to mangle __asm declarations and
  // strip it if present.
  return GlobalValue::getRealLinkageName(funcName);
}

bool TargetLibraryInfoImpl::getLibFunc(StringRef funcName,
                                   LibFunc::Func &F) const {
  const char *const *Start = &StandardNames[0];
  const char *const *End = &StandardNames[LibFunc::NumLibFuncs];

  funcName = sanitizeFunctionName(funcName);
  if (funcName.empty())
    return false;

  const char *const *I = std::lower_bound(
      Start, End, funcName, [](const char *LHS, StringRef RHS) {
        return std::strncmp(LHS, RHS.data(), RHS.size()) < 0;
      });
  if (I != End && *I == funcName) {
    F = (LibFunc::Func)(I - Start);
    return true;
  }
  return false;
}

void TargetLibraryInfoImpl::disableAllFunctions() {
  memset(AvailableArray, 0, sizeof(AvailableArray));
}

static bool compareByScalarFnName(const VecDesc &LHS, const VecDesc &RHS) {
  return std::strncmp(LHS.ScalarFnName, RHS.ScalarFnName,
                      std::strlen(RHS.ScalarFnName)) < 0;
}

static bool compareByVectorFnName(const VecDesc &LHS, const VecDesc &RHS) {
  return std::strncmp(LHS.VectorFnName, RHS.VectorFnName,
                      std::strlen(RHS.VectorFnName)) < 0;
}

static bool compareWithScalarFnName(const VecDesc &LHS, StringRef S) {
  return std::strncmp(LHS.ScalarFnName, S.data(), S.size()) < 0;
}

static bool compareWithVectorFnName(const VecDesc &LHS, StringRef S) {
  return std::strncmp(LHS.VectorFnName, S.data(), S.size()) < 0;
}

void TargetLibraryInfoImpl::addVectorizableFunctions(ArrayRef<VecDesc> Fns) {
  VectorDescs.insert(VectorDescs.end(), Fns.begin(), Fns.end());
  std::sort(VectorDescs.begin(), VectorDescs.end(), compareByScalarFnName);

  ScalarDescs.insert(ScalarDescs.end(), Fns.begin(), Fns.end());
  std::sort(ScalarDescs.begin(), ScalarDescs.end(), compareByVectorFnName);
}

void TargetLibraryInfoImpl::addVectorizableFunctionsFromVecLib(
    enum VectorLibrary VecLib) {
  switch (VecLib) {
  case Accelerate: {
    const VecDesc VecFuncs[] = {
        // Floating-Point Arithmetic and Auxiliary Functions
        {"ceilf", "vceilf", 4},
        {"fabsf", "vfabsf", 4},
        {"llvm.fabs.f32", "vfabsf", 4},
        {"floorf", "vfloorf", 4},
        {"sqrtf", "vsqrtf", 4},
        {"llvm.sqrt.f32", "vsqrtf", 4},

        // Exponential and Logarithmic Functions
        {"expf", "vexpf", 4},
        {"llvm.exp.f32", "vexpf", 4},
        {"expm1f", "vexpm1f", 4},
        {"logf", "vlogf", 4},
        {"llvm.log.f32", "vlogf", 4},
        {"log1pf", "vlog1pf", 4},
        {"log10f", "vlog10f", 4},
        {"llvm.log10.f32", "vlog10f", 4},
        {"logbf", "vlogbf", 4},

        // Trigonometric Functions
        {"sinf", "vsinf", 4},
        {"llvm.sin.f32", "vsinf", 4},
        {"cosf", "vcosf", 4},
        {"llvm.cos.f32", "vcosf", 4},
        {"tanf", "vtanf", 4},
        {"asinf", "vasinf", 4},
        {"acosf", "vacosf", 4},
        {"atanf", "vatanf", 4},

        // Hyperbolic Functions
        {"sinhf", "vsinhf", 4},
        {"coshf", "vcoshf", 4},
        {"tanhf", "vtanhf", 4},
        {"asinhf", "vasinhf", 4},
        {"acoshf", "vacoshf", 4},
        {"atanhf", "vatanhf", 4},
    };
    addVectorizableFunctions(VecFuncs);
    break;
  }
  case NoLibrary:
    break;
  }
}

bool TargetLibraryInfoImpl::isFunctionVectorizable(StringRef funcName) const {
  funcName = sanitizeFunctionName(funcName);
  if (funcName.empty())
    return false;

  std::vector<VecDesc>::const_iterator I = std::lower_bound(
      VectorDescs.begin(), VectorDescs.end(), funcName,
      compareWithScalarFnName);
  return I != VectorDescs.end() && StringRef(I->ScalarFnName) == funcName;
}

StringRef TargetLibraryInfoImpl::getVectorizedFunction(StringRef F,
                                                       unsigned VF) const {
  F = sanitizeFunctionName(F);
  if (F.empty())
    return F;
  std::vector<VecDesc>::const_iterator I = std::lower_bound(
      VectorDescs.begin(), VectorDescs.end(), F, compareWithScalarFnName);
  while (I != VectorDescs.end() && StringRef(I->ScalarFnName) == F) {
    if (I->VectorizationFactor == VF)
      return I->VectorFnName;
    ++I;
  }
  return StringRef();
}

StringRef TargetLibraryInfoImpl::getScalarizedFunction(StringRef F,
                                                       unsigned &VF) const {
  F = sanitizeFunctionName(F);
  if (F.empty())
    return F;

  std::vector<VecDesc>::const_iterator I = std::lower_bound(
      ScalarDescs.begin(), ScalarDescs.end(), F, compareWithVectorFnName);
  if (I == VectorDescs.end() || StringRef(I->VectorFnName) != F)
    return StringRef();
  VF = I->VectorizationFactor;
  return I->ScalarFnName;
}

TargetLibraryInfo TargetLibraryAnalysis::run(Module &M) {
  if (PresetInfoImpl)
    return TargetLibraryInfo(*PresetInfoImpl);

  return TargetLibraryInfo(lookupInfoImpl(M.getTargetTuple()));
}

TargetLibraryInfo TargetLibraryAnalysis::run(Function &F) {
  if (PresetInfoImpl)
    return TargetLibraryInfo(*PresetInfoImpl);

  return TargetLibraryInfo(lookupInfoImpl(F.getParent()->getTargetTuple()));
}

TargetLibraryInfoImpl &TargetLibraryAnalysis::lookupInfoImpl(TargetTuple TT) {
  std::unique_ptr<TargetLibraryInfoImpl> &Impl =
      Impls[TT.getTargetTriple().normalize()];
  if (!Impl)
    Impl.reset(new TargetLibraryInfoImpl(TargetTuple(TT)));

  return *Impl;
}


TargetLibraryInfoWrapperPass::TargetLibraryInfoWrapperPass()
    : ImmutablePass(ID), TLIImpl(), TLI(TLIImpl) {
  initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
}

TargetLibraryInfoWrapperPass::TargetLibraryInfoWrapperPass(
    const TargetTuple &TT)
    : ImmutablePass(ID), TLIImpl(TT), TLI(TLIImpl) {
  initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
}

TargetLibraryInfoWrapperPass::TargetLibraryInfoWrapperPass(
    const TargetLibraryInfoImpl &TLIImpl)
    : ImmutablePass(ID), TLIImpl(TLIImpl), TLI(this->TLIImpl) {
  initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
}

char TargetLibraryAnalysis::PassID;

// Register the basic pass.
INITIALIZE_PASS(TargetLibraryInfoWrapperPass, "targetlibinfo",
                "Target Library Information", false, true)
char TargetLibraryInfoWrapperPass::ID = 0;

void TargetLibraryInfoWrapperPass::anchor() {}
