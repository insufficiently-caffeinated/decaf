
#include <llvm/IR/InstVisitor.h>

#include <z3++.h>

#include <cassert>
#include <vector>
#include <unordered_map>

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
    z3::solver solver;

    Context fork() const {
      assert("not implemented");
      std::exit(EXIT_FAILURE);
    }
  };

  class Executor {
  private:
    std::vector<Context> contexts;

  public:
    void add_context(Context&& ctx) {}
    void add_failure(const z3::model& model) {}
  };

  enum class ExecutionResult {
    Continue,
    Stop,
    Unimplemented
  };

  class Interpreter : public llvm::InstVisitor<Interpreter, ExecutionResult> {
  private:
    Context* ctx; // Non-owning context reference
    Executor* exec;

  public:
    // Add some more parameters here
    Interpreter();

    ExecutionResult visitInstruction(llvm::Instruction& I) {
      assert("Instruction not implemented");
      return ExecutionResult::Unimplemented;
    }

    // Replace this with implementation in cpp file as we go
    ExecutionResult visitAdd(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitSub(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitMul(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitUDiv(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitSDiv(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitURem(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitSRem(llvm::BinaryOperator& op) { return ExecutionResult::Unimplemented; }

    ExecutionResult visitPHINode(llvm::PHINode& node) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitBranchInst(llvm::BranchInst& inst) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitReturnInst(llvm::ReturnInst& inst) { return ExecutionResult::Unimplemented; }
    ExecutionResult visitCallInst(llvm::CallInst& inst) { return ExecutionResult::Unimplemented; }
  };
}
