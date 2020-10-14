
#include "decaf.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <stdexcept>

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
    variables.insert({ value, expr });
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

  z3::expr evaluate_constant(z3::context& ctx, llvm::Constant* constant) {
    auto type = constant->getType();

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
}
