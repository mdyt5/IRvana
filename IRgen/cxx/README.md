# LLVM IR generation for C++ programs

To convert C++ source files (.cpp) into LLVM Intermediate Representation (.ll)  Clang++ can be used which serves as the official C++ frontend for LLVM. Clang++ translates your cpp code into Assembly, Object files and LLVM IR (.ll/.bc) enabling JIT execution, IR analysis, obfuscation, and further compilation.
There are two main tools you can use:
- clang++.exe: Standard Clang frontend
- clang-cl.exe: MSVC-compatible Clang frontend

clang-cl.exe is an MSVC-compatible frontend for Clang. It mimics `cl.exe` (Microsoft’s compiler), supporting MSVC flags (`/MT`, `/MD`, `/c`, etc.) and integrates better with Windows build systems and Visual Studio.
Internally, clang-cl translates MSVC-style flags to Clang-compatible backend options and invokes the same LLVM-based compilation pipeline.
clang can also be used to compile C++ programs but dosen't handle libc++ integration and other C++ features the way clang++ does, hence it is advised to use clang++ for all cpp IR generation.

At this stage, it's recommended to avoid compiling C++ programs that rely on Run-Time Type Information (RTTI) or standard C++ library features such as std::cout, std::type_info, or exceptions and replace them with C counterparts. These features require linking against libc++ and libc++abi, which are not yet fully supported for dynamic loading or integration in this pipeline and is currently being researched.

It’s also important to note that the Makefile defaults to `/std:c++17` for C++ builds. You can adjust this flag depending on the required C++ standard or compatibility with external dependencies in the Makefile.

Additionally, if your C++ source files use extensions other than .cpp (such as .cc or .cxx), make sure to either rename them to .cpp or update the build scripts (e.g., the Makefile) accordingly to ensure they are properly processed.


## Using clang++ for IR generation

Here’s a full workflow for generating IR, obfuscating it, and converting it to an executable:

```powershell
## Standard IR gen
IRvana\IRgen\cxx\src> X:\DELTA\IRvana\LLVM-18.1.5\bin\clang++.exe ^
  -I "C:\Program Files (x86)\Windows Kits\10" ^
  -I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.36.32532" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" ^
  -I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" ^
  -I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.16.27023\include" ^
  -S -emit-llvm -o output.ll main.c

## Obf IR gen
IRvana\IRgen\cxx\src> X:\DELTA\IRvana\LLVM-18.1.5\bin\opt.exe -load-pass-plugin="X:\DELTA\IRvana\OLLVM\build\obfuscation\Release\LLVMObfuscationx.dll" --passes="irobf(irobf-indbr,irobf-icall,irobf-indgv,irobf-cff,irobf-cse)" output.ll -o obf.ll

# IR to obj conversion
IRvana\IRgen\cxx\src> X:\DELTA\IRvana\LLVM-18.1.5\bin\llc.exe -filetype=obj obf.ll -o obf.o

# obj to exe conversion
IRvana\IRgen\cxx\src> X:\DELTA\IRvana\LLVM-18.1.5\bin\clang++.exe obf.o -o obf.exe
```

## Using clang-cl for IR generation

With clang-cl, IR generation is a bit different but more MSVC-friendly. clang-cl automatically invoke clang or clang++ under the hood based on the file extension. You can use `/clang:` to pass Clang-specific flags. Additionally specifying the `/TP` flag forces the compiler to treat all source files as raw C.

However, during my experiments a few libraries were still found missing from the final IR linking phase with generation using clang-cl. 

## Example using IRvana for c++ IR gneration

As an example the following project has been targetted for IR generation: <https://github.com/lsecqt/OffensiveCpp/blob/main/Shellcode%20Execution/CreateThreadPoolWait/>

`CreateThreadPoolWait.cpp` from the project was placed in the `IRvana\IRgen\cxx\src` directory and IR was generated after.

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

>> Enter the programming language: cxx

------------------------------------------------------------
[Info] Language selected: cxx
[Note] Make sure your source file(s) are placed in:
        C:\IRvana\IRgen\cxx\src
[Tip] If your project includes headers or extra src dirs,
      edit the Makefile (Makefile.ir.mk) to add them (e.g., `include` folder).
------------------------------------------------------------


-- Clean Build Directory --
>> Clean before build? [1 = Yes, 0 = No]: 1

-- Obfuscation Settings --
>> Apply obfuscation? [1 = Yes, 0 = No]: 0

============================================================
 Build Configuration
------------------------------------------------------------
Language      : cxx
Clean Build   : Yes
Obfuscation   : No
============================================================

[Build] Running: make delete -f Makefile.ir.mk && make ir -f Makefile.ir.mk
[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
if exist vs_env.mk del /f /q vs_env.mk
if exist ir_bin rd /s /q ir_bin
Makefile.ir.mk:16: vs_env.mk: No such file or directory
Makefile.ir.mk:59: [Warning] winsdkdir is not set
Makefile.ir.mk:65: [Warning] vctoolsdir is not set
Makefile.ir.mk:71: [Warning] sdkver is not set
[Bootstrap] Generating vs_env.mk using detect_sdk.bat...
[Debug] winsdkdir   = C:\\Program Files (x86)\\Windows Kits\\10\\
[Debug] vctoolsdir  = C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207
[Debug] sdkver  = 10.0.26100.0
if not exist ir_bin mkdir ir_bin
C:\IRvana\IRgen\cxx\..\..\LLVM-18.1.5\bin\clang++.exe -I "C:\\Program Files (x86)\\Windows Kits\\10\\" -I "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207" -I include -c -O2 -std=c++17 -S -emit-llvm -Wall -Wno-unused-command-line-argument -nostdlib -Llib -lc++ -lc++abi -lmsvcrt -I "C:\\Program Files (x86)\\Windows Kits\\10\\/Include/10.0.26100.0/ucrt" -I "C:\\Program Files (x86)\\Windows Kits\\10\\/Include/10.0.26100.0/um" -I "C:\\Program Files (x86)\\Windows Kits\\10\\/Include/10.0.26100.0/shared" -I "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207/include" -I "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207/lib" -o ir_bin/CreateThreadPool.ll src/CreateThreadPool.cpp
src/CreateThreadPool.cpp:50:16: warning: format specifies type 'int' but the argument has type 'DWORD'
      (aka 'unsigned long') [-Wformat]
   50 |                 printf("%d", GetLastError());
      |                         ~~   ^~~~~~~~~~~~~~
      |                         %lu
1 warning generated.
C:\IRvana\IRgen\cxx\..\..\LLVM-18.1.5\bin\llvm-link.exe -o ir_bin/final.ll ir_bin/CreateThreadPool.ll

[+] IR generated successfully for language: cxx
[+] Output located in: C:\IRvana\IRgen\cxx\ir_bin\final.ll

[+] Test using:
    C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\cxx\ir_bin\final.ll

[IR] IRvana achieved [IR]

## Test execution
IRvana\x64\Debug> C:\IRvana\LLVM-18.1.5\bin\lli.exe C:\IRvana\IRgen\cxx\ir_bin\final.ll
```



