# Malware Development and LLVM interpreters

Standard malware development techniques can be effectively integrated with LLVM interpreters to enable IR-level stealth and improve delivery methods.
Since interpreters can be written in any language, malware development techniques applicable to C, C++, Rust, Nim and more can be well integrated.
Examples include in-memory IR execution, module stomping, payload storage, custom LLVM API implementations, hashing-based symbol resolution, and more.

This project also aims to document techniques and experiments that are applicable to malware development workflows involving LLVM IR based Interpreters. At the moment there are two techniques documented as mentioned below:
- [IR Payload Encryption](IREncryption/README.md#ir-payload-encryption)
- [IR Payload Staging from Remote Server](RemoteLoad/README.md#ir-payload-staging-from-remote-server)


Contributions are always welcome for theoretical yet practically applicable ideas. Several advanced concepts can also be explored such as Reflective IR Execution (Embedding interpreters entirely in memory to achieve reflective IR execution), Kernel-Mode Interpreters and drivers in IR, lld debugger based execution.
