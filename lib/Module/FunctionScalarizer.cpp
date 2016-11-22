#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/InstVisitor.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

#define DEBUG_TYPE "functionScalarizer"

using namespace llvm;
using namespace std;

namespace llvm {
ModulePass *createFunctionScalarizerPass();
void initializeFunctionScalarizerPass(PassRegistry &);
} // namespace llvm

namespace {
class FunctionScalarizer : public ModulePass {
public:
  static char ID;

  FunctionScalarizer() : ModulePass(ID) {
    initializeFunctionScalarizerPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnModule(Module &M);
};

char FunctionScalarizer::ID = 0;
} // anonymous namespace

INITIALIZE_PASS(FunctionScalarizer, "functionScalarizer",
                "Scalarize function calls", false, false)

ModulePass *llvm::createFunctionScalarizerPass() {
  return new FunctionScalarizer();
}

bool FunctionScalarizer::runOnModule(Module &M) {
  for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
    Function *oldFunction = FI;

    bool changedReturn = false;

    Type *oldReturnType = oldFunction->getReturnType();
    Type *newReturnType;

    if (VectorType *oldVec = dyn_cast<VectorType>(oldReturnType)) {
      vector<Type *> elements;
      for (size_t i = 0; i < oldVec->getNumElements(); ++i) {
        elements.push_back(oldVec->getElementType());
      }

      // The new return type is a struct that contains the vector's elements
      newReturnType =
          StructType::get(M.getContext(), ArrayRef<Type *>(elements), false);

      changedReturn = true;
    } else {
      newReturnType = oldReturnType;
    }

    bool changedArgs = false;

    vector<Type *> oldArgumentTypes;
    vector<Type *> newArgumentTypes;

    for (size_t i = 0; i < oldFunction->getFunctionType()->getNumParams();
         ++i) {
      Type *oldArg = oldFunction->getFunctionType()->getParamType(i);

      oldArgumentTypes.push_back(oldArg);

      if (VectorType *oldVec = dyn_cast<VectorType>(oldArg)) {
        vector<Type *> elements;
        for (size_t i = 0; i < oldVec->getNumElements(); ++i) {
          elements.push_back(oldVec->getElementType());
        }
        newArgumentTypes.push_back(
            StructType::get(M.getContext(), ArrayRef<Type *>(elements), false));

        changedArgs = true;
      } else {
        newArgumentTypes.push_back(oldArg);
      }
    }

    if (changedReturn || changedArgs) {
      FunctionType *newType =
          FunctionType::get(newReturnType, ArrayRef<Type *>(newArgumentTypes),
                            oldFunction->getFunctionType()->isVarArg());
      Function *newFunction =
          Function::Create(newType, Function::InternalLinkage,
                           Twine(oldFunction->getName()) + Twine("_clone"), &M);

      vector<Instruction *> AllocaInsts, CastInsts, StoreInsts, LoadInsts;

      // Map the old arguments to the new. If the argument was a vector, cast it
      // from the changed struct
      ValueToValueMapTy VMap;
      {
        size_t argIndex = 0;
        Function::arg_iterator oldArg = oldFunction->arg_begin();
        Function::arg_iterator newArg = newFunction->arg_begin();
        for (; argIndex < oldFunction->arg_size();
             ++argIndex, ++oldArg, ++newArg) {
          assert(argIndex < newFunction->arg_size());

          // If the old argument has vector type, create a new vector, save the
          // new struct argument at its address and map the new vector to the
          // old argument.
          // This seems unsafe, but llvm does the same when vectorizing, so it
          // should work.
          if (oldArgumentTypes[argIndex]->isVectorTy()) {
            Instruction *Alloca = new AllocaInst(
                oldArgumentTypes[argIndex],
                Twine("arg_") + Twine((unsigned)argIndex) + Twine("clone"));
            AllocaInsts.push_back(Alloca);

            Instruction *Cast = new BitCastInst(
                Alloca, newArgumentTypes[argIndex]->getPointerTo());
            CastInsts.push_back(Cast);

            Instruction *Store = new StoreInst(newArg, Cast);
            StoreInsts.push_back(Store);

            Instruction *Load = new LoadInst(Alloca);
            LoadInsts.push_back(Load);

            VMap[oldArg] = Load;

            // Now add the same cast in reverse before each call or invoke
            // instruction
            for (Function::use_iterator UI = oldFunction->use_begin(),
                                        UE = oldFunction->use_end();
                 UI != UE; ++UI) {
              if (CallInst *oldCall = dyn_cast<CallInst>(*UI)) {
                Instruction *CallAlloca = new AllocaInst(
                    newArgumentTypes[argIndex], Twine(""), oldCall);
                Instruction *CallCast = new BitCastInst(
                    CallAlloca, oldArgumentTypes[argIndex]->getPointerTo(),
                    Twine(""), oldCall);
                new StoreInst(oldCall->getArgOperand(argIndex), CallCast,
                              oldCall);
                Instruction *Load =
                    new LoadInst(CallAlloca, Twine(""), oldCall);
                oldCall->setArgOperand(argIndex, Load);
              }
              if (InvokeInst *oldInvoke = dyn_cast<InvokeInst>(*UI)) {
                Instruction *CallAlloca = new AllocaInst(
                    newArgumentTypes[argIndex], Twine(""), oldInvoke);
                Instruction *CallCast = new BitCastInst(
                    CallAlloca, oldArgumentTypes[argIndex]->getPointerTo(),
                    Twine(""), oldInvoke);
                new StoreInst(oldInvoke->getArgOperand(argIndex), CallCast,
                              oldInvoke);
                Instruction *Load =
                    new LoadInst(CallAlloca, Twine(""), oldInvoke);
                oldInvoke->setArgOperand(argIndex, Load);
              }
            }
          } else {
            VMap[oldArg] = newArg;
          }
        }
      }

      typedef SmallVector<ReturnInst *, 10> ReturnsVector;
      ReturnsVector Returns;

      CloneFunctionInto(newFunction, oldFunction, VMap, false, Returns);

      for (size_t i = 0; i < AllocaInsts.size(); ++i) {
        // Insert Instructions in reverse order so we can just keep inserting at
        // the front
        LoadInsts[i]->insertBefore(newFunction->begin()->begin());
        StoreInsts[i]->insertBefore(newFunction->begin()->begin());
        CastInsts[i]->insertBefore(newFunction->begin()->begin());
        AllocaInsts[i]->insertBefore(newFunction->begin()->begin());
      }

      if (changedReturn) {
        // Before each return instruction, create a new struct, save the vector
        // at its address and return the struct.
        for (ReturnsVector::iterator I = Returns.begin(), E = Returns.end();
             I != E; ++I) {
          Instruction *Alloca = new AllocaInst(newReturnType, Twine(""), *I);
          Instruction *Cast = new BitCastInst(
              Alloca, oldReturnType->getPointerTo(), Twine(""), *I);
          new StoreInst((*I)->getReturnValue(), Cast, *I);
          Instruction *Load = new LoadInst(Alloca, Twine(""), *I);
          ReturnInst::Create(M.getContext(), Load, *I);
          (*I)->dropAllReferences();
          (*I)->eraseFromParent();
        }
      }

      // Call the new function. Any changed arguments have already been changed
      // in the original call instructions
      Function::use_iterator UI = oldFunction->use_begin();
      while (!oldFunction->use_empty()) {
        Instruction *newCall;
        if (CallInst *oldCall = dyn_cast<CallInst>(*UI)) {
          vector<Value *> args;
          for (size_t i = 0; i < oldCall->getNumArgOperands(); ++i) {
            args.push_back(oldCall->getArgOperand(i));
          }
          newCall = CallInst::Create(newFunction, ArrayRef<Value *>(args),
                                     Twine(""), oldCall);
        } else if (InvokeInst *oldCall = dyn_cast<InvokeInst>(*UI)) {
          vector<Value *> args;
          for (size_t i = 0; i < oldCall->getNumArgOperands(); ++i) {
            args.push_back(oldCall->getArgOperand(i));
          }
          newCall = InvokeInst::Create(
              newFunction, oldCall->getNormalDest(), oldCall->getUnwindDest(),
              ArrayRef<Value *>(args), Twine(""), oldCall);
        } else {
          assert(0 && "Function used outside of Call or Invoke instruction");
        }

        // After each call instruction, cast the returned struct back into a
        // vector. Then delete the old call instruction, this is the last time
        // we'll need it
        Instruction *oldCall = cast<Instruction>(*UI);
        Instruction *Alloca = new AllocaInst(oldReturnType, Twine(""), oldCall);
        Instruction *Cast = new BitCastInst(
            Alloca, newReturnType->getPointerTo(), Twine(""), oldCall);
        new StoreInst(newCall, Cast, oldCall);
        Instruction *Load = new LoadInst(Alloca, Twine(""), oldCall);
        oldCall->replaceAllUsesWith(Load);
        oldCall->eraseFromParent();
        UI = oldFunction->use_begin();
      }

      oldFunction->deleteBody();
    }
  }
  return true;
}

