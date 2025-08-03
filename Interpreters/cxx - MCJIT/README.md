# C++ MCJIT Interpreter

MCJIT (Multi-Class Just-In-Time) is an older LLVM JIT compilation engine that, despite being deprecated in favor of ORCJIT, remains functional and effective in specific scenarios, especially within offensive security tooling and research. It provides a relatively straightforward way to execute LLVM IR at runtime without requiring complex infrastructure, making it ideal for building lightweight interpreters or custom loaders. In red team and malware development contexts, MCJIT is particularly useful for executing obfuscated or multi-language IR payloads in-memory, reducing reliance on dropping executables to disk and allowing for greater stealth. While newer LLVM versions emphasize ORCJIT for its modularity and performance, MCJIT continues to serve as a valuable, stable foundation for rapid prototyping and IR-level execution in controlled or legacy-compatible environments.

The MCJIT interpreter in this example uses LLVM’s `ExecutionEngine` API, a legacy interface for just-in-time execution of IR, to initialize the target, parse the LLVM IR, and execute functions with runtime arguments.

## MCJIT execution flow

The MCJIT Interpreter works as follows:

1. Initialize LLVM targets: Register the native architecture and assembler support

```c++
LLVMInitializeNativeTarget();
LLVMInitializeNativeAsmPrinter();
LLVMInitializeNativeAsmParser();
```

2. Parse the LLVM IR file: Load .ll IR into a Module and uses MCJIT to initialize the LLVM engine builder with your LLVM IR module (IR to JIT and execute).

```c++
auto module = parseIRFile(llvmIRFile, error, context);

Create the Execution Engine
Create an instance of the MCJIT ExecutionEngine:

ExecutionEngine* engine = EngineBuilder(std::move(module))
    .setEngineKind(EngineKind::JIT)
    .setMCJITMemoryManager(std::make_unique<SectionMemoryManager>())
    .create();
```

3. Load optional shared libraries: Allow for importing DLLs required during runtime execution. (like user32.dll)

```c++
llvm::sys::DynamicLibrary::LoadLibraryPermanently(sharedLibPath.c_str());
```

4. Prepare main() arguments: Convert command-line inputs to LLVM GenericValue form to pass to the main function and execute main()

```c++
GenericValue result = engine->runFunction(func, args);
```

## Build Steps

Use cmake to build a Visual Studio project, and make sure to always compile the `Release` version.

```powershell
## Make project Build
IRvana\Interpreters\cxx - MCJIT> mkdir build
IRvana\Interpreters\cxx - MCJIT> cd build
IRvana\Interpreters\cxx - MCJIT\build> cmake -G "Visual Studio 17 2022" -A x64 ..

## Compile
IRvana\Interpreters\cxx - MCJIT\build> cmake --build . --config Release
```

## Test Execution

If you compile and run the binary like so:

```powershell
## Standard execution
IRvana\Interpreters\cxx - MCJIT\build\Release> MCJITExec.exe final.ll arg1 arg2

## Load libraries not included statically (Ex: MessageBox)
IRvana\Interpreters\cxx - MCJIT\build\Release> MCJITExec.exe final.ll --load=user32.dll
```
