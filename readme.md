**stay3** is a game framework allowing rapid development... to be continued.

As of document writing (01/2025), only Ninja and Visual Studio support modules.

# Build instructions

Requirements: C++ toolchains capable of compiling C++20 and CMake version 3.31 or higher.

```sh
# example configuration
# root directory
cmake -S . -B build -GNinja
```

Other arguments:
* `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for better LSP support
* `-Dstay3_BUILD_TESTS` to build tests
* `-Dstay3_BUILD_EXAMPLES` to build examples

If you are using `clangd` as language server, it might work better with `clang++` as compiler.
