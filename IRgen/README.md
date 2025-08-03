
# LLVM IR & Streamlining generation with IRvana

This project currently targets `.ll` IR generation, with future updates planned for full `.bc` support.

Whether you're using C, C++, Rust, or Nim, IR can be generated and interpreted by a single runtime, making language-agnostic payload execution possible. However, generating LLVM IR manually especially for multi-file or dependency heavy projects can be tedious. IRvana solves this by streamlining the IR generation process across multiple languages, ensuring consistent toolchains and LLVM versioning. 

All IR generation is targeted and tested against **LLVM 18.1.5**. Other projects used to match LLVM 18.1.5 are listed below:
- OLLVM plugin supporting LLVM 18.1.5: <https://github.com/0xlane/ollvm-rust>
- Nim 1.6.6: <https://nim-lang.org/blog/2022/05/05/version-166-released.html>
- Rust 1.81.0 nightly: <https://blog.rust-lang.org/2024/09/05/Rust-1.81.0/>

Generated IR can be executed using lli or custom interpreters as mentioned in the project. It is also possible to leverage maldev techniqes as mentioned in this project for stealthy IR execution.

After IR generation depending on the source compiled, it may be required to load external libraries not found in the standard DLL load order like `user32.dll` for `MessageBoxAPI`. It is possible to load these dependencies using the `--load` argument in lli and custom interpreters.

If the IR has been compiled using `/MDd` to make it ligter, it is possible to dynamically load the Runtime using the `--extra-acrhive` argument in lli.

```
## JIT executing IR using /MDd
lli --extra-archive="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.40.33807\lib\x64\msvcrtd.lib" final.ll
```

Reference: <https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170>

## Streamlined generation with IRvana

`IRvana.sln` acts as the main project and interactive tool for IR generation. Under the hood, it delegates to custom makefiles (`Makefile.ir.mk`) for each supported language, which handle:
* IR file generation
* Optional obfuscation
* Linking
* Directory discovery (Visual Studio, Windows SDK)

You can use either the interactive CLI binary generated after compiling the `IRvana.sln` (Visual Studio) main project or manually invoke the makefiles directly from the command line.
Each makefile:
* Detects and injects appropriate paths using `detect_sdk.bat` (Visual Studio + Windows SDK)
* Compiles and links to LLVM IR with correct flags for the specified language
* Supports easy extension for additional include paths in other Makefiles

If your project includes external dependencies, you can manually add them under the `include` directive within the respective `Makefile.ir.mk` file.
Additionally these Makefiles leverage the .mk format so that they can further be extended and integrated in other projects with primary Makefiles using the `include` directive as so: `include Makefile.ir.mk`

IRvana integrates with the latest OLLVM passes inspired from the Arkari project (compatible with LLVM 18.1.5) to apply powerful obfuscation layers to your IR output, making analysis and reverse-engineering significantly more difficult.

Supported obfuscation passes include:
* `indbr` – Indirect branch insertion
* `icall` – Indirect call obfuscation
* `indgv` – Indirect global variable access
* `cff` – Control flow flattening
* `cse` – Common subexpression elimination

IRvana currently supports IR generation for the following. For language-specific implementation details and extension guides, refer to the documentation in each language subfolder.
* C
* C++ (without libstd)
* Rust
* Nim

IR generation with IRvana is currently experimental and may fail in certain cases depending on the source code and its dependencies. Adapting and engineering the project to suit your specific needs is both required and recommended. Ongoing development is focused on improving the stability and reliability of IR generation.

### Project compilation

To compile the project open `IRvana.sln` in Visual Studio and compile as Debug or Release.  

```
IRvana.cpp --> Source code for automating IR generation and obfuscation
IRvana.sln --> Primary project for handling IR generation and obfuscation
├── x64/
│   └── Debug/
│       └── IRVana.exe --> Compiled build
```

### Interactive Usage

The User is required to place project source as standalone files to compile in the respective language `src` directory. 

| Folder       | Purpose                                     |
| ------------ | ------------------------------------------- |
| `src/`       | Place source code here (C, C++, Rust, Nim)  |
| `ir_bin/`    | Output folder for generated `.ll` IR files  |
| `makefile.ir.mk` | Makefiles for each language's IR generation |

`IRVana.exe` can then be executed to automate IR generation and obfuscation.

```
IRvana\Debug\x64> .\IRvana.exe

============================================================
         _____  ______ _    _ _______ __   _ _______
           |   |_____/  \  /  |_____| | \  | |_____|
         __|__ |    \_   \/   |     | |  \_| |     |

    Slaying multi-language IR with obfuscation passes to
                      achieve "IRvana"

                                           ~ Author: @m3rcer
============================================================

Automate easy IR generation and obfuscation for direct JIT interpretation on Windows.
Supported languages:
 - c
 - cxx (without RTTI)
 - nim
 - rust
 ```

The resulting IR is output to `ir_bin/` as either:

* `final.ll` — Plain IR output
* `final-obf.ll` — Obfuscated IR output (via OLLVM)

To get a better understanding of the internals with practical examples please reference the follows:
- [LLVM IR generation for C programs](c/README.md#llvm-ir-generation-for-c-programs)
- [LLVM IR generation for C++ programs](cxx/README.md#llvm-ir-generation-for-c-programs)
- [LLVM IR generation for Nim programs](nim/README.md#llvm-ir-generation-for-nim-programs)
- [LLVM IR generation for Rust programs](rust/README.md#llvm-ir-generation-for-rust-programs)

### Manual Makefile Usage

It is optionally possible to perform manual generation by interfacing with respective `Makefile.ir.mk` files as mentioned below.

```powershell
# Generate raw LLVM IR
IRvana\IRgen\lang> make ir -f Makefile.ir.mk

# Obfuscate final linked IR file
IRvana\IRgen\lang> make obf_final OBF_PASSES=indbr,icall,indgv,cff,cse -f Makefile.ir.mk

# Obfuscate each IR file before linking
IRvana\IRgen\lang> make obf_ir OBF_PASSES=indbr,icall,indgv,cff,cse -f Makefile.ir.mk
```

> You can customize `OBF_PASSES` to apply specific OLLVM transformations.











