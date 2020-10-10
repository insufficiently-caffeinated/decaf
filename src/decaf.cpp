
#include "decaf.h"

#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>

namespace decaf {
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
}
