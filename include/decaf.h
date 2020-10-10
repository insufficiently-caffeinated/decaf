
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
    llvm::BasicBlock::iterator current;
  };

  class Context {
  public:
    std::vector<StackFrame> stack;
    z3::solver solver;

    Context fork() const {
      DECAF_UNIMPLEMENTED();
    }

    StackFrame& stack_top();
  };

  class Executor {
  private:
    std::vector<Context> contexts;

  public:
    // The current context has forked and the fork needs to be added
    // to the queue.
    void add_context(Context&& ctx) {}

    // The current context has encountered a failure that needs to be
    // recorded.
    void add_failure(const z3::model& model) {}
  };

  enum class ExecutionResult {
    Continue,
    Stop
  };

  class Interpreter : public llvm::InstVisitor<Interpreter, ExecutionResult> {
  private:
    Context* ctx;
    Executor* queue;

  public:
    // Add some more parameters here
    Interpreter(
      Context* ctx,
      Executor* queue
    );

    /**
     * Execute this interpreter's context until it finishes.
     * 
     * Contexts from forks will be placed into the execution queue.
     * Failures resulting from this context will also be added to
     * the execution queue.
     */
    void execute();

    // Marks an unimplemented instruction.
    //
    // TODO: Better error message?
    ExecutionResult visitInstruction(llvm::Instruction&) {
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
