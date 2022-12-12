#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "Mu"
)

SET(_sources
    HashTable.cpp
    StringHashTable.cpp
    Name.cpp
    Symbol.cpp
    SymbolTable.cpp
    Namespace.cpp
    Node.cpp
    Object.cpp
    Type.cpp
    TypePattern.cpp
    Function.cpp
    PrimitiveType.cpp
    Variable.cpp
    StackVariable.cpp
    GlobalVariable.cpp
    Context.cpp
    MuProcess.cpp
    Thread.cpp
    NodeAssembler.cpp
    NodeVisitor.cpp
    NodePrinter.cpp
    BaseFunctions.cpp
    ReferenceType.cpp
    Module.cpp
    Construct.cpp
    MemberVariable.cpp
    TypeModifier.cpp
    MachineRep.cpp
    MemberFunction.cpp
    Class.cpp
    ClassInstance.cpp
    ParameterVariable.cpp
    Exception.cpp
    NodePatch.cpp
    SymbolicConstant.cpp
    Environment.cpp
    Alias.cpp
    GarbageCollector.cpp
    Signature.cpp
    FunctionType.cpp
    FunctionObject.cpp
    Interface.cpp
    InterfaceImp.cpp
    Archive.cpp
    NilType.cpp
    Language.cpp
    PartialApplicator.cpp
    FunctionSpecializer.cpp
    Unresolved.cpp
    FreeVariable.cpp
    NodeSimplifier.cpp
    TupleType.cpp
    ParameterModifier.cpp
    UTF8.cpp
    TypeVariable.cpp
    VariantType.cpp
    VariantTagType.cpp
    VariantInstance.cpp
    List.cpp
    ListType.cpp
    StructType.cpp
    OpaqueType.cpp
    SymbolType.cpp
    ASTNode.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

# Harcoded special case
SET(CFILES_PTHREAD
    "pthread_support pthread_stop_world pthread_start"
)
SET(CFILES_DARWIN
    "${CFILES_PTHREAD} darwin_stop_world"
)
SET(CFILES_LINUX
    "${CFILES_PTHREAD}"
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC stl_ext
  PRIVATE ${CMAKE_DL_LIBS}
)

IF(RV_TARGET_LINUX)
  SET(THREADS_PREFER_PTHREAD_FLAG
      TRUE
  )
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC Threads::Threads BDWGC::Gc
  )
ELSEIF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC BDWGC::Gc
  )
ELSEIF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC win_pthreads win_posix gc
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
