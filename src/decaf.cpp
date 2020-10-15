
#include "decaf.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <stdexcept>
#include <optional>

namespace decaf {
  /************************************************
   * Executor                                     *
   ************************************************/
  void Executor::add_context(Context&& ctx) {
    contexts.push_back(ctx);
  }

  void Executor::add_failure(const z3::model& model) {
    // TODO: It might not be necessary for the prototype but we'll need
    //       to figure what we want to do with test failures.
    std::cout
      << "Found failed model! Inputs: \n"
      << model << std::endl;
  }

  Context Executor::next_context() {
    DECAF_ASSERT(has_next());

    Context ctx = std::move(contexts.back());
    contexts.pop_back();
    return ctx;
  }

  bool Executor::has_next() const {
    return !contexts.empty();
  }

  /************************************************
   * StackFrame                                   *
   ************************************************/
  StackFrame::StackFrame(llvm::Function* function) :
    function(function),
    current_block(&function->getEntryBlock()),
    prev_block(nullptr),
    current(current_block->begin())
  {}
  
  void StackFrame::insert(llvm::Value* value, const z3::expr& expr) {
    variables.insert_or_assign(value, expr);
  }

  z3::expr StackFrame::lookup(llvm::Value* value, z3::context& ctx) const {
    if (auto* constant = llvm::dyn_cast_or_null<llvm::Constant>(value))
      return evaluate_constant(ctx, constant);
    
    auto it = variables.find(value);
    DECAF_ASSERT(it != variables.end(), "Tried to access unknown variable");
    return it->second;
  }

  /************************************************
   * Context                                      *
   ************************************************/
  Context::Context(z3::context& z3, llvm::Function* function) :
    solver(z3)
  {
    stack.emplace_back(function);
    StackFrame& frame = stack_top();

    int argnum = 0;
    for (auto& arg : function->args()) {
      z3::sort sort = sort_for_type(z3, arg.getType());
      z3::symbol symbol = z3.int_symbol(argnum);

      frame.variables.insert({ &arg, z3.constant(symbol, sort) });
      argnum += 1;
    }
  }

  StackFrame& Context::stack_top() {
    DECAF_ASSERT(!stack.empty());
    return stack.back();
  }

  z3::check_result Context::check(const z3::expr& expr) {
    auto cond = normalize_to_bool(expr);
    DECAF_ASSERT(cond.is_bool());
    return solver.check(1, &cond);
  }
  z3::check_result Context::check() {
    return solver.check();
  }

  void Context::add(const z3::expr& assertion) {
    solver.add(assertion);
  }

  /************************************************
   * Interpreter                                  *
   ************************************************/
  Interpreter::Interpreter(Context* ctx, Executor* queue, z3::context* z3) :
    ctx(ctx), queue(queue), z3(z3)
  {}

  void Interpreter::execute() {
    ExecutionResult exec;

    do {
      StackFrame& frame = ctx->stack_top();

      DECAF_ASSERT(
        frame.current != frame.current_block->end(),
        "Instruction pointer ran off end of block.");

      llvm::Instruction& inst = *frame.current;

      // Note: Need to increment the iterator before actually doing
      //       anything with the instruction since instructions can
      //       modify the current position (e.g. branch, call, etc.)
      ++frame.current;

      exec = visit(inst);
    } while (exec == ExecutionResult::Continue);
  }

  /************************************************
   * Free Functions                               *
   ************************************************/
  z3::sort sort_for_type(z3::context& ctx, llvm::Type* type) {
    if (type->isIntegerTy()) {
      return ctx.bv_sort(type->getIntegerBitWidth());
    }

    std::string message;
    llvm::raw_string_ostream os{message};
    os << "Unsupported LLVM type: ";
    type->print(os);

    DECAF_ABORT(message);
  }

  void execute_symbolic(llvm::Function* function) {
    z3::config cfg;

    // We want Z3 to generate models
    cfg.set("model", true);
    // Automatically select and configure the solver
    cfg.set("auto_config", true);

    z3::context z3{cfg};
    Executor exec;

    exec.add_context(Context(z3, function));

    while (exec.has_next()) {
      Context ctx = exec.next_context();
      Interpreter interp{&ctx, &exec, &z3};

      interp.execute();
    }
  }
  
