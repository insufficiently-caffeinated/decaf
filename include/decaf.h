
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

    StackFrame(llvm::Function* function);
    
    /**
     * Insert a new value into the current stack frame. If that value
     * is already in the current stack frame then it overwrites it.
     */
    void insert(llvm::Value* value, const z3::expr& expr);
    /**
     * Lookup a value within the current stack frame.
     * 
     * There are two main cases here:
     * 1. `value` is an existing variable
     * 2. `value` is a constant
     * 
     * In the first case we just look up the variable the `variables` map
     * and then return it. In the second case we build a Z3 expression
     * that represents the constant and return that.
     * 
     * This method should be preferred over directly interacting with
     * `variables` as it correctly handles constants.
     */
    z3::expr lookup(llvm::Value* value, z3::context& ctx) const;
  };

  class Context {
  public:
    std::vector<StackFrame> stack;
    z3::solver solver;

    Context(z3::context& z3, llvm::Function* function);

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
    void add_context(Context&& ctx);

    // The current context has encountered a failure that needs to be
    // recorded.
    void add_failure(const z3::model& model);

    // Get the next context to be executed.
    Context next_context();

    // Are there any contexts left?
    bool has_next() const;
  };

  enum class ExecutionResult {
    Continue,
    Stop
  };

  class Interpreter : public llvm::InstVisitor<Interpreter, ExecutionResult> {
  private:
    Context* ctx;
    Executor* queue;
    z3::context* z3;

  public:
    // Add some more parameters here
    Interpreter(Context* ctx, Executor* queue, z3::context* z3);

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
    ExecutionResult visitAdd(llvm::BinaryOperator& op);
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

  /**
   * Get the Z3 sort corresponding to the provided LLVM type.
   *
   * Only works for supported scalar values at the moment
   * (i.e. only integers).
   *
   * Invalid types will result in an assertion.
   */
  z3::sort sort_for_type(z3::context& ctx, llvm::Type* type);

  /**
   * Executes the given function symbolically.
   *
   * Currently this works by making all the function arguments symbolic.
   * Assertion failures during symbolic execution will result in the
   * model being printed to stdout.
   *
   * Note: This is probably good enough for the prototype but we should
   *       definitely refine the interface as we figure out the best API
   *       for this.
   */
  void execute_symbolic(llvm::Function* function);

  /**
   * Create a Z3 expression with the same value as the given constant.
   * 
   * Currently only supports integers and will abort on any other LLVM
   * type.
   */
  z3::expr evaluate_constant(z3::context& ctx, llvm::Constant* constant);
}
