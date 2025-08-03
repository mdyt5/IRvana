/*
Author: @m3rcer
MCJIT interpreter POC
*/

use inkwell::context::Context;
use inkwell::execution_engine::ExecutionEngine;
use inkwell::memory_buffer::MemoryBuffer;
use inkwell::OptimizationLevel;
use std::env;
use std::error::Error;
use std::ffi::{CString}; // CStr removed (only needed on Unix)
use std::fs;
use std::os::raw::{c_char, c_int};

#[cfg(unix)]
fn load_shared_library(path: &str) -> Result<(), Box<dyn Error>> {
    use std::ffi::CStr; // only needed on Unix
    unsafe {
        let c_path = CString::new(path)?;
        let handle = libc::dlopen(c_path.as_ptr(), libc::RTLD_NOW | libc::RTLD_GLOBAL);
        if handle.is_null() {
            let err = CStr::from_ptr(libc::dlerror()).to_string_lossy().into_owned();
            return Err(format!("Failed to load library {}: {}", path, err).into());
        }
    }
    Ok(())
}

#[cfg(windows)]
fn load_shared_library(path: &str) -> Result<(), Box<dyn Error>> {
    unsafe {
        libloading::Library::new(path)?; // Required to be called in unsafe
    }
    Ok(())
}

fn main() -> Result<(), Box<dyn Error>> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!(
            "Usage:\n  {} <LLVM IR file> [main-args...] [--load=shared-lib]\n",
            args[0]
        );
        eprintln!("Options:");
        eprintln!("  <LLVM IR file>         Path to LLVM IR file (.ll) to execute");
        eprintln!("  [main-args...]         Arguments passed to main(int argc, char** argv)");
        eprintln!("  --load=shared-lib      Load shared library before execution\n");
        return Ok(());
    }

    let ir_file = &args[1];
    let mut main_args: Vec<String> = Vec::new();

    for arg in &args[2..] {
        if let Some(lib_path) = arg.strip_prefix("--load=") {
            load_shared_library(lib_path)?;
        } else {
            main_args.push(arg.clone());
        }
    }

    let context = Context::create();
    let ir_bytes = fs::read(ir_file)?;
    let memory_buffer = MemoryBuffer::create_from_memory_range(&ir_bytes, ir_file);
    let module = context.create_module_from_ir(memory_buffer)?;
    let execution_engine: ExecutionEngine =
        module.create_jit_execution_engine(OptimizationLevel::Default)?;

    let maybe_main = unsafe {
        execution_engine.get_function::<unsafe extern "C" fn(c_int, *const *const c_char) -> i32>("main")
    };

    let main = match maybe_main {
        Ok(f) => f,
        Err(_) => return Err("Function `main` not found in IR".into()),
    };

    let c_main_args: Vec<CString> = main_args
        .iter()
        .map(|arg| CString::new(arg.as_str()).unwrap())
        .collect();
    let c_main_ptrs: Vec<*const c_char> = c_main_args.iter().map(|arg| arg.as_ptr()).collect();

    unsafe {
        let ret = main.call(c_main_ptrs.len() as c_int, c_main_ptrs.as_ptr());
        println!("Program exited with: {}", ret);
    }

    Ok(())
}
