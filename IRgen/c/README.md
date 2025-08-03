# LLVM IR generation for C programs

To convert C source files (.c) into LLVM Intermediate Representation (.ll)  Clang can be used which serves as the official C/C++ frontend for LLVM. Clang translates your C code into Assembly, Object files and LLVM IR (.ll/.bc) enabling JIT execution, IR analysis, obfuscation, and further compilation.
There are two main tools you can use:
- clang.exe: Standard Clang frontend
- clang-cl.exe: MSVC-compatible Clang frontend

clang-cl is an MSVC-compatible frontend for Clang. It mimics cl.exe (Microsoft’s compiler), supporting MSVC flags (`/MT`, `/MD`, `/c`, etc.) and integrates better with Windows build systems and Visual Studio.
Internally, clang-cl translates MSVC-style flags to Clang-compatible backend options and invokes the same LLVM-based compilation pipeline.


## Using clang for IR generation

Here’s a full workflow for generating IR, obfuscating it, and converting it to an executable:

```powershell
## Standard IR gen
IRvana\IRgen\c\src> X:\DELTA\C:\IRvana\LLVM-18.1.5\bin\clang.exe ^
  -I "C:\Program Files (x86)\Windows Kits\10" ^
  -I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.36.32532" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" ^
  -I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.16.27023\include" ^
  -S -emit-llvm -o output.ll main.c

## Obf IR gen
IRvana\IRgen\c\src> X:\DELTA\C:\IRvana\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="X:\DELTA\C:\IRvana\OLLVM\build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" output.ll -o obf.ll

# IR to obj conversion
IRvana\IRgen\c\src> X:\DELTA\C:\IRvana\LLVM-18.1.5\bin\llc.exe -filetype=obj obf.ll -o obf.o

# obj to exe conversion
IRvana\IRgen\c\src> X:\DELTA\C:\IRvana\LLVM-18.1.5\bin\clang.exe obf.o -o obf.exe
```

## Using clang-cl for IR generation

With clang-cl, IR generation is a bit different but more MSVC-friendly. clang-cl automatically invoke clang or clang++ under the hood based on the file extension. You can use `/clang:` to pass Clang-specific flags. Additionally specifying the `/TO` flag forces the compiler to treat all source files as raw C.

```powershell
## Standard IR gen
IRvana\IRgen\c\src> clang-cl ^
  /winsdkdir "C:\Program Files (x86)\Windows Kits\10" ^
  /vctoolsdir "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.36.32532" ^
  /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" ^
  /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" ^
  /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" ^
  /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.16.27023\include" ^
  /c /GS- /MT /clang:-S /clang:-emit-llvm hello.c
```

Using flags such as `/MT` and `/MDd` are crucial in statically or dynamically  linking the Runtime library.

## Example using IRvana for C IR gneration

As an example the following project has been targetted for IR generation: <https://github.com/0xNinjaCyclone/EarlyCascade>

`main.c` from the project was placed in the `IRvana\IRgen\c\src` directory and IR was generated after.

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

>> Enter the programming language: c

------------------------------------------------------------
[Info] Language selected: c
[Note] Make sure your source file(s) are placed in:
        C:\IRvana\IRgen\c\src
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
Language      : c
Clean Build   : Yes
Obfuscation   : Yes
Obf. Mode     : final.ll (after linking)
Obf. Passes   : indbr,icall,indgv,cff,cse
============================================================

[Build] Running: make delete -f Makefile.ir.mk && make ir -f Makefile.ir.mk && make obf_final -f Makefile.ir.mk OBF_PASSES=indbr,icall,indgv,cff,cse
[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
if exist vs_env.mk del /f /q vs_env.mk
if exist ir_bin rd /s /q ir_bin
[Bootstrap] Generating vs_env.mk using detect_sdk.bat...
[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
if not exist ir_bin mkdir ir_bin
[snip]
6 warnings generated.
C:\IRvana\IRgen\c\..\..\LLVM-18.1.5\bin\llvm-link.exe -o ir_bin/final.ll ir_bin/main.ll
[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
C:\IRvana\IRgen\c\..\..\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="C:\IRvana\IRgen\c\..\..\OLLVM\vs_build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" ir_bin/final.ll -o ir_bin/final-obf.ll

[+] IR generated successfully for language: c
[+] IR generated successfully for language: c
[+] Output located in: C:\IRvana\IRgen\c\ir_bin\final-obf.ll

[+] Test using:
    C:\Users\m3rcer\Desktop\C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\c\ir_bin\final-obf.ll

[IR] IRvana achieved [IR]

## Test execution
IRvana\x64\Debug> C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\c\ir_bin\final-obf.ll
[*] Create a process in suspended mode ( Notepad.exe )
[+] The process has been created successfully
[*] Getting a handle on NtDLL
[+] NtDLL Base Address = 0x00007FF9E3150000
[*] Dynamically Search for the Callback Pointer Address ( g_pfnSE_DllLoaded )
[+] Found the Callback Address at 0x00007FF9E32D1268
[*] Dynamically Search for the Enabling Flag Address ( g_ShimsEnabled )
[+] Found the Enabling Flag Address at 0x00007FF9E32BD194
[snip]
```



