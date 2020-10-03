
#include <llvm/IR/InstVisitor.h>

#include <z3++.h>

#include <cassert>
#include <vector>
#include <unordered_map>

#include "macros.h"

namespace decaf {
  class StackFrame {
  public:
    std::unordered_map<llvm::Value*, z3::expr> variables;
    
    llvm::Function* function;
    llvm::BasicBlock* current_block;
    llvm::BasicBlock* prev_block;
    // Need a current instruction pointer (maybe block iterator?)
  };

  class Context {
  public:
    std::vector<StackFrame> stack;

    Context fork() const {
      assert("not implemented");
      std::exit(EXIT_FAILURE);
    }
  };

  enum class ExecutionResult {
    Continue,
    Stop
  };

  class Interpreter : public llvm::InstVisitor<Interpreter, ExecutionResult> {
  private:
    Context* ctx; // Non-owning context reference

  public:
    // Add some more parameters here
    Interpreter();

    ExecutionResult visitInstruction(llvm::Instruction& I) {
      DECAF_UNIMPLEMENTED();
    }

    // Replace this with implementation in cpp file as we go
    ExecutionResult visitAdd(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitSub(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitMul(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitUDiv(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitSDiv(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitURem(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitSRem(llvm::BinaryOperator& op) { DECAF_UNIMPLEMENTED(); }

    ExecutionResult visitPHINode(llvm::PHINode& node) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitBranchInst(llvm::BranchInst& inst) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitReturnInst(llvm::ReturnInst& inst) { DECAF_UNIMPLEMENTED(); }
    ExecutionResult visitCallInst(llvm::CallInst& inst) { DECAF_UNIMPLEMENTED(); }
  };
}
