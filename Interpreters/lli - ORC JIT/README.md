# LLVM `lli` Tool (Part of LLVM toolchain) 

The `lli` tool is LLVM's official LLVM IR interpreter / JIT compiler launcher. It provides a CLI interface to execute `.bc` (bitcode) or `.ll` (IR) files directly, without needing to compile them to native binaries. Due to its potential signed nature and presence in official LLVM toolchains, it may serve as a LOLBAS and can already be present and trusted in many developer based environments, enabling low-friction trusted interpreter payload execution.

Internally, `lli` supports multiple execution engines, including:
* Interpreter  (pure IR execution)
* MCJIT  (legacy JIT compiler)
* ORCJIT / ORC Lazy JIT (modern modular JIT framework)

## lli Execution Flow

1. Initialize LLVM Targets: 
This enables both MCJIT and ORCJIT to compile LLVM IR to native machine code.
`lli` sets up native target support for the host platform. 

```cpp
InitializeNativeTarget();
InitializeNativeTargetAsmPrinter();
InitializeNativeTargetAsmParser();
```

2. Parse Command-Line Arguments: The tool provides many flags as mentioned below and these flags are handled using `llvm::cl::opt` and `cl::list`.
* `--jit-kind=[mcjit|orc|orc-lazy]` – Choose the JIT engine.
* `--force-interpreter` – Bypass JIT and run in interpreter mode.
* `--load | --dlopen` – Load DLLs/shared objects at runtime.
* `--extra-module`, `--extra-object` – Load additional IR/object files.
* `--entry-function=main` – Customize entry point.

3. Parse IR or Bitcode: Reads the `.ll` or `.bc` file using LLVM's IRReader. The parsed `Module` is passed to the selected execution engine.

```cpp
auto mod = parseIRFile(InputFile, err, ctx);
```

4. Select Execution Engine: Based on CLI options, one of the following is chosen:

🔹 MCJIT: compiles the entire module ahead-of-time and runs the native code.

```cpp
EngineBuilder(std::move(module))
  .setEngineKind(EngineKind::JIT)
  .setMCJITMemoryManager(std::make_unique<SectionMemoryManager>())
  .create();
```

🔹 ORCJIT: ORCJIT enables on-demand, per-function compilation and supports lazy linking and more modular architecture.

```cpp
auto jit = LLJITBuilder().create();
jit->addIRModule(std::move(tsm));
```

🔹Interpreter: The interpreter emulates IR line-by-line. It’s slower, but useful where no JIT backend is available.
 

```cpp
ExecutionEngine *EE = Interpreter::create(std::move(module));
```

5. Load External Shared Libraries (Optional): Allows IR code to call into OS-level functions (e.g., `MessageBoxA` in `user32.dll`)

```cpp
sys::DynamicLibrary::LoadLibraryPermanently("user32.dll");
```

6. Find Entry Point and Execute: The chosen execution engine looks up the user-specified entry point (default `main`) and executes it. For JIT engines, this involves resolving symbols and emitting native code just-in-time.

```cpp
Function *EntryFn = mod->getFunction("main");
GenericValue Result = EE->runFunction(EntryFn, Args);
```

## Build Steps

Use `cmake` to generate and build the Visual Studio project:

```sh
## Setup project
IRvana\Interpreters\lli - ORCJIT> mkdir build
IRvana\Interpreters\lli - ORCJIT> cd build
IRvana\Interpreters\lli - ORCJIT\build> cmake -G "Visual Studio 17 2022" -A x64 ..

## Compile (Release only)
IRvana\Interpreters\lli - ORCJIT\build> cmake --build . --config Release
```


## Test Execution

Run the compiled binary like so:

```powershell
## Standard execution
IRvana\Interpreters\lli - ORCJIT\build\Release> lli.exe final.ll arg1 arg2

## Load DLLs (Ex: user32.dll for MessageBoxA)
IRvana\Interpreters\lli - ORCJIT\build\Release> lli.exe final.ll arg1 arg2 --load=user32.dll
```


