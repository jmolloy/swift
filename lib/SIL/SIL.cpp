//===--- SILFunction.cpp - Defines the SILFunction data structure ---------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/SIL/Function.h"
#include "swift/SIL/SILBuilder.h"
#include "swift/SIL/SILModule.h"
#include "swift/SIL/SILConstant.h"
#include "swift/AST/ASTContext.h"
#include "swift/AST/Decl.h"
#include "swift/AST/Expr.h"
#include "swift/AST/Pattern.h"
#include "swift/AST/Types.h"
using namespace swift;

Function::~Function() {
}

static SILType toplevelFunctionType(ASTContext &C) {
  auto t = FunctionType::get(TupleType::getEmpty(C),
                             TupleType::getEmpty(C),
                             C);
  return SILType::getPreLoweredType(t);
}

SILModule::SILModule(ASTContext &Context, bool hasTopLevel) :
  Context(Context), toplevel(nullptr) {
    
  if (hasTopLevel)
    toplevel = new (*this) Function(*this, toplevelFunctionType(Context));
}

SILModule::~SILModule() {
}

SILConstant::SILConstant(SILConstant::Loc baseLoc) {
  if (ValueDecl *vd = baseLoc.dyn_cast<ValueDecl*>()) {
    // Explicit getters and setters have independent decls, but we also need
    // to reference implicit getters and setters. For consistency, generate
    // getter and setter constants as references to the parent decl.
    if (FuncDecl *fd = dyn_cast<FuncDecl>(vd)) {
      if (fd->isGetterOrSetter()) {
        if (Decl *getterFor = fd->getGetterDecl()) {
          loc = cast<ValueDecl>(getterFor);
          id = Getter;
        } else if (Decl *setterFor = fd->getSetterDecl()) {
          loc = cast<ValueDecl>(setterFor);
          id = Setter;
        } else {
          llvm_unreachable("no getter or setter decl?!");
        }
        return;
      }
    }
  }
  
  loc = baseLoc;
  id = 0;
}

SILType SILType::getObjectPointerType(ASTContext &C) {
  return SILType(C.TheObjectPointerType);
}

SILType SILType::getRawPointerType(ASTContext &C) {
  return SILType(C.TheRawPointerType);
}

SILType SILType::getEmptyTupleType(ASTContext &C) {
  return SILType(TupleType::getEmpty(C));
}

SILType SILType::getBuiltinIntegerType(unsigned bitWidth, ASTContext &C) {
  return SILType(BuiltinIntegerType::get(bitWidth, C));
}

SILType SILType::getAddressType(ASTContext &C) const {
  if ((*this)->is<LValueType>())
    return *this;
  else
    return SILType(LValueType::get(*this, LValueType::Qual::DefaultForType, C));
}

TupleInst *SILBuilder::createEmptyTuple(SILLocation Loc) {
  return createTuple(Loc,
                     SILType::getPreLoweredType(
                       TupleType::getEmpty(F.getContext())),
                     ArrayRef<Value>());
}