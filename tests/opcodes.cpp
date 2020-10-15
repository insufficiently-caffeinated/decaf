
#include <gtest/gtest.h>

#include "decaf.h"

#include <llvm/IR/Instructions.h>

#include <memory>

using namespace decaf;

using llvm::ConstantInt;
using llvm::LLVMContext;
using llvm::IntegerType;
using llvm::APInt;

// Create a z3 context with the required configuration
//
// This should be kept roughly in sync with the context creation
// within decaf::execute_symbolic.
static z3::context default_context() {
  z3::config cfg;

  // We want Z3 to generate models
  cfg.set("model", true);
  // Automatically select and configure the solver
  cfg.set("auto_config", true);

  return z3::context{cfg};
}

/**
 * Creates a function with a single basic block.
 * 
 * When creating a few simple instructions you should insert them into
 * the provided basic block otherwise you'll get LLVM assertions when
 * attempting to run the tests.
 */
std::unique_ptr<llvm::Function> empty_function(LLVMContext& llvm) {
  auto function = llvm::Function::Create(
    llvm::FunctionType::get(llvm::Type::getVoidTy(llvm), false),
    llvm::GlobalValue::LinkageTypes::PrivateLinkage,
    0 // addrspace
  );

  llvm::BasicBlock::Create(llvm, "entry", function);

  return std::unique_ptr<llvm::Function>(function);
}

/**
 * Tests that creating constant integers with bitwidth > 64 works
 * as expected.
 */
TEST(opcodes, large_constant_integer) {
  LLVMContext llvm;
  z3::context z3 = default_context();

  unsigned bitwidth = 20000;
  const char* string = "99999999999999999999999999999999999999999999999999991";
  auto value = ConstantInt::get(IntegerType::get(llvm, bitwidth), APInt(bitwidth, string, 10));

  auto evaluated = decaf::evaluate_constant(z3, value);
  auto expected = z3.bv_val(string, bitwidth);

  z3::solver solver{z3};
  solver.add(evaluated == expected);

  ASSERT_EQ(solver.check(), z3::sat);
}

/**
 * Test that 7 + 9 + 5 == 21.
 * 
 * This tests that
 * 1. Creating integer constants works properly.
 * 2. Addition instructions with only constant operands work properly.
 * 3. Addition instructions with at least one non-constant operand work
 *    properly.
 */
TEST(opcodes, basic_add_test) {
  LLVMContext llvm;
  z3::context z3 = default_context();

  auto func = empty_function(llvm);
  auto val1 = ConstantInt::get(IntegerType::getInt32Ty(llvm), APInt(32, 7));
  auto val2 = ConstantInt::get(IntegerType::getInt32Ty(llvm), APInt(32, 9));
  auto val3 = ConstantInt::get(IntegerType::getInt32Ty(llvm), APInt(32, 5));

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto& bb = func->getEntryBlock();
  auto add1 = llvm::BinaryOperator::CreateAdd(val1, val2, "add1", &bb);
  auto add2 = llvm::BinaryOperator::CreateAdd(add1, val3, "add2", &bb);

  interp.visitAdd(*add1);
  interp.visitAdd(*add2);

  auto expr = ctx.stack_top().lookup(add2, z3);
  ctx.solver.add(expr == 7 + 9 + 5);

  EXPECT_EQ(ctx.solver.check(), z3::sat);
}

TEST(opcodes, 1bit_add_test) {
  LLVMContext llvm;
  z3::context z3 = default_context();

  auto func = empty_function(llvm);

  // There's only two constant 1-bit integers
  auto val0 = ConstantInt::get(IntegerType::getInt1Ty(llvm), APInt(1, 0));
  auto val1 = ConstantInt::get(IntegerType::getInt1Ty(llvm), APInt(1, 1));

  auto& bb = func->getEntryBlock();
  // We'll never actually use this value, just need a non-constant value
  // type that we can manually insert into the context.
  auto dummy = llvm::BinaryOperator::CreateAdd(val0, val1, "dummy", &bb);
  
  auto add0 = llvm::BinaryOperator::CreateAdd(dummy, val0, "add0", &bb);
  auto add1 = llvm::BinaryOperator::CreateAdd(dummy, val1, "add1", &bb);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  ctx.stack_top().insert(dummy, z3.bool_val(true));

  interp.visit(add0);
  interp.visit(add1);

  auto& frame = ctx.stack_top();
  auto expr0 = frame.lookup(add0, z3);
  auto expr1 = frame.lookup(add1, z3);

  // 1 + 0 == 1 (mod 2)
  EXPECT_EQ(ctx.check(expr0 == true), z3::sat);
  // 1 + 1 == 0 (mod 2)
  EXPECT_EQ(ctx.check(expr1 == false), z3::sat);
}
