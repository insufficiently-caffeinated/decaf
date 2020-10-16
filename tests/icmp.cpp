
#include <gtest/gtest.h>

#include "common.h"
#include "decaf.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

using decaf::Context;
using decaf::Interpreter;

using llvm::BinaryOperator;
using llvm::ICmpInst;

class ICmpTests : public ::testing::Test {
protected:
  LLVMContext llvm;
  z3::context z3 = default_context();

  Constant *MakeConstant(uint64_t value) {
    return ConstantInt::get(IntegerType::getInt32Ty(llvm), APInt(32, value));
  }
};

TEST_F(ICmpTests, test_eq) {
  auto func = empty_function(llvm);
  auto val1 = MakeConstant(10);
  auto val2 = MakeConstant(5);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto add = BinaryOperator::CreateAdd(val2, val2, "add", &bb);
  auto cmp1 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, val1, val1, "cmp1", &bb);
  auto cmp2 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, val1, add, "cmp2", &bb);
  auto cmp3 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, val1, val2, "cmp3", &bb);

  interp.visit(add);
  interp.visit(cmp1);
  interp.visit(cmp2);
  interp.visit(cmp3);

  auto &frame = ctx.stack_top();
  ctx.add(frame.lookup(cmp1, z3));
  ctx.add(frame.lookup(cmp2, z3));
  ctx.add(!frame.lookup(cmp3, z3));

  ASSERT_EQ(ctx.check(), z3::sat);
}

TEST_F(ICmpTests, test_eq_ne_duals) {
  auto func = empty_function(llvm);
  auto val = MakeConstant(0);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto dummy1 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto dummy2 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto eq = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_EQ, dummy1, dummy2, "", &bb);
  auto ne = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_NE, dummy1, dummy2, "", &bb);

  auto &frame = ctx.stack_top();
  frame.insert(dummy1, z3.bv_const("a", 32));
  frame.insert(dummy2, z3.bv_const("b", 32));

  interp.visit(eq);
  interp.visit(ne);

  auto cmp_eq = frame.lookup(eq, z3);
  auto cmp_ne = frame.lookup(ne, z3);

  ASSERT_EQ(ctx.check((cmp_eq && cmp_ne) || (!cmp_eq && !cmp_ne)), z3::unsat);
}

TEST_F(ICmpTests, test_ule_ugt_duals) {
  auto func = empty_function(llvm);
  auto val = MakeConstant(0);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto dummy1 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto dummy2 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto expr1 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_ULE, dummy1, dummy2, "", &bb);
  auto expr2 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_UGT, dummy1, dummy2, "", &bb);

  auto &frame = ctx.stack_top();
  frame.insert(dummy1, z3.bv_const("a", 32));
  frame.insert(dummy2, z3.bv_const("b", 32));

  interp.visit(expr1);
  interp.visit(expr2);

  auto cmp_ult = frame.lookup(expr1, z3);
  auto cmp_uge = frame.lookup(expr2, z3);

  ASSERT_EQ(ctx.check((cmp_ult && cmp_uge) || (!cmp_ult && !cmp_uge)), z3::unsat);
}

TEST_F(ICmpTests, test_ult_uge_duals) {
  auto func = empty_function(llvm);
  auto val = MakeConstant(0);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto dummy1 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto dummy2 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto expr1 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_ULT, dummy1, dummy2, "", &bb);
  auto expr2 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_UGE, dummy1, dummy2, "", &bb);

  auto &frame = ctx.stack_top();
  frame.insert(dummy1, z3.bv_const("a", 32));
  frame.insert(dummy2, z3.bv_const("b", 32));

  interp.visit(expr1);
  interp.visit(expr2);

  auto cmp_ule = frame.lookup(expr1, z3);
  auto cmp_ugt = frame.lookup(expr2, z3);

  ASSERT_EQ(ctx.check((cmp_ule && cmp_ugt) || (!cmp_ule && !cmp_ugt)), z3::unsat);
}

TEST_F(ICmpTests, test_sle_sgt_duals) {
  auto func = empty_function(llvm);
  auto val = MakeConstant(0);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto dummy1 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto dummy2 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto expr1 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_SLE, dummy1, dummy2, "", &bb);
  auto expr2 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_SGT, dummy1, dummy2, "", &bb);

  auto &frame = ctx.stack_top();
  frame.insert(dummy1, z3.bv_const("a", 32));
  frame.insert(dummy2, z3.bv_const("b", 32));

  interp.visit(expr1);
  interp.visit(expr2);

  auto cmp_slt = frame.lookup(expr1, z3);
  auto cmp_sge = frame.lookup(expr2, z3);

  ASSERT_EQ(ctx.check((cmp_slt && cmp_sge) || (!cmp_slt && !cmp_sge)), z3::unsat);
}

TEST_F(ICmpTests, test_slt_sge_duals) {
  auto func = empty_function(llvm);
  auto val = MakeConstant(0);

  Context ctx{z3, func.get()};
  Interpreter interp{&ctx, nullptr, &z3};

  auto &bb = func->getEntryBlock();
  auto dummy1 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto dummy2 = BinaryOperator::CreateAdd(val, val, "", &bb);
  auto expr1 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_SLT, dummy1, dummy2, "", &bb);
  auto expr2 = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_SGE, dummy1, dummy2, "", &bb);

  auto &frame = ctx.stack_top();
  frame.insert(dummy1, z3.bv_const("a", 32));
  frame.insert(dummy2, z3.bv_const("b", 32));

  interp.visit(expr1);
  interp.visit(expr2);

  auto cmp_sle = frame.lookup(expr1, z3);
  auto cmp_sgt = frame.lookup(expr2, z3);

  ASSERT_EQ(ctx.check((cmp_sle && cmp_sgt) || (!cmp_sle && !cmp_sgt)), z3::unsat);
}
