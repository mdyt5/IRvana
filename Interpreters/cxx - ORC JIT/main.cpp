/*
Author: @m3rcer
ORCJIT interpreter POC
*/

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <vector>
#include <memory>
#include <string>

using namespace llvm;
using namespace llvm::orc;

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    if (argc < 2) {
        errs() << "Usage:\n";
        errs() << "  " << argv[0] << " <LLVM IR file> [main-args...] [--load=shared-lib]\n\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::vector<std::string> mainArgs;
    std::string sharedLibPath;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.starts_with("--load=")) {
            sharedLibPath = arg.substr(7);
        } else {
            mainArgs.push_back(arg);
        }
    }

    if (!sharedLibPath.empty()) {
        if (sys::DynamicLibrary::LoadLibraryPermanently(sharedLibPath.c_str())) {
            errs() << "Failed to load shared lib: " << sharedLibPath << "\n";
            return 1;
        } else {
            outs() << "Loaded shared lib: " << sharedLibPath << "\n";
        }
    }

    LLVMContext ctx;
    SMDiagnostic err;
    auto mod = parseIRFile(inputFile, err, ctx);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    auto tsm = ThreadSafeModule(std::move(mod), std::make_unique<LLVMContext>());

    auto jitOrErr = LLJITBuilder().create();
    if (!jitOrErr) {
        logAllUnhandledErrors(jitOrErr.takeError(), errs(), "JIT creation failed: ");
        return 1;
    }
    auto jit = std::move(*jitOrErr);

    sys::DynamicLibrary::LoadLibraryPermanently(nullptr); // Load host symbols

    if (auto err = jit->addIRModule(std::move(tsm))) {
        logAllUnhandledErrors(std::move(err), errs(), "AddModule Error: ");
        return 1;
    }

    auto mainSym = jit->lookup("main");
    if (!mainSym) {
        logAllUnhandledErrors(mainSym.takeError(), errs(), "Lookup Error: ");
        return 1;
    }

    using MainFuncType = int (*)(int, char **);
    auto mainFn = mainSym->toPtr<MainFuncType>();  // ✅ FIXED here

    std::vector<const char*> argvPtrs;
    for (const auto& arg : mainArgs)
        argvPtrs.push_back(arg.c_str());

    int result = mainFn(static_cast<int>(argvPtrs.size()), const_cast<char**>(argvPtrs.data()));
    outs() << "Result: " << result << "\n";

    return 0;
}
