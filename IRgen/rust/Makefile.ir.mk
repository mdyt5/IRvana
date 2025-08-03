#############################################################
## === LLVM IR Generation and Obfuscation for Rust ===
#############################################################
# Author: @m3rcer
# ========== Configuration ==========

.ONESHELL:
SHELL = cmd.exe
.SHELLFLAGS = /V:ON /C

# Bootstrap the environment
vs_env.mk:
	@echo [Bootstrap] Generating vs_env.mk using detect_sdk.bat...
	@cmd /c detect_sdk.bat

include vs_env.mk

# ========== Configuration ==========

IR_BIN_DIR = ir_bin
IR_LL_FILES = $(wildcard $(IR_BIN_DIR)\*.ll)
RUST_SRC_FILE = src\main.rs
#RUST_SRC_DIR = src
RUST_BASENAME = $(basename $(notdir $(RUST_SRC_FILE)))
RUST_LL_FILE = $(IR_BIN_DIR)\$(RUST_BASENAME).ll
RUST_OBF_LL_FILE = $(IR_BIN_DIR)\$(RUST_BASENAME)-obf.ll

# Parsing special chars for obfuscation
comma := ,
space :=
space +=

# Tools
# Get the absolute path to this Makefile
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR  := $(dir $(MAKEFILE_PATH))
# Normalize slashes (for Windows)
MAKEFILE_DIR_W := $(subst /,\,$(MAKEFILE_DIR))
# Dynamically find IRVANA_ROOT as 2 levels up from current directory
IRVANA_ROOT := $(MAKEFILE_DIR_W)..\..
# Set LLVM directory inside IRVANA_ROOT
LLVM_DIR      := $(IRVANA_ROOT)\LLVM-18.1.5
LLVM_OPT      := $(LLVM_DIR)\bin\opt.exe
LLVM_LINK      := $(LLVM_DIR)\bin\llvm-link.exe
# OLLVM Plugin path
OLLVM_PLUGIN ?= $(IRVANA_ROOT)\OLLVM\vs_build\obfuscation\Release\LLVMObfuscationx.dll
CARGO = cargo +nightly-2024-06-26 rustc

# Location of your VC and SDK lib paths (adjust if different)
VC_LIB_PATH = "$(vctoolsdir)\\include\\lib\\x64"
SDK_LIB_PATH = "$(winsdkdir)\\Include\\$(sdkver)\\um\\x64"
UNIVERSAL_LIBS = $(VC_LIB_PATH);$(SDK_LIB_PATH)

# Obfuscation passes
OBF_PASSES ?=
ifeq ($(strip $(OBF_PASSES)),)
  OBF_PASS_OPT :=
else
  OBF_PASS_EXPANDED := $(foreach pass,$(subst $(comma), ,$(OBF_PASSES)),irobf-$(pass))
  OBF_PASS_OPT := --passes="irobf($(subst $(space),$(comma),$(strip $(OBF_PASS_EXPANDED))))"
endif

# Common LLVM IR Generation Flags
COMMON_CFLAGS:=--release -- --emit=llvm-ir

# ========== Targets ==========

.PHONY: ir obf_ir ir_setup rust_to_ir clean delete

ir_setup:
	mkdir $(IR_BIN_DIR)

rust_to_ir:
	@echo Generating LLVM IR from Rust using Cargo...
	@"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && \
	cargo +nightly-2024-06-26 rustc --release -- --emit=llvm-ir
	@echo Copying messagebox LLVM IR to ir_bin...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		for %%f in (target\release\deps\*.ll) do ( \
			echo Found IR file: %%f && \
			copy /Y %%f $(RUST_LL_FILE) >nul && \
			exit /b \
		) \
	)"

rust_to_obf_ir:
	@echo Generating LLVM IR from Rust using Cargo...
	@"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && \
	cargo +nightly-2024-06-26 rustc --release -- --emit=llvm-ir
	@echo Obfuscating and writing to $(RUST_OBF_LL_FILE)...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		for %%f in (target\release\deps\*.ll) do ( \
			echo Found IR file: %%f && \
			echo Running obfuscation... && \
			opt -load-pass-plugin=$(OLLVM_PLUGIN) $(OBF_PASS_OPT) %%f -o $(RUST_OBF_LL_FILE) && \
			echo Done: Obfuscated IR written to $(RUST_OBF_LL_FILE) && \
			exit /b \
		) \
	)"


$(IR_BIN_DIR)/%-obf.ll: $(IR_BIN_DIR)/%.ll
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $< -o $@

ir_link:
	@echo Linking all IR files in $(IR_BIN_DIR)...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		set FILES= && \
		for %%f in ($(IR_BIN_DIR)\*.ll) do ( \
			if /I not %%~nxf==final.ll ( \
				echo Found: %%f && \
				set FILES=!FILES! %%f \
			) \
		) && \
		echo Running llvm-link on: !FILES! && \
		$(LLVM_LINK) -o $(IR_BIN_DIR)\final.ll !FILES! \
	)"



ir_obf_link:
	@echo Linking all *-obf.ll IR files in $(IR_BIN_DIR)...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		set FILES= && \
		for %%f in ($(IR_BIN_DIR)\*-obf.ll) do ( \
			if /I not %%~nxf==final-obf.ll ( \
				echo Found: %%f && \
				set FILES=!FILES! %%f \
			) \
		) && \
		echo Running llvm-link on: !FILES! && \
		$(LLVM_LINK) -o $(IR_BIN_DIR)\final-obf.ll !FILES! \
	)"

ir: ir_setup rust_to_ir ir_link

# Obfuscate rust_example.ll -> rust_example-obf.ll
obf_ir: ir_setup rust_to_obf_ir ir_obf_link

$(IR_OBF_FILE): $(RUST_LL_FILE)
	$(LLVM_OPT) -load-pass-plugin=$(OLLVM_PLUGIN) $(OBF_PASS_OPT) $< -o $@

# Optional: obfuscate final.ll (if you link other IRs later)
obf_final:
	$(LLVM_OPT) -load-pass-plugin=$(OLLVM_PLUGIN) $(OBF_PASS_OPT) $(RUST_LL_FILE) -o $(IR_BIN_DIR)\final-obf.ll

delete:
	if exist vs_env.mk del /f /q vs_env.mk
	if exist $(IR_BIN_DIR) rd /s /q $(IR_BIN_DIR) 
	@cargo clean

#############################################################
## === End: LLVM IR Generation and Obfuscation for Rust ==== 
#############################################################
