#############################################################
## === LLVM IR Generation and Obfuscation for Nim ===
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

IR_SRC_DIR = nim_src
IR_BIN_DIR = ir_bin
NIM_SRC_DIR = src
IR_LL_FILES = $(wildcard $(IR_BIN_DIR)\*.ll)

# Parsing special chars for obfuscation
comma := ,
space :=
space +=

# Discover Nim-generated C files
## Fix for Windows-style paths
#IR_SRC_C_FILES := $(subst \,/,$(wildcard $(IR_SRC_DIR)\*.c))
# Normalize slashes for compatibility
#IR_LL_FILES := $(subst \,/,$(wildcard $(IR_BIN_DIR)\*.ll))
#IR_OBF_FILES := $(patsubst $(IR_BIN_DIR)/%.ll,$(IR_BIN_DIR)/%-obf.ll,$(IR_LL_FILES))

# Paths to tools
# Get the absolute path to this Makefile
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR  := $(dir $(MAKEFILE_PATH))
# Normalize slashes (for Windows)
MAKEFILE_DIR_W := $(subst /,\,$(MAKEFILE_DIR))
# Dynamically find IRVANA_ROOT as 2 levels up from current directory
IRVANA_ROOT := $(MAKEFILE_DIR_W)..\..
# Set LLVM directory inside IRVANA_ROOT
LLVM_DIR      := $(IRVANA_ROOT)\LLVM-18.1.5
LLVM_CLANG    := $(LLVM_DIR)\bin\clang.exe
## clang-cl doset work - compiles but dosent run
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
NIM_COMPILER = $(IRVANA_ROOT)\nim-1.6.6\bin\nim.exe

# Path to Internal library includes
INTERNAL_LIBS:=-I include

# Windows SDK and Visual Studio paths
## === Debug Info ===
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

# Common LLVM IR Generation Flags
#COMMON_CFLAGS:=-mllvm -fla -mllvm -sub -mllvm -bcf
COMMON_CFLAGS:=-S -emit-llvm -fcommon -fno-stack-protector
COMMON_CFLAGS+=-I "$(winsdkdir)\\Include\\$(sdkver)\\ucrt"
COMMON_CFLAGS+=-I "$(winsdkdir)\\Include\\$(sdkver)\\um"
COMMON_CFLAGS+=-I "$(winsdkdir)\\Include\\$(sdkver)\\shared"
COMMON_CFLAGS+=-I "$(vctoolsdir)\\include"
COMMON_CFLAGS+=-I "$(IRVANA_ROOT)\\nim-1.6.6\\lib"

# Obfuscation passes (e.g., make obf_ir OBF_PASSES=sub,cff,bcf)
OBF_PASSES ?=
ifeq ($(strip $(OBF_PASSES)),)
  OBF_PASS_OPT :=
else
  OBF_PASS_EXPANDED := $(foreach pass,$(subst $(comma), ,$(OBF_PASSES)),irobf-$(pass))
  OBF_PASS_OPT := --passes="irobf($(subst $(space),$(comma),$(strip $(OBF_PASS_EXPANDED))))"
endif

# ========== Targets ==========

.PHONY: ir obf_ir link_ir link_obf_ir setup clean nim_to_c

ir_setup: 
	mkdir $(IR_SRC_DIR) $(IR_BIN_DIR)

nim_to_c:
	@echo Generating .c files from Nim sources...
	@for %%f in ($(NIM_SRC_DIR)\*.nim) do ( \
		echo Processing %%f... & \
		$(NIM_COMPILER) c --genScript --cpu:amd64 --nimcache=$(IR_SRC_DIR) %%f & \
		for %%c in ($(IR_SRC_DIR)\@m%%~nf.nim.c) do ( \
			endlocal \
		) \
	)

gen_ir: 
	@echo Discovering .c files in $(IR_SRC_DIR)...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		for %%f in ($(IR_SRC_DIR)\*.c) do ( \
			set src=%%~nxf && \
			set dst=$(IR_BIN_DIR)\%%~nf.ll && \
			echo Compiling !src! to !dst!... && \
			$(LLVM_CLANG) $(IR_WINSDK) $(IR_VCTOOL) $(COMMON_CFLAGS) $(INTERNAL_LIBS) -o!dst! $(IR_SRC_DIR)\!src! \
		) \
	)"

gen_obf_ir:
	@echo Discovering .c files in $(IR_SRC_DIR)...
	@cmd /V:ON /C "( \
		setlocal EnableDelayedExpansion && \
		for %%f in ($(IR_SRC_DIR)\*.c) do ( \
			set src=%%~nxf && \
			set base=%%~nxf && \
			set name=%%~nf && \
			set dst=$(IR_BIN_DIR)\!base:.c=.ll! && \
			set obf=$(IR_BIN_DIR)\!base:.c=-obf.ll! && \
			echo Compiling !src! to !dst!... && \
			$(LLVM_CLANG) $(IR_WINSDK) $(IR_VCTOOL) $(COMMON_CFLAGS) $(INTERNAL_LIBS) -o!dst! $(IR_SRC_DIR)\!src! && \
			echo Obfuscating !dst! to !obf!... && \
			$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) !dst! -o !obf! \
		) \
	)"


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

ir: ir_setup nim_to_c gen_ir ir_link

obf_ir: ir_setup nim_to_c gen_obf_ir ir_obf_link

$(info Obfuscating: $(IR_OBF_FILES))
# Obfuscate each IR file
$(IR_BIN_DIR)/%-obf.ll: $(IR_BIN_DIR)/%.ll
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $< -o $@

obf_final:
	$(LLVM_OPT) -load-pass-plugin="$(OLLVM_PLUGIN)" $(OBF_PASS_OPT) $(IR_BIN_DIR)/final.ll -o $(IR_BIN_DIR)/final-obf.ll

link_obf_ir: $(IR_OBF_FILES)
	$(LLVM_LINK) -o $(IR_BIN_DIR)/final-obf.ll $^

delete:	
	if exist vs_env.mk del /f /q vs_env.mk
	if exist $(IR_BIN_DIR) rd /s /q $(IR_BIN_DIR) 
	if exist $(IR_SRC_DIR) rd /s /q $(IR_SRC_DIR)	

###########################################################
## === End: LLVM IR Generation and Obfuscation Snippet ===
###########################################################
