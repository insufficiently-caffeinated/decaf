
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <llvm/IR/Function.h>
#include <z3++.h>

#include <memory>

/**
 * Create a z3 context with the required configuration.
 *
 * This should be kept roughly in sync with the context creation
 * within decaf::execute_symbolic.
 */
z3::context default_context();

/**
 * Creates a function with a single basic block.
 *
 * When creating a few simple instructions you should insert them into
 * the provided basic block otherwise you'll get LLVM assertions when
 * attempting to run the tests.
 */
std::unique_ptr<llvm::Function> empty_function(llvm::LLVMContext &llvm);

#endif
