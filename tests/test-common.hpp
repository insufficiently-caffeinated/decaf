#ifndef DECAF_TEST_COMMON_H
#define DECAF_TEST_COMMON_H

#include "decaf.h"

#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/WithColor.h>

#include <memory>

using namespace llvm;
using decaf::Context;
using decaf::FailureTracker;

cl::opt<std::string> input_filename{cl::Positional};
cl::opt<std::string> target_method{cl::Positional};

ExitOnError exit_on_err;

class CountingFailureTracker : public FailureTracker {
public:
  uint64_t count = 0;

  void add_failure(Context &ctx, const z3::model &model) override {
    count += 1;

    std::cout << "Found failure:\n" << model << std::endl;
    std::cout << ctx.solver.to_smt2();
  }
};

static std::unique_ptr<Module> loadFile(const char *argv0, const std::string &filename,
                                        LLVMContext &context) {
  llvm::SMDiagnostic error;
  auto module = llvm::parseIRFile(filename, error, context);

  if (!module) {
    error.print(argv0, llvm::errs());
    return nullptr;
  }

  return module;
}

static void run_with_tracker(int argc, char **argv, FailureTracker *tracker) {
  DECAF_ASSERT(tracker != nullptr);

  InitLLVM X(argc, argv);
  exit_on_err.setBanner(std::string(argv[0]) + ':');

  LLVMContext context;
  cl::ParseCommandLineOptions(argc, argv, argv[0]);

  auto module = loadFile(argv[0], input_filename.getValue(), context);
  if (!module) {
    errs() << argv[0] << ": ";
    WithColor::error() << " loading file '" << input_filename.getValue() << "'\n";
    std::exit(1);
  }

  auto function = module->getFunction(target_method.getValue());
  if (!function) {
    errs() << argv[0] << ": ";
    WithColor::error() << " no method '" << target_method.getValue() << "'";
    std::exit(1);
  }

  decaf::execute_symbolic(function, tracker);
}

#endif
