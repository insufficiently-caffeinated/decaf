
#include "common.h"

z3::context default_context() {
  z3::config cfg;

  // We want Z3 to generate models
  cfg.set("model", true);
  // Automatically select and configure the solver
  cfg.set("auto_config", true);

  return z3::context{cfg};
}

std::unique_ptr<llvm::Function> empty_function(llvm::LLVMContext &llvm) {
  auto function =
      llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(llvm), false),
                             llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                             0 // addrspace
      );

  llvm::BasicBlock::Create(llvm, "entry", function);

  return std::unique_ptr<llvm::Function>(function);
}
