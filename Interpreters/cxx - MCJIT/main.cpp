/*
Author: @m3rcer
MCJIT interpreter POC
*/

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

int main(int argc, char **argv) {
    llvm::InitLLVM X(argc, argv);
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    if (argc < 2) {
        errs() << "Usage:\n";
        errs() << "  " << argv[0] << " <LLVM IR file> [main-args...] [--load=shared-lib]\n\n";
        errs() << "Options:\n";
        errs() << "  <LLVM IR file>         Path to LLVM IR file (.ll) to execute\n";
        errs() << "  [main-args...]         (Optional) Arguments passed to main(argc, argv)\n";
        errs() << "  --load=shared-lib      (Optional) Load shared library for symbols\n";
        return 1;
    }

    const char* llvmIRFile = argv[1];
    std::vector<std::string> mainArgs;
    std::string sharedLibPath;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--load=", 0) == 0) {
            sharedLibPath = arg.substr(7);
        } else {
            mainArgs.push_back(arg);
        }
    }

    if (!sharedLibPath.empty()) {
        if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(sharedLibPath.c_str())) {
            errs() << "Failed to load shared library: " << sharedLibPath << "\n";
            return 1;
        } else {
            outs() << "Successfully loaded shared library: " << sharedLibPath << "\n";
        }
    }

    LLVMContext context;
    SMDiagnostic error;
    auto module = parseIRFile(llvmIRFile, error, context);
    if (!module) {
        error.print(argv[0], errs());
        return 1;
    }

    std::string errStr;
    auto memoryManager = std::make_unique<llvm::SectionMemoryManager>();
    ExecutionEngine* engine = EngineBuilder(std::move(module))
                            .setEngineKind(EngineKind::JIT)
                            .setMCJITMemoryManager(std::make_unique<SectionMemoryManager>())
                            .create();

    if (!engine) {
        errs() << "Failed to create ExecutionEngine: " << errStr << "\n";
        return 1;
    }

    Function* func = engine->FindFunctionNamed("main");
    if (!func) {
        errs() << "'main' function not found in module.\n";
        return 1;
    }

    std::vector<GenericValue> args;

    if (!mainArgs.empty()) {
        // argc
        GenericValue gv_argc;
        gv_argc.IntVal = APInt(32, static_cast<uint64_t>(mainArgs.size()));
        args.push_back(gv_argc);

        // argv
        std::vector<void*> argvPointers;
        Module* mod = func->getParent();
        LLVMContext& ctx = mod->getContext();

        for (const auto& s : mainArgs) {
            Constant* strConst = ConstantDataArray::getString(ctx, s, true);
            GlobalVariable* gv = new GlobalVariable(
                *mod,
                strConst->getType(),
                true,
                GlobalValue::PrivateLinkage,
                strConst
            );

            argvPointers.push_back(engine->getPointerToGlobal(gv));
        }

        GenericValue gv_argv;
        gv_argv.PointerVal = argvPointers.data();
        args.push_back(gv_argv);
    }

    GenericValue result = engine->runFunction(func, args);
    outs() << "Result: " << result.IntVal << "\n";

    return 0;
}
