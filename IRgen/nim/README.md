# LLVM IR generation for Nim programs

Nim is a compiled systems programming language. Its compiler first parses Nim code into an abstract syntax tree (AST), applies semantic analysis and optimizations, and then translates it into C, C++, or JavaScript, depending on the target. The C/C++ backend is most common and provides broad platform support with mature compiler toolchains.

Nim translates its high-level constructs directly into readable, idiomatic C code using its internal codegen. This approach gives Nim several advantages:
- Leverages the C toolchain (e.g., gcc, clang) for compilation, linking, and optimization.
- Reduces the complexity of writing custom machine code emitters.
- Enables targeting bare metal, embedded systems, and OS-level APIs through generated C.

The generated C code then goes through a standard C compiler, producing native machine code (e.g., PE/ELF binaries or object files), which can eventually be linked into executables, libraries, or injected shellcode

Although Nim doesn't use LLVM directly in its main toolchain, LLVM can be integrated via the C backend for indirect usage. After Nim outputs C, you can compile the C using clang with `-emit-llvm` to produce LLVM IR or bitcode.
In essence, Nim is a high-level frontend that emits C/C++ as an intermediate form, and by hijacking this step, LLVM can be inserted into its toolchain to enable obfuscation, analysis, or in-memory execution.


## Using nim and clang for IR generation

Here’s a full workflow for generating IR, obfuscating it, and converting it to an executable:

nim to C generation

```powershell
## Converting nim to C source files
IRvana\IRgen\nim\src> nim c --genScript --cpu:amd64 --nimcache:. hello.nim

IRvana\IRgen\nim\src> copy ~\nimcache\hello_d .
~\nimcache\hello_d\@mhello.nim.c
~\nimcache\hello_d\compile_hello.bat
~\nimcache\hello_d\hello.json
~\nimcache\hello_d\stdlib_digitsutils.nim.c
~\nimcache\hello_d\stdlib_dollars.nim.c
~\nimcache\hello_d\stdlib_io.nim.c
~\nimcache\hello_d\stdlib_system.nim.c
        7 file(s) copied.
```

IR generation and linking

```powershell
## Converting C generated nim files to IR and linking
C:\IRvana\LLVM-18.1.5\bin\clang.exe -S -emit-llvm -I C:\IRvana\nim-1.6.6\lib -o @mhello.nim.ll @mhello.nim.c

C:\IRvana\LLVM-18.1.5\bin\clang.exe -S -emit-llvm -I C:\IRvana\nim-1.6.6\lib -o stdlib_io.nim.ll stdlib_io.nim.c

C:\IRvana\LLVM-18.1.5\bin\clang.exe -S -emit-llvm -I C:\IRvana\nim-1.6.6\lib -o stdlib_system.nim.ll stdlib_system.nim.c

C:\IRvana\LLVM-18.1.5\bin\clang.exe -S -emit-llvm -I C:\IRvana\nim-1.6.6\lib -o stdlib_dollars.nim.ll stdlib_dollars.nim.c

C:\IRvana\LLVM-18.1.5\bin\clang.exe -S -emit-llvm -I C:\IRvana\nim-1.6.6\lib -o stdlib_digitsutils.nim.ll stdlib_digitsutils.nim.c

C:\IRvana\LLVM-18.1.5\bin\llvm-link.exe -o combined.ll @mhello.nim.ll stdlib_io.nim.ll stdlib_system.nim.ll stdlib_dollars.nim.ll stdlib_digitsutils.nim.ll
```

Obfuscation and object conversion

```powershell
## Obf IR gen
C:\IRvana\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="C:\IRvana\OLLVM\build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" combined.ll -o obf.ll

# IR to obj conversion
C:\IRvana\LLVM-18.1.5\bin\llc.exe -filetype=obj obf.ll -o obf.o

# obj to exe conversion
C:\IRvana\LLVM-18.1.5\bin\clang.exe obf.o -o obf.exe
```

## Using clang-cl for IR generation

With clang-cl, IR generation is a bit different but more MSVC-friendly. clang-cl automatically invoke clang or clang++ under the hood based on the file extension. You can use `/clang:` to pass Clang-specific flags. 

However, during my experiments a few libraries were still found missing from the final IR linking phase with generation using clang-cl. 

## Example using IRvana for c++ IR gneration

As an example the following project has been targetted for IR generation: <https://github.com/byt3bl33d3r/OffensiveNim/blob/master/src/amsi_providerpatch_bin.nim>

`amsi_providerpatch_bin.nim` from the project was placed in the `IRvana\IRgen\nim\src` directory and IR was generated after.

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

>> Enter the programming language: nim

------------------------------------------------------------
[Info] Language selected: nim
[Note] Make sure your source file(s) are placed in:
        C:\IRvana\IRgen\nim\src
[Tip] If your project includes headers or extra src dirs,
      edit the Makefile (Makefile.ir.mk) to add them (e.g., `include` folder).
------------------------------------------------------------


-- Clean Build Directory --
>> Clean before build? [1 = Yes, 0 = No]: 1

-- Obfuscation Settings --
>> Apply obfuscation? [1 = Yes, 0 = No]: 1

>> Select obfuscation mode:
   [1] Obfuscate after linking final.ll (obf_final)
   [2] Obfuscate individual IR files before linking (obf_ir)
>> Choice: 1

>> Available Obfuscation Passes:
   [cff] - Procedure dependent control flow flattening obfuscation
   [cse] - C string encryption (not effective in Rust)
   [icall] - Indirect function call, encrypt target function address
   [indbr] - Indirect jump, encrypt jump target
   [indgv] - Indirect global variable, encrypt variable address
   [all] - Apply all passes

>> Enter comma-separated passes (e.g., indbr,icall) or type 'all': all

============================================================
 Build Configuration
------------------------------------------------------------
Language      : nim
Clean Build   : Yes
Obfuscation   : Yes
Obf. Mode     : final.ll (after linking)
Obf. Passes   : indbr,icall,indgv,cff,cse
============================================================

[Build] Running: make delete -f Makefile.ir.mk && make ir -f Makefile.ir.mk && make obf_final -f Makefile.ir.mk OBF_PASSES=indbr,icall,indgv,cff,cse

[snip]

[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
Obfuscating:
C:\IRvana\IRgen\nim\..\..\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="C:\IRvana\IRgen\nim\..\..\OLLVM\vs_build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" ir_bin/final.ll -o ir_bin/final-obf.ll

[+] IR generated successfully for language: nim
[+] Output located in: C:\IRvana\IRgen\nim\ir_bin\final-obf.ll

[+] Test using:
    C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\nim\ir_bin\final-obf.ll

[IR] IRvana achieved [IR]

## Test execution
C:\IRvana\x64\Debug> C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\nim\ir_bin\final-obf.ll
[*] Running in x64 process
[*] Applying patch
[*] AMSI disabled: true
```
