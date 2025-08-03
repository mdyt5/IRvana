/*
Author: @m3rcer
ORCJIT AES Encryption interpreter POC
*/

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOGDI
#define NOUSER
#define NOSERVICE
#define NOHELP

#include <windows.h>
#include <wincrypt.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>

#include <vector>
#include <string>
#include <memory>
#include <iostream>

using namespace llvm;
using namespace llvm::orc;

// Replace with your actual encrypted LLVM IR blob
char AESkey[] = { 0xcc, 0xcc };
unsigned char AESIR[] = { 0xcc, 0xcc };
unsigned int AESIR_len = sizeof(AESIR);


// AES Decrypt buffer
std::vector<char> DecryptAESBuffer(const BYTE* encrypted, DWORD len, const char* key, DWORD keyLen) {
    std::vector<char> decrypted(encrypted, encrypted + len);

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;

    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return {};
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) return {};
    if (!CryptHashData(hHash, (BYTE*)key, keyLen, 0)) return {};
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) return {};

    DWORD decLen = len;
    if (!CryptDecrypt(hKey, NULL, TRUE, 0, (BYTE*)decrypted.data(), &decLen)) return {};
    decrypted.resize(decLen);

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return decrypted;
}

int main(int argc, char** argv) {
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    std::vector<std::string> mainArgs;
    std::string sharedLibPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--load=", 0) == 0) {
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

    // Decrypt IR
    std::vector<char> decrypted = DecryptAESBuffer(AESIR, AESIR_len, AESkey, sizeof(AESkey));
    if (decrypted.empty()) {
        errs() << "AES decryption failed\n";
        return 1;
    }

    // Parse decrypted IR from memory
    auto ctx = std::make_unique<LLVMContext>();
    SMDiagnostic err;
    auto memBuf = MemoryBuffer::getMemBufferCopy(std::string(decrypted.data(), decrypted.size()), "<in-mem-ir>");
    auto mod = parseIR(memBuf->getMemBufferRef(), err, *ctx);
    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    auto tsm = ThreadSafeModule(std::move(mod), std::move(ctx));

    auto jitOrErr = LLJITBuilder().create();
    if (!jitOrErr) {
        logAllUnhandledErrors(jitOrErr.takeError(), errs(), "JIT creation failed: ");
        return 1;
    }
    auto jit = std::move(*jitOrErr);

    sys::DynamicLibrary::LoadLibraryPermanently(nullptr); // Load host process symbols

    if (auto err = jit->addIRModule(std::move(tsm))) {
        logAllUnhandledErrors(std::move(err), errs(), "AddModule Error: ");
        return 1;
    }

    // Lookup and run main
    auto mainSym = jit->lookup("main");
    if (!mainSym) {
        logAllUnhandledErrors(mainSym.takeError(), errs(), "Lookup Error: ");
        return 1;
    }

    using MainFuncType = int (*)(int, char**);
    auto mainFn = mainSym->toPtr<MainFuncType>();

    std::vector<const char*> argvPtrs;
    for (const auto& arg : mainArgs)
        argvPtrs.push_back(arg.c_str());

    int result = mainFn(static_cast<int>(argvPtrs.size()), const_cast<char**>(argvPtrs.data()));
    outs() << "Result: " << result << "\n";

    return 0;
}
