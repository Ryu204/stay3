# 🚧 Under Construction 🚧

**stay3** is a game framework allowing rapid development... to be continued.

As of document writing (01/2025), only Ninja and Visual Studio support modules.

However, currently `EnTT` failed to build if used inside a module with MSVC.

# Important note

1. Coordinate system is left handed, and positive rotation means clockwise. For example, 90 deg rotation around X+ axis is from Y+ to Z+.

2. Angle unit is radians, `radians` is alias of float.

3. The framework follows a tree hierarchy. Root node is created by a `tree_context` instance.

4. ECS is supported by attaching arbitrary amount of entities to each node, and use the registry provided by `tree_context`.

5. Any class act as multiple system types. Requirements are having public methods with:
* correct names
* arguments convertible to types in following table.

|system type|method name|argument(s)|
|-----------|-----------|--------|
|`sys_type::start`|`start`|`tree_context&`|
|`sys_type::update`|`update`|`seconds`, `tree_context&`|
|`sys_type::post_update`|`post_update`|`seconds`, `tree_context&`|
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

6. Systems can use `tree_context&` to access and manipulate components

```cpp
struct component { int value{}; };
struct my_system {
    static void update(seconds, tree_context &ctx) {
        auto sum = 0;
        for(auto &&[en, comp]: ctx.ecs().each<const component>()) {
            // `comp` provides pointer-like interface to component
            sum -= comp->value;
            // Or dereference
            sum += 2 * (*comp).value;
        }
        std::cout << "Sum = " << sum << '\n';
    }
};
```
If you want to modify the component, use non const template. This however may be a litle bit slower [^1].

The syntax is so dumb because I wanted to make it possible to automatically react to changes after users are done with `each` or `get_components`, which means `comp` must be some proxy tricky class to report the change in destructor. To actually get the component reference, I have to call a method or operator on `comp`. `->` and `*` seems to take least number of characters.

[^1]: In above example, `comp`'s destructor will invoke the update signal if somebody subcribed to it before.

7. System can add callback to certain events related to component, including `construct`, `update` and `destroy`.
```cpp
// All component signal handlers share this method
void on_entity_construct(ecs_registry& reg, entity en) {
    std::cout << "Entity constructed\n";
}
struct my_system {
    static void start(tree_context &ctx) {
        ctx.ecs()
            .on<comp_event::construct, my_component_class>()
            .connect<&on_entity_construct>();
    }
};
```
This is the (non exhaustive) list of available events.
|Event|Trigger time|Causes|
|-----|------------|------|
|construct|**After** the component is created|`add_component`|
|update|**After** the component is changed|`patch` or `replace`<br>`each` or `get_components` with a non const type parameter and the proxy goes out of scope|
|destroy|**Before** the component is removed|`remove_component`|

8. To signal exit from inside a system method, use `sys_run_result` as a return type:

```cpp
struct my_system {
    static sys_run_result update(seconds, tree_context &) {
        return sys_run_result::exit;
    }
};
```

9. It's possible to iterate over `node` and its entities holder:

```cpp
node& my_node = /* ... */
for (auto& child : my_node) {
    /* child is reference to child node */
}
for (auto entity : my_node.entities()) {
    /* entity belongs to my_node */
}
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
