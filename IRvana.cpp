/*
# ========== LLVM IR generation and obfuscation project ==========
##################################################################
# Author: @m3rcer
*/

#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <filesystem>

// Converts std::string to std::wstring
std::wstring strToWstr(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

// Find project root by walking up the directory tree
std::filesystem::path findProjectRoot(const std::wstring& markerFile) {
    std::filesystem::path current = std::filesystem::path(_wgetcwd(nullptr, 0));
    for (int i = 0; i < 5; ++i) {
        std::filesystem::path candidate = current / markerFile;
        if (std::filesystem::exists(candidate)) {
            return current;
        }
        current = current.parent_path();
    }
    return {};
}

// Prompt user for path if automatic detection fails
std::filesystem::path getProjectRoot() {
    std::filesystem::path root = findProjectRoot(L"IRvana.sln");
    if (!root.empty()) return root;

    std::wcout << L"[Warning] Could not locate project root via IRvana.sln.\n";
    std::wcout << L"[-] Please enter the full path to your project root (where IRvana.sln is located):\n> ";

    std::wstring input;
    std::getline(std::wcin >> std::ws, input);

    std::filesystem::path userPath = input;
    if (!std::filesystem::exists(userPath / "IRvana.sln")) {
        std::wcerr << L"[Error] Provided path does not contain IRvana.sln. Aborting.\n";
        return {};
    }

    return userPath;
}

// Executes a command in a given working directory
bool runCommand(const std::wstring& command, const std::wstring& workingDir) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::wstring cmdLine = L"cmd.exe /C " + command;

    BOOL success = CreateProcessW(
        NULL,
        cmdLine.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        workingDir.c_str(),
        &si,
        &pi
    );

    if (!success) {
        DWORD error = GetLastError();
        std::wcerr << L"[Error] Failed to run: " << cmdLine << L"\nError code: " << error << L"\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

// Main logic for build orchestration
bool runMake(const std::string& language, bool cleanBuild, bool obfuscate, const std::string& obfMode, const std::string& obfPasses) {
    std::filesystem::path baseDir = getProjectRoot();
    if (baseDir.empty()) return false;

    std::filesystem::path makeDir = baseDir / "IRgen" / language;
    std::wstring workingDir = makeDir.wstring();
    std::wstring makefile = L"-f Makefile.ir.mk";

    std::wstring fullCommand;

    if (cleanBuild) {
        fullCommand += L"make delete " + makefile + L" && ";
    }

    if (!obfuscate) {
        fullCommand += L"make ir " + makefile;
    }
    else {
        if (obfMode == "final") {
            fullCommand += L"make ir " + makefile + L" && make obf_final " + makefile;
        }
        else {
            fullCommand += L"make obf_ir " + makefile;
        }
        if (!obfPasses.empty()) {
            fullCommand += L" OBF_PASSES=" + strToWstr(obfPasses);
        }
    }

    std::wcout << L"[Build] Running: " << fullCommand << L"\n";

    return runCommand(fullCommand, workingDir);
}

// Entry point
int main() {
    std::string language;
    bool cleanBuild = false;
    bool applyObfuscation = false;
    std::string obfMode;
    std::string obfPasses;

    // Display Header
    std::cout << R"(
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

)";

    std::cout << ">> Enter the programming language: ";
    std::cin >> language;

    // Validate language
    std::map<std::string, bool> supported = {
        {"c", true}, {"cxx", true}, {"rust", true}, {"nim", true}
    };

    if (!supported[language]) {
        std::cerr << "\n[Error] Unsupported language: " << language << "\n";
        return 1;
    }

    // Instructions
    std::filesystem::path projectRoot = getProjectRoot();
    std::filesystem::path srcDir = projectRoot / "IRgen" / language / "src";

    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "[Info] Language selected: " << language << "\n";
    std::cout << "[Note] Make sure your source file(s) are placed in:\n";
    std::wcout << L"        " << srcDir.wstring() << L"\n";

    std::cout << "[Tip] If your project includes headers or extra src dirs,\n";
    std::cout << "      edit the Makefile (Makefile.ir.mk) to add them (e.g., `include` folder).\n";

    if (language == "rust") {
        std::cout << "\n[Note for Rust]:\n";
        std::cout << " - Ensure your project's Cargo.toml is integrated with the one in:\n";
        std::wcout << L"   " << (srcDir / "Cargo.toml").wstring() << L"\n";
        std::cout << " - This allows dynamic dependency fetching during IR generation.\n";
    }

    // Check if any source files exist in src/
    bool foundSrc = false;
    if (std::filesystem::exists(srcDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(srcDir)) {
            if (entry.is_regular_file()) {
                foundSrc = true;
                break;
            }
        }
    }

    if (!foundSrc) {
        std::cerr << "[!] Warning: No source files found in " << srcDir << "\n";
        std::cerr << "    Make sure to place your source file(s) before continuing.\n";
    }
    std::cout << "------------------------------------------------------------\n\n";


    std::cout << "\n-- Clean Build Directory --\n";
    std::cout << ">> Clean before build? [1 = Yes, 0 = No]: ";
    std::cin >> cleanBuild;

    std::cout << "\n-- Obfuscation Settings --\n";
    std::cout << ">> Apply obfuscation? [1 = Yes, 0 = No]: ";
    std::cin >> applyObfuscation;

    if (applyObfuscation) {
        std::cout << "\n>> Select obfuscation mode:\n";
        std::cout << "   [1] Obfuscate after linking final.ll (obf_final)\n";
        std::cout << "   [2] Obfuscate individual IR files before linking (obf_ir)\n";
        std::cout << ">> Choice: ";

        int modeChoice = 0;
        std::cin >> modeChoice;
        obfMode = (modeChoice == 1) ? "final" : "ir";

        std::map<std::string, std::string> passDescriptions = {
            {"indbr", "Indirect jump, encrypt jump target"},
            {"icall", "Indirect function call, encrypt target function address"},
            {"indgv", "Indirect global variable, encrypt variable address"},
            {"cff",   "Procedure dependent control flow flattening obfuscation"},
            {"cse",   "C string encryption (not effective in Rust)"}
        };

        std::cout << "\n>> Available Obfuscation Passes:\n";
        for (const auto& [key, desc] : passDescriptions) {
            std::cout << "   [" << key << "] - " << desc << "\n";
        }
        std::cout << "   [all] - Apply all passes\n";

        std::cout << "\n>> Enter comma-separated passes (e.g., indbr,icall) or type 'all': ";
        std::cin.ignore(); // Clear input buffer
        std::string input;
        std::getline(std::cin, input);

        // Normalize input
        std::string cleaned;
        for (char c : input) {
            if (!isspace(c)) cleaned += tolower(c);
        }

        if (cleaned == "all") {
            obfPasses = "indbr,icall,indgv,cff,cse";
        }
        else {
            // Validate and clean input
            std::stringstream ss(cleaned);
            std::string token;
            std::vector<std::string> selected;
            std::map<std::string, std::string> passDescriptions = {
                {"indbr", ""}, {"icall", ""}, {"indgv", ""}, {"cff", ""}, {"cse", ""}
            };

            while (std::getline(ss, token, ',')) {
                if (passDescriptions.count(token)) {
                    selected.push_back(token);
                }
                else {
                    std::cerr << "[!] Unknown pass ignored: " << token << "\n";
                }
            }

            if (selected.empty()) {
                std::cerr << "[Error] No valid obfuscation passes selected.\n";
                return 1;
            }

            obfPasses = "";
            for (size_t i = 0; i < selected.size(); ++i) {
                obfPasses += selected[i];
                if (i < selected.size() - 1) obfPasses += ",";
            }
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << " Build Configuration\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Language      : " << language << "\n";
    std::cout << "Clean Build   : " << (cleanBuild ? "Yes" : "No") << "\n";
    std::cout << "Obfuscation   : " << (applyObfuscation ? "Yes" : "No") << "\n";
    if (applyObfuscation) {
        std::cout << "Obf. Mode     : " << (obfMode == "final" ? "final.ll (after linking)" : "IR files (before linking)") << "\n";
        std::cout << "Obf. Passes   : " << obfPasses << "\n";
    }
    std::cout << "============================================================\n\n";

    bool result = runMake(language, cleanBuild, applyObfuscation, obfMode, obfPasses);

    if (!result) {
        std::cerr << "[x] Build failed.\n";
        return 1;
    }

    std::filesystem::path baseDir = getProjectRoot();
    std::filesystem::path irOutputPath = baseDir / "IRgen" / language / "ir_bin" / ("final" + std::string(applyObfuscation ? "-obf.ll" : ".ll"));
    std::wstring fullWinPath = irOutputPath.wstring();

    // Construct path to lli.exe inside LLVM-18.1.5
    std::filesystem::path lliPath = baseDir / "LLVM-18.1.5" / "bin" / "lli.exe";
    std::wstring lliFullPath = lliPath.wstring();

    std::wcout << L"\n[+] IR generated successfully for language: " << strToWstr(language) << L"\n";
    std::wcout << L"[+] Output located in: " << fullWinPath << L"\n";
    std::wcout << L"\n[+] Test using:\n    " << lliFullPath << L" " << fullWinPath << L"\n";
    std::wcout << L"\n[IR] IRvana achieved [IR]\n\n";


    return 0;
}