  ExecutionResult Interpreter::visitAdd(llvm::BinaryOperator& op) {
    StackFrame& frame = ctx->stack_top();

    auto lhs = frame.lookup(op.getOperand(0), *z3);
    auto rhs = frame.lookup(op.getOperand(1), *z3);

    frame.insert(&op, lhs + rhs);

    return ExecutionResult::Continue;
  }

  ExecutionResult Interpreter::visitBranchInst(llvm::BranchInst& inst) {
    auto jump_to = [&](llvm::BasicBlock* target) {
      auto& frame = ctx->stack_top();
      frame.prev_block = frame.current_block;
      frame.current_block = target;
    };
    
    if (!inst.isConditional()) {
      jump_to(inst.getSuccessor(0));
      return ExecutionResult::Continue;
    }

    auto& frame = ctx->stack_top();
    auto cond = normalize_to_bool(frame.lookup(inst.getCondition(), *z3));

    auto is_t = ctx->check(cond);
    auto is_f = ctx->check(!cond);

    // Note: For the purposes of branching we consider unknown to be
    //       equivalent to sat. Maybe future branches will bring the
    //       equation back to being solvable.
    if (is_t != z3::unsat && is_f != z3::unsat) {
      auto fork = ctx->fork();

      // In cases where both conditions are possible we follow the
      // false path. This should be enough to get us out of most loops
      // and actually exploring the rest of the program.
      fork.add(cond);
      ctx->add(!cond);

      queue->add_context(std::move(fork));
      return ExecutionResult::Continue;
    } else if (is_t != z3::unsat) {
      ctx->add(cond);
      return ExecutionResult::Continue;
    } else if (is_f != z3::unsat) {
      ctx->add(!cond);
      return ExecutionResult::Continue;
    } else {
      return ExecutionResult::Stop;
    }
  }

  ExecutionResult Interpreter::visitReturnInst(llvm::ReturnInst& inst) {
    auto& frame = ctx->stack_top();

    std::optional<z3::expr> result;
    if (inst.getNumOperands() != 0)
      result = frame.lookup(inst.getOperand(0), *z3);

    ctx->stack.pop_back();

    // Reached end of function, nothing left to do.
    if (ctx->stack.empty())
      return ExecutionResult::Stop;

    if (result.has_value()) {
      auto& parent = ctx->stack_top();
      auto& caller = *std::prev(parent.current);

      parent.insert(&caller, *result);
    }

    return ExecutionResult::Continue;
  }

  ExecutionResult Interpreter::visitPHINode(llvm::PHINode& node) {
    auto& frame = ctx->stack_top();

    // PHI nodes in the entry block is invalid.
    DECAF_ASSERT(frame.prev_block != nullptr);

    auto value = frame.lookup(node.getIncomingValueForBlock(frame.prev_block), *z3);
    frame.insert(&node, value);

    return ExecutionResult::Continue;
  }

  z3::expr evaluate_constant(z3::context& ctx, llvm::Constant* constant) {
    if (auto* intconst = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
      const llvm::APInt& value = intconst->getValue();

      if (value.getBitWidth() <= 64) {
        return ctx.bv_val(value.getLimitedValue(), value.getBitWidth());
      } 

      // This isn't particularly efficient. Unfortunately, when it comes
      // to integers larger than uint64_t there's no efficient way to get
      // them into Z3. The options are either
      //  - Convert to base-10 string and use that
      //  - Put every single bit into a separate boolean then load that
      // I've opted to go the string route since it's easier here. Maybe
      // in the future we can get an API for doing this more efficiently
      // added to Z3.
      llvm::SmallString<64> str;
      value.toStringUnsigned(str, 10);

      return ctx.bv_val(str.c_str(), value.getBitWidth());
    }

    // We only implement integers at the moment
    DECAF_UNIMPLEMENTED();
  }

  z3::expr normalize_to_bool(const z3::expr& expr) {
    auto sort = expr.get_sort();

    if (sort.is_int() && sort.bv_size() == 1)
      return expr == 1;

    return expr;
  }
}
