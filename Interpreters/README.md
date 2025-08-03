# LLVM Interpreters

In the context of LLVM-based obfuscation and adversary emulation, interpreters play a vital role in the execution pipeline. LLVM IR as mentioned before can be interpreted in two forms: .ll and .bc. The .ll file is a human-readable textual representation of LLVM IR, while .bc (bitcode) is the binary encoding of that same IR. Both forms carry the same semantics, and the choice between them depends largely on tooling and size constraints. While .ll is great for inspection and static analysis, .bc is more compact and more suitable for automated deployment or embedded payloads.

To interpret either form, the LLVM toolchain natively offers several approaches. One such tool is `lli`, an official LLVM interpreter that executes .ll or .bc files directly. It internally uses either the MCJIT or ORCJIT engine, depending on the LLVM version and build configuration. These JIT engines bridge the gap between static IR and native runtime execution.
- MCJIT (MC-based JIT) is the older of the two and is widely supported across older LLVM versions. It offers relatively straightforward support for static linking and simple IR execution but lacks advanced features like lazy compilation or symbol resolution customization.
- ORCJIT (On-Request Compilation JIT) is the modern replacement for MCJIT. It provides greater flexibility, modular design, and runtime symbol resolution. ORC supports advanced use cases such as dynamic linking of external functions, multiple module execution, and even in-memory IR transformations before execution.

The choice of JIT engine directly affects compatibility with LLVM versions. This versioning nuance is critical when trying to achieve a state of seamless cross-language, cross-version IR execution through singular consinsitent interpreter interfaces. Achieving "IRvana" means ensuring your interpreters are version-matched, your IR is compiled with compatible flags, and your execution engine supports all used features and constructs.

## Interpreter Options

```
# IRvana project root
├── Interpreters/
│   └── lli - ORCJIT           # Official LLVM interpreter using ORC
│   └── cxx - MCJIT            # Custom interpreter in C++ using MCJIT
│   └── cxx - ORCJIT           # Custom interpreter in C++ using ORCJIT
│   └── Rust - ORCJIT          # Custom Rust interpreter using ORCJIT
```

To interpret and execute IR, LLVM's official tool lli is usually used for debugging purposes. Found under `Interpreters/lli - ORCJIT`, this tool supports both .ll and .bc.

Additionally, the IRvana project includes custom interpreters implemented in both C++ and Rust, leveraging either MCJIT or ORCJIT targetted for the LLVM version used by this project. These interpreters offer flexibility in integrating with different host applications, modifying memory behavior, or obfuscating the execution environment itself. 

Custom interpreters can also be written that do not leverage MCJIT or ORCJIT, but instead implement their own mechanisms for IR interpretation. Below are a few notable examples of custom IR interpreters:
- <https://github.com/grievejia/LLVMDynamicTools>
- <https://github.com/softdevteam/bcvm>
- <https://github.com/sampsyo/llvm-ei>

