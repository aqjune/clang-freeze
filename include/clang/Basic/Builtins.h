//===--- Builtins.h - Builtin function header -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines enum values for all the target-independent builtin
/// functions.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_BUILTINS_H
#define LLVM_CLANG_BASIC_BUILTINS_H

#include "llvm/ADT/ArrayRef.h"
#include <cstring>

// VC++ defines 'alloca' as an object-like macro, which interferes with our
// builtins.
#undef alloca

namespace clang {
class TargetInfo;
class IdentifierTable;
class ASTContext;
class QualType;
class LangOptions;

enum LanguageID {
  GNU_LANG = 0x1,     // builtin requires GNU mode.
  C_LANG = 0x2,       // builtin for c only.
  CXX_LANG = 0x4,     // builtin for cplusplus only.
  OBJC_LANG = 0x8,    // builtin for objective-c and objective-c++
  MS_LANG = 0x10,     // builtin requires MS mode.
  OCLC_LANG = 0x20,   // builtin for OpenCL C only.
  ALL_LANGUAGES = C_LANG | CXX_LANG | OBJC_LANG, // builtin for all languages.
  ALL_GNU_LANGUAGES = ALL_LANGUAGES | GNU_LANG,  // builtin requires GNU mode.
  ALL_MS_LANGUAGES = ALL_LANGUAGES | MS_LANG     // builtin requires MS mode.
};

namespace Builtin {
enum ID {
  NotBuiltin  = 0,      // This is not a builtin function.
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "clang/Basic/Builtins.def"
  FirstTSBuiltin
};

struct Info {
  const char *Name, *Type, *Attributes, *HeaderName;
  LanguageID Langs;
  const char *Features;
};

/// \brief Holds information about both target-independent and
/// target-specific builtins, allowing easy queries by clients.
///
/// Builtins from an optional auxiliary target are stored in
/// AuxTSRecords. Their IDs are shifted up by TSRecords.size() and need to
/// be translated back with getAuxBuiltinID() before use.
class Context {
  llvm::ArrayRef<Info> TSRecords;
  llvm::ArrayRef<Info> AuxTSRecords;

public:
  Context() {}

  /// \brief Perform target-specific initialization
  /// \param AuxTarget Target info to incorporate builtins from. May be nullptr.
  void InitializeTarget(const TargetInfo &Target, const TargetInfo *AuxTarget);

  /// \brief Mark the identifiers for all the builtins with their
  /// appropriate builtin ID # and mark any non-portable builtin identifiers as
  /// such.
  void initializeBuiltins(IdentifierTable &Table, const LangOptions& LangOpts);

  /// \brief Return the identifier name for the specified builtin,
  /// e.g. "__builtin_abs".
  const char *getName(unsigned ID) const {
    return getRecord(ID).Name;
  }

  /// \brief Get the type descriptor string for the specified builtin.
  const char *getTypeString(unsigned ID) const {
    return getRecord(ID).Type;
  }

  /// \brief Return true if this function is a target-specific builtin.
  bool isTSBuiltin(unsigned ID) const {
    return ID >= Builtin::FirstTSBuiltin;
  }

  /// \brief Return true if this function has no side effects.
  bool isPure(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'U') != nullptr;
  }

  /// \brief Return true if this function has no side effects and doesn't
  /// read memory.
  bool isConst(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'c') != nullptr;
  }

  /// \brief Return true if we know this builtin never throws an exception.
  bool isNoThrow(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'n') != nullptr;
  }

  /// \brief Return true if we know this builtin never returns.
  bool isNoReturn(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'r') != nullptr;
  }

  /// \brief Return true if we know this builtin can return twice.
  bool isReturnsTwice(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'j') != nullptr;
  }

  /// \brief Returns true if this builtin does not perform the side-effects
  /// of its arguments.
  bool isUnevaluated(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'u') != nullptr;
  }

  /// \brief Return true if this is a builtin for a libc/libm function,
  /// with a "__builtin_" prefix (e.g. __builtin_abs).
  bool isLibFunction(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'F') != nullptr;
  }

  /// \brief Determines whether this builtin is a predefined libc/libm
  /// function, such as "malloc", where we know the signature a
  /// priori.
  bool isPredefinedLibFunction(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'f') != nullptr;
  }

  /// \brief Determines whether this builtin is a predefined compiler-rt/libgcc
  /// function, such as "__clear_cache", where we know the signature a
  /// priori.
  bool isPredefinedRuntimeFunction(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'i') != nullptr;
  }

  /// \brief Determines whether this builtin has custom typechecking.
  bool hasCustomTypechecking(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 't') != nullptr;
  }

  /// \brief Determines whether this builtin has a result or any arguments which
  /// are pointer types.
  bool hasPtrArgsOrResult(unsigned ID) const {
    return strchr(getRecord(ID).Type, '*') != nullptr;
  }

  /// \brief Completely forget that the given ID was ever considered a builtin,
  /// e.g., because the user provided a conflicting signature.
  void forgetBuiltin(unsigned ID, IdentifierTable &Table);

  /// \brief If this is a library function that comes from a specific
  /// header, retrieve that header name.
  const char *getHeaderName(unsigned ID) const {
    return getRecord(ID).HeaderName;
  }

  /// \brief Determine whether this builtin is like printf in its
  /// formatting rules and, if so, set the index to the format string
  /// argument and whether this function as a va_list argument.
  bool isPrintfLike(unsigned ID, unsigned &FormatIdx, bool &HasVAListArg);

  /// \brief Determine whether this builtin is like scanf in its
  /// formatting rules and, if so, set the index to the format string
  /// argument and whether this function as a va_list argument.
  bool isScanfLike(unsigned ID, unsigned &FormatIdx, bool &HasVAListArg);

  /// \brief Return true if this function has no side effects and doesn't
  /// read memory, except for possibly errno.
  ///
  /// Such functions can be const when the MathErrno lang option is disabled.
  bool isConstWithoutErrno(unsigned ID) const {
    return strchr(getRecord(ID).Attributes, 'e') != nullptr;
  }

  const char *getRequiredFeatures(unsigned ID) const {
    return getRecord(ID).Features;
  }

  /// \brief Return true if builtin ID belongs to AuxTarget.
  bool isAuxBuiltinID(unsigned ID) const {
    return ID >= (Builtin::FirstTSBuiltin + TSRecords.size());
  }

  /// Return real buitin ID (i.e. ID it would have furing compilation
  /// for AuxTarget).
  unsigned getAuxBuiltinID(unsigned ID) const { return ID - TSRecords.size(); }

  /// Returns true if this is a libc/libm function without the '__builtin_'
  /// prefix.
  static bool isBuiltinFunc(const char *Name);

private:
  const Info &getRecord(unsigned ID) const;

  /// \brief Is this builtin supported according to the given language options?
  bool builtinIsSupported(const Builtin::Info &BuiltinInfo,
                          const LangOptions &LangOpts);

  /// \brief Helper function for isPrintfLike and isScanfLike.
  bool isLike(unsigned ID, unsigned &FormatIdx, bool &HasVAListArg,
              const char *Fmt) const;
};

}

/// \brief Kinds of BuiltinTemplateDecl.
enum BuiltinTemplateKind : int {
  /// \brief This names the __make_integer_seq BuiltinTemplateDecl.
  BTK__make_integer_seq
};

} // end namespace clang
#endif
