
#include "decaf.h"

#include <boost/range/combine.hpp>
#include <boost/range/iterator_range.hpp>
#include <fmt/format.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <optional>
#include <stdexcept>

namespace decaf {
/************************************************
 * Executor                                     *
 ************************************************/
void Executor::add_context(Context &&ctx) {
  contexts.push_back(ctx);
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
StackFrame::StackFrame(llvm::Function *function)
    : function(function), current_block(&function->getEntryBlock()), prev_block(nullptr),
      current(current_block->begin()) {}

void StackFrame::jump_to(llvm::BasicBlock *block) {
  prev_block = current_block;
  current_block = block;
  current = block->begin();
}

void StackFrame::insert(llvm::Value *value, const z3::expr &expr) {
  variables.insert_or_assign(value, expr);
}

z3::expr StackFrame::lookup(llvm::Value *value, z3::context &ctx) const {
  if (auto *constant = llvm::dyn_cast_or_null<llvm::Constant>(value))
    return evaluate_constant(ctx, constant);

  auto it = variables.find(value);
  DECAF_ASSERT(it != variables.end(), "Tried to access unknown variable");
  return it->second;
}

/************************************************
 * Context                                      *
 ************************************************/

Context::Context(z3::context &z3, llvm::Function *function)
    : solver(z3::tactic(z3, "default").mk_solver()) {
  stack.emplace_back(function);
  StackFrame &frame = stack_top();

  int argnum = 0;
  for (auto &arg : function->args()) {
    z3::sort sort = sort_for_type(z3, arg.getType());
    z3::symbol symbol = z3.int_symbol(argnum);

    frame.variables.insert({&arg, z3.constant(symbol, sort)});
    argnum += 1;
  }
}
Context::Context(const Context &ctx, z3::solver &&solver)
    : stack(ctx.stack), solver(std::move(solver)) {}

StackFrame &Context::stack_top() {
  DECAF_ASSERT(!stack.empty());
  return stack.back();
}

z3::check_result Context::check(const z3::expr &expr) {
  auto cond = normalize_to_bool(expr);
  DECAF_ASSERT(cond.is_bool());
  return solver.check(1, &cond);
}
z3::check_result Context::check() {
  return solver.check();
}

void Context::add(const z3::expr &assertion) {
  DECAF_ASSERT(assertion.is_bool(), "assertions must be booleans");
  solver.add(assertion);
}

Context Context::fork() const {
  z3::solver new_solver = z3::tactic(solver.ctx(), "default").mk_solver();

  for (const auto &assertion : solver.assertions()) {
    new_solver.add(assertion);
  }

  return Context(*this, std::move(new_solver));
}

/************************************************
 * Interpreter                                  *
 ************************************************/
Interpreter::Interpreter(Context *ctx, Executor *queue, z3::context *z3, FailureTracker *tracker)
    : ctx(ctx), queue(queue), z3(z3), tracker(tracker) {}

void Interpreter::execute() {
  ExecutionResult exec;

  do {
    StackFrame &frame = ctx->stack_top();

    DECAF_ASSERT(frame.current != frame.current_block->end(),
                 "Instruction pointer ran off end of block.");

    llvm::Instruction &inst = *frame.current;

    // Note: Need to increment the iterator before actually doing
    //       anything with the instruction since instructions can
    //       modify the current position (e.g. branch, call, etc.)
    ++frame.current;

    exec = visit(inst);
  } while (exec == ExecutionResult::Continue);
}

ExecutionResult Interpreter::visitAdd(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs + rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitSub(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs - rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitMul(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs * rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitICmpInst(llvm::ICmpInst &icmp) {
  using llvm::ICmpInst;

  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(icmp.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(icmp.getOperand(1), *z3));

  switch (icmp.getPredicate()) {
  case ICmpInst::ICMP_EQ:
    frame.insert(&icmp, lhs == rhs);
    break;
  case ICmpInst::ICMP_NE:
    frame.insert(&icmp, lhs != rhs);
    break;
  case ICmpInst::ICMP_UGT:
    frame.insert(&icmp, z3::ugt(lhs, rhs));
    break;
  case ICmpInst::ICMP_UGE:
    frame.insert(&icmp, z3::uge(lhs, rhs));
    break;
  case ICmpInst::ICMP_ULT:
    frame.insert(&icmp, z3::ult(lhs, rhs));
    break;
  case ICmpInst::ICMP_ULE:
    frame.insert(&icmp, z3::ule(lhs, rhs));
    break;
  case ICmpInst::ICMP_SGT:
    frame.insert(&icmp, lhs > rhs);
    break;
  case ICmpInst::ICMP_SGE:
    frame.insert(&icmp, lhs >= rhs);
    break;
  case ICmpInst::ICMP_SLT:
    frame.insert(&icmp, lhs < rhs);
    break;
  case ICmpInst::ICMP_SLE:
    frame.insert(&icmp, lhs <= rhs);
    break;
  default:
    DECAF_UNREACHABLE();
  }

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitSDiv(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  if (ctx->check(rhs == 0 || !z3::bvsdiv_no_overflow(lhs, rhs)) == z3::sat) {
    tracker->add_failure(*ctx, ctx->solver.get_model());
  }
  ctx->add(rhs != 0);
  ctx->add(z3::bvsdiv_no_overflow(lhs, rhs));

  frame.insert(&op, lhs / rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitUDiv(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  if (ctx->check(rhs == 0) == z3::sat) {
    tracker->add_failure(*ctx, ctx->solver.get_model());
  }
  ctx->add(rhs != 0);

  frame.insert(&op, z3::udiv(lhs, rhs));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitSRem(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  if (ctx->check(rhs == 0 || !z3::bvsdiv_no_overflow(lhs, rhs)) == z3::sat) {
    tracker->add_failure(*ctx, ctx->solver.get_model());
  }
  ctx->add(rhs != 0);
  ctx->add(z3::bvsdiv_no_overflow(lhs, rhs));

  frame.insert(&op, lhs % rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitURem(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  if (ctx->check(rhs == 0) == z3::sat) {
    tracker->add_failure(*ctx, ctx->solver.get_model());
  }
  ctx->add(rhs != 0);

  frame.insert(&op, z3::urem(lhs, rhs));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitShl(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, z3::shl(lhs, rhs));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitAShr(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, z3::ashr(lhs, rhs));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitLShr(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, z3::lshr(lhs, rhs));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitAnd(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs & rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitOr(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs | rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitXor(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto lhs = normalize_to_int(frame.lookup(op.getOperand(0), *z3));
  auto rhs = normalize_to_int(frame.lookup(op.getOperand(1), *z3));

  frame.insert(&op, lhs ^ rhs);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitNot(llvm::BinaryOperator &op) {
  StackFrame &frame = ctx->stack_top();

  auto expr = normalize_to_int(frame.lookup(op.getOperand(0), *z3));

  frame.insert(&op, ~expr);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitTrunc(llvm::TruncInst &trunc) {
  auto &frame = ctx->stack_top();

  auto src = normalize_to_int(frame.lookup(trunc.getOperand(0), *z3));
  auto dst_ty = trunc.getDestTy();

  DECAF_ASSERT(dst_ty->isIntegerTy());
  DECAF_ASSERT(dst_ty->getIntegerBitWidth() <= src.get_sort().bv_size());

  frame.insert(&trunc, src.extract(dst_ty->getIntegerBitWidth() - 1, 0));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitBranchInst(llvm::BranchInst &inst) {
  if (!inst.isConditional()) {
    ctx->stack_top().jump_to(inst.getSuccessor(0));
    return ExecutionResult::Continue;
  }

  auto &frame = ctx->stack_top();
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

    fork.stack_top().jump_to(inst.getSuccessor(0));
    ctx->stack_top().jump_to(inst.getSuccessor(1));

    queue->add_context(std::move(fork));
    return ExecutionResult::Continue;
  } else if (is_t != z3::unsat) {
    ctx->add(cond);
    ctx->stack_top().jump_to(inst.getSuccessor(0));
    return ExecutionResult::Continue;
  } else if (is_f != z3::unsat) {
    ctx->add(!cond);
    ctx->stack_top().jump_to(inst.getSuccessor(1));
    return ExecutionResult::Continue;
  } else {
    return ExecutionResult::Stop;
  }
}

ExecutionResult Interpreter::visitReturnInst(llvm::ReturnInst &inst) {
  auto &frame = ctx->stack_top();

  std::optional<z3::expr> result;
  if (inst.getNumOperands() != 0)
    result = frame.lookup(inst.getOperand(0), *z3);

  ctx->stack.pop_back();

  // Reached end of function, nothing left to do.
  if (ctx->stack.empty())
    return ExecutionResult::Stop;

  if (result.has_value()) {
    auto &parent = ctx->stack_top();
    auto &caller = *std::prev(parent.current);

    parent.insert(&caller, *result);
  }

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitPHINode(llvm::PHINode &node) {
  auto &frame = ctx->stack_top();

  // PHI nodes in the entry block is invalid.
  DECAF_ASSERT(frame.prev_block != nullptr);

  auto value = frame.lookup(node.getIncomingValueForBlock(frame.prev_block), *z3);
  frame.insert(&node, value);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitCallInst(llvm::CallInst &call) {
  auto func = call.getCalledFunction();

  if (func->isIntrinsic()) {
    DECAF_ABORT(fmt::format("Intrinsic function '{}' not supported", func->getName().str()));
  }

  DECAF_ASSERT(!call.isIndirectCall(), "Indirect functions are not implemented yet");

  if (func->empty())
    return visitExternFunc(call);

  StackFrame callee{func};
  auto &frame = ctx->stack_top();

  for (auto arg_pair :
       boost::combine(boost::make_iterator_range(func->arg_begin(), func->arg_end()),
                      boost::make_iterator_range(call.arg_begin(), call.arg_end()))) {
    callee.insert(&boost::get<0>(arg_pair), frame.lookup(boost::get<1>(arg_pair).get(), *z3));
  }

  ctx->stack.push_back(std::move(callee));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitSelectInst(llvm::SelectInst &inst) {
  auto &frame = ctx->stack_top();

  auto cond = normalize_to_bool(frame.lookup(inst.getCondition(), *z3));
  auto t_val = normalize_to_int(frame.lookup(inst.getTrueValue(), *z3));
  auto f_val = normalize_to_int(frame.lookup(inst.getFalseValue(), *z3));

  frame.insert(&inst, z3::ite(cond, t_val, f_val));

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitExternFunc(llvm::CallInst &call) {
  auto func = call.getCalledFunction();
  auto name = func->getName();

  DECAF_ASSERT(func->empty(), "visitExternFunc called with non-external function");

  if (name == "decaf_assert")
    return visitAssert(call);
  if (name == "decaf_assume")
    return visitAssume(call);

  DECAF_ABORT(fmt::format("external function '{}' not implemented", name.str()));
}

ExecutionResult Interpreter::visitAssume(llvm::CallInst &call) {
  DECAF_ASSERT(call.getNumArgOperands() == 1);

  auto &frame = ctx->stack_top();
  ctx->add(normalize_to_bool(frame.lookup(call.getArgOperand(0), *z3)));

  // Don't check whether adding the assumption causes this path to become
  // dead since assumptions are rare, solver calls are expensive, and it'll
  // get caught at the next conditional branch anyway.
  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitAssert(llvm::CallInst &call) {
  DECAF_ASSERT(call.getNumArgOperands() == 1);

  auto &frame = ctx->stack_top();
  auto assertion = normalize_to_bool(frame.lookup(call.getArgOperand(0), *z3));

  DECAF_ASSERT(assertion.is_bool(),
               fmt::format("Called decaf_assert with invalid type, found: {}, expected bool",
                           assertion.get_sort().to_string()));

  if (ctx->check(!assertion) == z3::sat) {
    tracker->add_failure(*ctx, ctx->solver.get_model());
  }
  ctx->add(assertion);

  return ExecutionResult::Continue;
}

ExecutionResult Interpreter::visitInstruction(llvm::Instruction &inst) {
  DECAF_ABORT(fmt::format("Instruction '{}' not implemented!", inst.getOpcodeName()));
}

/************************************************
 * PrintingFailureTracker                       *
 ************************************************/
void PrintingFailureTracker::add_failure(Context &, const z3::model &model) {
  std::cout << "Found failed model! Inputs: \n" << model << std::endl;
}

FailureTracker *PrintingFailureTracker::default_instance() {
  static PrintingFailureTracker instance;

  return &instance;
}

/************************************************
 * Free Functions                               *
 ************************************************/
z3::sort sort_for_type(z3::context &ctx, llvm::Type *type) {
  if (type->isIntegerTy()) {
    return ctx.bv_sort(type->getIntegerBitWidth());
  }

  std::string message;
  llvm::raw_string_ostream os{message};
  os << "Unsupported LLVM type: ";
  type->print(os);

  DECAF_ABORT(message);
}

void execute_symbolic(llvm::Function *function, FailureTracker *tracker) {
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
    Interpreter interp{&ctx, &exec, &z3, tracker};

    interp.execute();
  }
}

z3::expr evaluate_constant(z3::context &ctx, llvm::Constant *constant) {
  if (auto *intconst = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
    const llvm::APInt &value = intconst->getValue();

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

z3::expr normalize_to_bool(const z3::expr &expr) {
  auto sort = expr.get_sort();

  if (sort.is_bv() && sort.bv_size() == 1)
    return expr == 1;

  return expr;
}

z3::expr normalize_to_int(const z3::expr &expr) {
  auto sort = expr.get_sort();

  if (sort.is_bool()) {
    auto &ctx = expr.ctx();
    return z3::ite(expr, ctx.bv_val(1, 1), ctx.bv_val(0, 1));
  }

  return expr;
}
} // namespace decaf
