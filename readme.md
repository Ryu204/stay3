# 🚧 Under Construction 🚧

**stay3** is a game framework allowing rapid development... to be continued.

As of document writing (01/2025), only Ninja and Visual Studio support modules.

However, currently `EnTT` failed to build if used inside a module with MSVC.

# Important note

1. Coordinate system is left handed, and positive rotation means clockwise. For example, 90 deg rotation around X+ axis is from Y+ to Z+.

1. Angle unit is radians, `radians` is alias of float.

1. The framework follows a tree hierarchy. Root node is created by a `tree_context` instance.

1. ECS is supported by attaching arbitrary amount of entities to each node, and use the registry provided by `tree_context`.

1. Any class act as multiple system types. Requirements are having public methods with:
* correct names
* arguments convertible to types in following table.

|system type|method name|argument(s)|
|-----------|-----------|--------|
|`sys_type::start`|`start`|`tree_context&`|
|`sys_type::update`|`update`|`seconds`, `tree_context&`|
|`sys_type::render`|`render`|`tree_context&`|
|`sys_type::cleanup`|`cleanup`|`tree_context&`|

```cpp
import stay3;
using namespace st;

class my_type;
struct system {
    // Qualified as start system
    bool start(const tree_context&) const;
    
    // Qualified as an update system
    static my_type update(double delta_time, tree_context& ctx);
    
    /* ... */
};
```
# Build instructions

Requirements: C++ toolchains capable of compiling C++23 and CMake version 3.31 or higher. Including but not limited to:
* `clang >= 19`
* `emscripten>=4`
* <del>`msvc >= v17.14`
* <del>`gcc>=14`

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
