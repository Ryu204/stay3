**stay3** is a game framework allowing rapid development... to be continued.

As of document writing (01/2025), only Ninja and Visual Studio support modules.

# Important note

1. Coordinate system is left handed, and positive rotation means clockwise. For example, 90 deg rotation around X+ axis is from Y+ to Z+.

1. Angle unit is radians, `radians` is alias of float.

# Build instructions

Requirements: C++ toolchains capable of compiling C++23 and CMake version 3.31 or higher.

(Linux only) Install dependencies:
```
sudo apt-get install libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev mesa-common-dev libx11-xcb-dev pkg-config nodejs npm libwayland-dev libxkbcommon-dev xorg-dev
```

```sh
# root directory
# Get build dependencies
git submodule update --init --remote --depth=1
# Configure project
cmake -S . -B build -GNinja
```

Other arguments:
* `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for better LSP support
* `-Dstay3_BUILD_TESTS` to build tests
* `-Dstay3_BUILD_EXAMPLES` to build examples

Web build with emscripten:
* Append `-DEMSCRIPTEN_FORCE_COMPILERS=OFF`

Testing commands:

(Web only) Install dependencies:
```sh
cd test/web
npm install
cd ../../
```

```sh
ctest --test-dir build/test --output-on-failure
```

If you are using `clangd` as language server, it might work better with `clang++` as compiler.

More build and run instruction can be found in `.github/workflows` directory.
