# LLVM IR generation for Rust programs

Rust uses LLVM natively as its backend. The Rust compiler (rustc) compiles source code through the following phases:
- Parsing and type-checking to build the HIR/MIR (High/Mid-level IR).
- Monomorphization for generics.
- LLVM IR generation.
- LLVM then optimizes and emits machine code.

Rust does not require an intermediate C representation (like Nim). It emits LLVM IR directly which makes it tightly coupled with LLVM and allows leveraging LLVM tooling such as opt, llc, and custom JITs.
Because Rust outputs LLVM IR directly, it integrates seamlessly with LLVM-based workflows. You can emit IR using `--emit=llvm-ir`.

Rust-generated IR often depends on the Rust standard library (libstd), especially when using higher-level features or formatting APIs. To interpret Rust IR:
- You must load the corresponding `std-<hash>.dll` during interpretation (Precompiled example can be found in `src`).
- Found at: `C:\Users\<you>\.rustup\toolchains\<toolchain>\lib\rustlib\x86_64-pc-windows-msvc\lib\std-*.dll`


## Using rust for IR generation

Here's a full workflow for generating IR, obfuscating it, and converting it to an executable:

```powershell
# Generate LLVM IR from a Cargo project
IRvana\IRgen\rust> cargo +nightly-2024-06-26 rustc --target x86_64-pc-windows-msvc --release -- --emit=llvm-ir

# Obfuscate the IR with custom passes
IRvana\IRgen\rust> C:\IRvana\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="C:\IRvana\OLLVM\build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" rust_example.ll -o rust_example-obf.ll

# Compile obfuscated IR to object file
IRvana\IRgen\rust> C:\IRvana\LLVM-18.1.5\bin\llc.exe -filetype=obj obf.ll -o obf.o

# Link object file to executable
IRvana\IRgen\rust> C:\IRvana\LLVM-18.1.5\bin\clang.exe obf.o -o obf.exe

# Direct exe generation with obfuscation
IRvana\IRgen\rust> cargo +nightly rustc --target x86_64-pc-windows-msvc --release -- -Zllvm-plugins="C:\IRvana\OLLVM\build\obfuscation\Release\LLVMObfuscationx.dll" -Cpasses="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)"
```

## Example using IRvana for Rust IR gneration

IRvana can automate this pipeline. Place Rust code inside the Rust project structure (with valid `Cargo.toml`), and use `Makefile.ir.mk` to Generate IR, Apply obfuscation and link the final executable.

Example project tested: <https://github.com/Whitecat18/Rust-for-Malware-Development/tree/main/Process-Injection/inject_on_localprocess>

Add `Cargo.toml` from the target project onto `IRVana\IRgen\rust`. 

```
[package]
name = "inject_on_localprocess"
version = "0.1.0"
edition = "2021"

[dependencies]
winapi = { version = "0.3.9", features = [
    "processthreadsapi",
    "memoryapi",
    "errhandlingapi",
    "synchapi",
    "handleapi",
    "winbase",
    "debugapi"
]}
```

Next `main.rs` was copied and placed in `IRVana\IRgen\rust\src` and IR was generated after.

```
## IR generation with IRvana
IRvana\x64\Debug> .\IRvana.exe

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
 - cxx (without libstd)
 - nim
 - rust

>> Enter the programming language: rust

------------------------------------------------------------
[Info] Language selected: rust
[Note] Make sure your source file(s) are placed in:
        C:\IRvana\IRgen\rust\src
[Tip] If your project includes headers or extra src dirs,
      edit the Makefile (Makefile.ir.mk) to add them (e.g., `include` folder).

[Note for Rust]:
 - Ensure your project's Cargo.toml is integrated with the one in:
   C:\IRvana\IRgen\rust\src\Cargo.toml
 - This allows dynamic dependency fetching during IR generation.
------------------------------------------------------------


-- Clean Build Directory --
>> Clean before build? [1 = Yes, 0 = No]: 1

-- Obfuscation Settings --
>> Apply obfuscation? [1 = Yes, 0 = No]: 0

============================================================
 Build Configuration
------------------------------------------------------------
Language      : rust
Clean Build   : Yes
Obfuscation   : No
============================================================

[Build] Running: make delete -f Makefile.ir.mk && make ir -f Makefile.ir.mk

[vcvarsall.bat] Environment initialized for: 'x64'
    Updating crates.io index
     Locking 4 packages to latest compatible versions
      Adding inject_on_localprocess v0.1.0 (C:\IRvana\IRgen\rust)
      Adding winapi v0.3.9
      Adding winapi-i686-pc-windows-gnu v0.4.0
      Adding winapi-x86_64-pc-windows-gnu v0.4.0
][snip]
Linking all IR files in ir_bin...

FLARE-VM 02-08-2025 16:13:34.94
C:\IRvana\IRgen\rust>(if /I not main.ll == final.ll (echo Found: ir_bin\main.ll   && set FILES=!FILES! ir_bin\main.ll              ) )  && echo Running llvm-link on: !FILES!   && C:\IRvana\IRgen\rust\..\..\LLVM-18.1.5\bin\llvm-link.exe -o ir_bin\final.ll !FILES!
Found: ir_bin\main.ll
Running llvm-link on:   ir_bin\main.ll

[+] IR generated successfully for language: rust
[+] Output located in: C:\IRvana\IRgen\rust\ir_bin\final.ll

[+] Test using:
    C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\rust\ir_bin\final.ll

[IR] IRvana achieved [IR]

## Test execution by loading user32.dll and libstd for rust
C:\IRvana\x64\Debug> C:\IRvana\LLVM-18.1.5\bin\lli.exe --load=C:\Windows\System32\user32.dll --load=C:\IRvana\IRgen\rust\std-cef76c2685dfb4ca.dll C:\IRvana\IRgen\rust\ir_bin\final.ll
| Executing SC in LocalProcess
\____[+] VirtualAlloc : 0x1a2ea6e0000
\____[+] VirtualProtect : 1
\____[+] Executing Payload --->
```