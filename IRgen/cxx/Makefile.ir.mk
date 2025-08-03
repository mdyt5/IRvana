#############################################################
## === LLVM IR Generation and Obfuscation for C ===
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

IR_SRC_DIR = src
IR_BIN_DIR = ir_bin
IR_SRC_C_FILES := $(wildcard $(IR_SRC_DIR)/*.cpp)
IR_SRC_FILES := $(IR_SRC_C_FILES)
IR_LL_FILES := $(patsubst $(IR_SRC_DIR)/%.cpp,$(IR_BIN_DIR)/%.ll,$(IR_SRC_C_FILES))
IR_OBF_FILES := $(patsubst $(IR_BIN_DIR)/%.ll,$(IR_BIN_DIR)/%-obf.ll,$(IR_LL_FILES))

# Parsing special chars for obfuscation
comma := ,
space :=
space +=

# Get the absolute path to this Makefile
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR  := $(dir $(MAKEFILE_PATH))
# Normalize slashes (for Windows)
MAKEFILE_DIR_W := $(subst /,\,$(MAKEFILE_DIR))
# Dynamically find IRVANA_ROOT as 2 levels up from current directory
IRVANA_ROOT := $(MAKEFILE_DIR_W)..\..
# Set LLVM directory inside IRVANA_ROOT
LLVM_DIR      := $(IRVANA_ROOT)\LLVM-18.1.5
LLVM_CLANG    := $(LLVM_DIR)\bin\clang++.exe
LLVM_OPT      := $(LLVM_DIR)\bin\opt.exe
LLVM_LINK     := $(LLVM_DIR)\bin\llvm-link.exe
# OLLVM Plugin path
OLLVM_PLUGIN ?= $(IRVANA_ROOT)\OLLVM\vs_build\obfuscation\Release\LLVMObfuscationx.dll
# === Debug Info ===
##$(info [Debug] MAKEFILE_PATH  = $(MAKEFILE_PATH))
##$(info [Debug] MAKEFILE_DIR   = $(MAKEFILE_DIR))
##$(info [Debug] IRVANA_ROOT    = $(IRVANA_ROOT))
##$(info [Debug] LLVM_DIR       = $(LLVM_DIR))
##$(info [Debug] LLVM_CLANG     = $(LLVM_CLANG))
##$(info [Debug] LLVM_OPT       = $(LLVM_OPT))
##$(info [Debug] LLVM_LINK      = $(LLVM_LINK))
##$(info [Debug] OLLVM_PLUGIN   = $(OLLVM_PLUGIN))

# Path to Internal library includes
INTERNAL_LIBS:=-I include

# === Debug Info ===
ifeq ("$(winsdkdir)","")
$(warning [Warning] winsdkdir is not set)
else
$(info [Debug] winsdkdir   = $(winsdkdir))
endif

ifeq ("$(vctoolsdir)","")
$(warning [Warning] vctoolsdir is not set)
else
$(info [Debug] vctoolsdir  = $(vctoolsdir))
endif

ifeq ("$(sdkver)","")
$(warning [Warning] sdkver is not set)
else
$(info [Debug] sdkver  = $(sdkver))
endif

IR_WINSDK:= -I "$(winsdkdir)"
IR_VCTOOL:= -I "$(vctoolsdir)"
sdkver:= $(sdkver)
#IR_WINSDK:=/winsdkdir "C://Program Files (x86)//Windows Kits//10"
#IR_VCTOOL:=/vctoolsdir "C://Program Files//Microsoft Visual Studio//2022//Community//VC//Tools//MSVC//14.36.32532"

# Common LLVM IR Generation Flags
COMMON_INCLUDES := \
  -I "$(winsdkdir)/Include/$(sdkver)/ucrt" \
  -I "$(winsdkdir)/Include/$(sdkver)/um" \
  -I "$(winsdkdir)/Include/$(sdkver)/shared" \
  -I "$(vctoolsdir)/include" \
  -I "$(vctoolsdir)/lib"

# Common C++ IR generation flags (converted to Clang-style)
COMMON_CFLAGS := \
  -c -O2 -std=c++17 -S -emit-llvm \
  -Wall -Wno-unused-command-line-argument \
  -nostdlib -Llib -lc++ -lc++abi -lmsvcrt  \
  $(COMMON_INCLUDES)

# User-friendly obfuscation pass selection
OBF_PASSES ?=
ifeq ($(strip $(OBF_PASSES)),)
  OBF_PASS_OPT :=
else
  OBF_PASS_EXPANDED := $(foreach pass,$(subst $(comma),$(space),$(OBF_PASSES)),irobf-$(pass))
  OBF_PASS_OPT := --passes="irobf($(subst $(space),$(comma),$(strip $(OBF_PASS_EXPANDED))))"
endif

# ========== Targets ==========

.PHONY: ir obf_ir link_ir link_obf_ir setup clean

ir_setup:
	if not exist $(IR_BIN_DIR) mkdir $(IR_BIN_DIR)

ir: ir_setup $(IR_LL_FILES) link_ir
# Compile .cpp -> .ll
$(IR_BIN_DIR)/%.ll: $(IR_SRC_DIR)/%.cpp
	$(LLVM_CLANG) $(IR_WINSDK) $(IR_VCTOOL) $(INTERNAL_LIBS) $(COMMON_CFLAGS) -o $@ $<

ir_nolink: ir_setup $(IR_LL_FILES)
# Compile .cpp -> .ll
$(IR_BIN_DIR)/%.ll: $(IR_SRC_DIR)/%.cpp
	$(LLVM_CLANG) $(IR_WINSDK) $(IR_VCTOOL) $(INTERNAL_LIBS) $(COMMON_CFLAGS) -o $@ $<
# Obfuscate each individual .ll file
obf_ir: ir_nolink $(IR_OBF_FILES) link_obf_ir
$(IR_BIN_DIR)/%-obf.ll: $(IR_BIN_DIR)/%.ll
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $< -o $@

# Obfuscation pass at final.ll level → final-obf.ll
obf_final: $(IR_BIN_DIR)/final.ll
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $< -o $(IR_BIN_DIR)/final-obf.ll

# Obfuscate each IR file
$(IR_BIN_DIR)/%-obf.ll: $(IR_BIN_DIR)/%.ll
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $< -o $@

link_ir: $(IR_LL_FILES)
	$(LLVM_LINK) -o $(IR_BIN_DIR)/final.ll $^

# Link obfuscated files -> final-obf.ll
link_obf_ir: $(IR_OBF_FILES)
	$(LLVM_LINK) -o $(IR_BIN_DIR)/final-obf.ll $^

delete:
	if exist vs_env.mk del /f /q vs_env.mk
	if exist ir_bin rd /s /q $(IR_BIN_DIR)	

###########################################################
## === End: LLVM IR Generation and Obfuscation Snippet ===
###########################################################