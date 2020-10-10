
#include "decaf.h"

#include <llvm/IR/Instructions.h>

namespace decaf {
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
