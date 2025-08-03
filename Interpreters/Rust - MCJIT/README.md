# Rust-Based LLVM IR Interpreter (via `inkwell` + MCJIT)

MCJIT (Multi-Class Just-In-Time) is an older LLVM JIT compilation engine that, despite being deprecated in favor of ORCJIT, remains functional and effective in specific scenarios, especially within offensive security tooling and research. It provides a relatively straightforward way to execute LLVM IR at runtime without requiring complex infrastructure, making it ideal for building lightweight interpreters or custom loaders. In red team and malware development contexts, MCJIT is particularly useful for executing obfuscated or multi-language IR payloads in-memory, reducing reliance on dropping executables to disk and allowing for greater stealth. While newer LLVM versions emphasize ORCJIT for its modularity and performance, MCJIT continues to serve as a valuable, stable foundation for rapid prototyping and IR-level execution in controlled or legacy-compatible environments.

This Rust program is a custom LLVM IR interpreter using [`inkwell`](https://github.com/TheDan64/inkwell), a high-level Rust wrapper over the LLVM C API. It uses LLVM’s MCJIT backend (via `ExecutionEngine`) to JIT-compile and run `.ll` files similar to `lli --jit-kind=mcjit`.

It also supports:
* Passing CLI args to `main(argc, argv)`
* Dynamically loading shared libraries at runtime (`--load=some.dll`)

This tool mirrors much of `lli`’s behavior, but is fully written in safe(ish) Rust.


## Execution Flow

1. Initialize CLI and Parse Arguments: Basic argument parsing using `std::env::args()`

```rust
rust_mcjit.exe input.ll arg1 arg2 --load=some.dll
```

* First argument is the `.ll` IR file.
* Additional args are passed to the JIT’d `main`.
* `--load=sharedlib` dynamically loads a DLL or `.so` before execution.

2. Load and Parse LLVM IR: The IR is read from file, loaded into a memory buffer, and parsed into an LLVM `Module`. The IR is now loaded and ready for compilation/interpretation.

```rust
let ir_bytes = fs::read(ir_file)?;
let memory_buffer = MemoryBuffer::create_from_memory_range(&ir_bytes, ir_file);
let module = context.create_module_from_ir(memory_buffer)?;
```

3. Create MCJIT Execution Engine: The execution engine is created from the module using Inkwell's high-level API, which wraps LLVM's `MCJIT`. This compiles the entire module ahead-of-time and prepares to invoke functions.

```rust
let execution_engine = module.create_jit_execution_engine(OptimizationLevel::Default)?;
```

4. Load External Shared Libraries (Optional): If the user passes `--load=some.dll`, the tool uses `libloading::Library::new()` on Windows. This enables your LLVM IR to call into native OS-level functions at runtime useful for sandbox evasion or dynamic payload execution.

```rust
#[cfg(unix)]
libc::dlopen(libpath, RTLD_NOW | RTLD_GLOBAL);

#[cfg(windows)]
libloading::Library::new(libpath)?;
```

5. Resolve `main` Function and Prepare Arguments: We dynamically look up the `main` function in the JIT-compiled module:

```rust
execution_engine.get_function::<unsafe extern "C" fn(c_int, *const *const c_char) -> i32>("main")
```

* We then convert CLI args to `CString` and assemble a raw `**argv` pointer array. This allows calling the compiled `main(int argc, char** argv)` from Rust.

```rust
let c_main_args: Vec<CString> = main_args.iter().map(...).collect();
let c_main_ptrs: Vec<*const c_char> = c_main_args.iter().map(...).collect();
```

6. Execute LLVM IR `main()`: We finally invoke the compiled `main()` function with prepared args. The JIT emits machine code, links symbols (including from shared libs), and executes like a native program.

```rust
unsafe {
    let ret = main.call(argc, argv);
    println!("Program exited with: {}", ret);
}
```

## Dependencies

1. Install vcpkg:

   ```powershell
   > git clone https://github.com/microsoft/vcpkg
   .\vcpkg\bootstrap-vcpkg.bat
   ```

2. Install `libxml2`:

   ```powershell
   > .\vcpkg\vcpkg.exe install libxml2:x64-windows
   ```

3. Find `libxml2.lib`:
   After install, you’ll get something like:

   ```powershell
   > C:\vcpkg\installed\x64-windows\lib\libxml2.lib
   ```

4. Rename `libxml2.lib` to `libxml2s.lib`:

   ```powershell
   > move libxml2.lib libxml2s.lib
   ```

5. Cargo.toml Dependencies: Dynamically downloaded during compilation

    ```
    [dependencies]
    inkwell = { version = "0.5.0", features = ["llvm18-0"] }
    libloading = "0.8"  # For Windows shared lib support
    ```

## Build Steps

Ensure the LLVM toolchain path in IRvana project root and vcpkg installations (`libxml2s.lib`) have been added to Environment Variables.

```powershell
set LLVM_SYS_180_PREFIX=C:\IRvana\LLVM-18.1.5
set LIB=C:\vcpkg\installed\x64-windows\lib;%LIB%
```

Perform Build using `rustc 1.81.0-nightly`.

```powershell
## Install dependencies
IRvana\Interpreters\Rust - MCJIT> cargo build --release
```

## Test Execution

Run the compiled binary like so:

```powershell
## Run LLVM IR with optional args and DLLs
IRvana\Interpreters\Rust - MCJIT> .\target\release\rust_mcjit.exe input.ll arg1 arg2 --load=user32.dll
```






