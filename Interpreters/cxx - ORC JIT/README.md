# C++ ORCJIT Interpreter

ORCJIT (On Request Compilation JIT) is the modern and actively developed JIT engine in LLVM, designed to replace MCJIT with a more modular, flexible, and performance-oriented architecture. Unlike MCJIT, which compiles entire modules eagerly, ORCJIT supports lazy compilation, symbol resolution customization, and finer-grained control over memory layout and linking, making it better suited for complex runtime environments. In the context of offensive security and malware development, ORCJIT opens up possibilities for crafting advanced in-memory loaders, dynamic language interpreters, and runtime code morphing techniques. Its capability to handle symbol resolution across multiple layers enables more sophisticated execution models, such as staged payloads or on-the-fly IR mutation. Though it comes with a steeper learning curve, ORCJIT is the go-to choice for modern LLVM-based interpreters and obfuscation-aware tooling where adaptability, modularity, and performance are critical.

The ORCJIT interpreter in this example uses LLVM’s `LLJIT` API, a high-level wrapper over the lower-level ORC layers, to simplify module management and function execution. The following ORCJIT implementation offers better flexibility and cleaner code for modern LLVM-based tools, payloads, and emulators.

## ORCJIT Execution Flow

Here's an explanation for the ORCJIT implementation in `main.cpp`.

1. Initialize LLVM targets: Registers the current architecture and assembler so LLVM can generate and execute native machine code.

```cpp
InitializeNativeTarget();
InitializeNativeTargetAsmPrinter();
InitializeNativeTargetAsmParser();
```

---

2. Parse the LLVM IR file: Reads the input `.ll` file and loads it into an LLVM `Module`. 

```cpp
auto mod = parseIRFile(inputFile, err, ctx);
```

3. The parsed module is wrapped in a `ThreadSafeModule`, which allows thread-safe execution and compilation.

```cpp
auto tsm = ThreadSafeModule(std::move(mod), std::make_unique<LLVMContext>());
```

---

4. Create the ORCJIT instance: The `LLJITBuilder` is used to create a high-level ORC JIT engine. If creation fails, error handling is used to show diagnostics.

```cpp
auto jitOrErr = LLJITBuilder().create();
auto jit = std::move(*jitOrErr);
```

---

5. Load shared libraries (optional): If specified, external DLLs can be dynamically loaded. This allows the IR to call into system libraries or dynamically linked modules at runtime.

```cpp
sys::DynamicLibrary::LoadLibraryPermanently(sharedLibPath.c_str());
```

6. Add IR module to the JIT: The module is added to the JIT, enabling symbol resolution and JIT compilation.

```cpp
jit->addIRModule(std::move(tsm));
```

7. Lookup and execute `main`: The symbol `main` is resolved from the module, and converted to a native function pointer.

```cpp
auto mainSym = jit->lookup("main");
auto mainFn = mainSym->toPtr<MainFuncType>();
```

8. Command-line arguments are converted into a format suitable for passing to the IR-defined `main()`:

```cpp
int result = mainFn(argc, argv);
```


## Build Steps

Use `cmake` to generate and build the Visual Studio project:

```powershell
## Setup project
IRvana\Interpreters\cxx - ORCJIT> mkdir build
IRvana\Interpreters\cxx - ORCJIT> cd build
IRvana\Interpreters\cxx - ORCJIT\build> cmake -G "Visual Studio 17 2022" -A x64 ..

## Compile (Release only)
IRvana\Interpreters\cxx - ORCJIT\build> cmake --build . --config Release
```



## Test Execution

Run the compiled binary like so:

```powershell
## Standard execution
IRvana\Interpreters\cxx - ORCJIT\build\Release> ORCJITExec.exe final.ll arg1 arg2

## Load DLLs (Ex: user32.dll for MessageBoxA)
IRvana\Interpreters\cxx - ORCJIT\build\Release> ORCJITExec.exe final.ll arg1 arg2 --load=user32.dll
```
