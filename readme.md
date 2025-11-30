# 🚧 Under Construction 🚧

**stay3** is a game framework allowing rapid development... to be continued.

As of document writing (01/2025), only Ninja and Visual Studio support modules.

However, currently `EnTT` failed to build if used inside a module with MSVC.

# Demo

Since the web version makes use of some features that are not wildly supported yet, it's recommended viewer use a Chromium based browser on desktop.

In order to run the test, please go to `chrome://flags` on the search bar and enable:
* Experimental WebAssembly JavaScript Promise Integration (JSPI)
* Unsafe WebGPU Support

Alternately, you can directly go to the address and enable the flags:
* `chrome://flags/#enable-experimental-webassembly-jspi`
* `chrome://flags/#enable-unsafe-webgpu`

![alt text](docs/media/chromium_flags.png)

Checkout the demos [here](https://ryu204.github.io/stay3/).

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
|`sys_type::input`|`input`|`const event&`,`tree_context&`|

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
        for(auto &&[en, comp]: ctx.ecs().each<component>()) {
            // `comp` provides pointer-like interface to component
            sum -= comp->value;
            // Or dereference
            sum += 2 * (*comp).value;
        }
        std::cout << "Sum = " << sum << '\n';
    }
};
```
If you want to modify the component, replace `component` with `mut<component>`. This however may be a litle bit slower [^1].

The syntax is so dumb because I wanted to make it possible to automatically react to changes after users are done with `each` or `get`, which means `comp` must be some proxy tricky class to report the change in destructor. To actually get the component reference, I have to call a method or operator on `comp`. `->` and `*` seems to take least number of characters.

[^1]: mutable version will have the destructor invoke the update signal (if somebody subcribed to it before).

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
|construct|**After** the component is created|`emplace`, `emplace_if_not_exist`, `emplace_or_replace`|
|update|**After** the component is changed|`patch` or `replace`<br>`each` or `get` or `emplace` variants with a non const type parameter and the proxy goes out of scope|
|destroy|**Before** the component is removed|`destroy`, `destroy_if_exist` (either the component or the whole entity)|

8. To signal exit from inside a system method, use `sys_run_result` as a return type:

```cpp
struct my_system {
    static sys_run_result update(seconds, tree_context &) {
        /* Do something here and realize we need to quit */
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

10. There are multiples signals published during the execution:
* `ecs_registry`:
    * `on<comp event, type>`: Signal related to components. Handlers should never add or remove component from any entity if it's observing the same type.
    * `on_entity_destroyed`: An entity is about to be destroyed, it is no longer related to the scene tree. Handlers should never add new component to it.
* `entities_holder`:
    * `on_destroyed`: An entity it owns is destroyed. The entity is still considered owned by holder and its node in the handler.
    * `on_created`: An entity is created and associated with the holder and its node.
* `tree_context`:
    * `on_entity_destroyed`: same with `entities_holder::on_destroyed`, but with `node&` as extra argument.
    * `on_entity_created`:same with `entities_holder::on_created`.

tl;dr:
* Entity signals emitted from ecs registry do not have associations with scene tree anymore (`tree_context::get_node` will not work)
* Never attach components to entity that is signaled to be destroyed
* Never add or destroy component in signal handler of that component.
* Components destruction order of a destroyed entity is not deterministic so you shouldn't perform arbitrary registry manipulation in the handler.

11. There is currently a primitive way to specify a dependency relationship between components. Definition:

Component `deps` is considered a dependency of component `base` in the registry if:
> For every entity `e` with `base`, it has a `deps`. 

This ensures whenever a system queries entities with `base`, they will have following `deps`s (hence the keyword *dependency*).

2 ways to specify dependency relationship:

```cpp
// Adds `deps` when `base` is added, and removes it when `base` is removed
template<component deps, component base, typename... args>
void make_hard_dependency(ecs_registry &reg, args &&...arguments);

// Considers `deps` a dependency if when `base` is added, `deps` does not exist
template<component deps, component base, typename... args>
void make_soft_dependency(ecs_registry &reg, args &&...arguments);
```

In soft version, when `base` is added to `e`, if it already has a `deps`, deletion of `e`'s `base` will not trigger deletion of `deps`, otherwise the behavior is identical to hard version. In other word, hard version is owning while soft version may be owning.

**Limitation**: In both version, `args` can contains no more than 1 type.

12. Render mental model:
* `mesh_data` component defines the geometry, color, uv,... information of drawn objects. It can be shared accross multiple entities.
* A `rendered_mesh` component corresponds to a drawn object in the scene. It holds reference to `mesh_data` component.
* There can be multiple cameras, but only the one with `main_camera` tag component will be used for rendering final image.
* Camera initially looks at Z+ direction, with its up vector being Y+.

13. Input
* Input systems will have their methods called for every event, unless one of them explicitly wants to exit. 
* The `event` paramter does not reflect realtime keyboard info. If you want to query realtime status:
```cpp
struct input_system {
    static void input(const event&, tree_context& ctx) {
        auto& window = ctx.vars().get<runtime_info>().window();
        if (window.get_key(scancode::enter) == key_status::pressed) {
            std::cout << "Enter pressed\n";
        }
    }
};
```

14. Component reference

It is common to reference a component from another component, for example a material component will need a texture component. It is dangerous to use pointer or reference directly because the actual component may be reallocated. This is similar to how we access elements of `std::vector` by their indices and not store reference to the element. It is best to use entity and query the component from the registry. 

There is a helper template class: `component_ref`. It stores the entity and component type.

```cpp
struct material {
    component_ref<texture> texture_ref;
};
// ...
auto mat = my_registry.get<material>(my_entity);
auto texture_comp = mat->texture_ref.get(my_registry);
// Equivalent to:
// auto texture_comp = my_registry.get<texture>(mat->texture_ref.entity());
```

15. Text

It's possible to display text in 3D environment with `st::text` component.

```cpp
entity font_en = /* ... */
entity text_en = /* ... */
registry.emplace<font>(font_en, "path/to/font/file");
registry.emplace<text>(text_en, text{
    .font=font_en,
    .content=u8"Hello",
    .size=30,
    .alignment = text::align::left,
});
```

Note that vertical alignment is at bottom.

16. Physics

stay3 uses *JoltPhysics* as its physics backend. There are basic functionalities such as:
* Rigidbody and Collider:
    * Rigidbody declares physics body settings such as `is_sensor`, `allow_sleep`. They are not modifiable after creation.
    * Collider defines the shape of the body. Having a collider means the entity has a physics representation in the backend.
* Collision detection:
    * I use a realtime query approach instead of callbacks. One can register checking for collision by adding component `collision_enter` to entity. 
```cpp
void setup(tree_context& ctx) {
    entity my_en = /* ... */;
    ctx.ecs().emplace<collision_enter>(my_en);
}

void update(seconds, tree_context& ctx) {
    entity my_en = /* ... */;
    for (const auto& col : ctx.ecs().get<collision_enter>(my_en)) {
        /* Access collision info here */
    }
}
```
* There is also `collision_stay` and `collision_exit` components
* **NOTE**: if `rigidbody::allow_sleep` was set to `true` which is the default, a contacting body pair will emit exit event when either of them goes to sleep. Set `allow_sleep` to `false` if it is not desirable.
* `rigidbody::dynamic` and `rigidbody::kinematic` entity will have a `motion` component attached. It can be used to change velocity and force or impulse of the body.
* Physics world transform state will be updated back to global transform component. It is best not to nest physics entity in deep node. 
* If user explicitly modifies local or global transform of a physics entity, its physics transform will be updated accordingly in `physics_system::update`.

17. Web build

Web build is based on [Emscripten toolchain](https://emscripten.org). There is a few limitations, some of them are because of how JavaScript and C++ works.

* There can only be one window instance at a time
* Consequently, there can only be one app instance at a time
* Most importantly, `app::run` ( and `app_launcher::launch`) does not start the app immediately like native version. It will schedule the app to run **AFTER** the `int main()` function returns. It brings some nuances and implications.
    * All internal data are stored inside the `app` instance. If you allocate an `app` in the stack and call its `run` method, the `app` will be destroyed before the game loop is even entered. So if you want to use `app`, you must allocate it somewhere else (static storage or the heap) so it will persist after `int main()`.
    * `app_launcher` is designed to solve this problem. Internally it allocates an `app` instance on the heap and does not free it in the destructor.
    * You should prefer `app_launcher` over `app` in both web and native builds.
* Alternatively, you can config `exit_main` properties of `app_config::web` to `false` when creating an instance. This will make the program behaves exactly like the native version. However, the rendering performance will be very poor [^2].

Please note these limitations are only relevant in the web context.

[^2]: Internally, the first approach uses JavaScript `requestAnimationFrame` to simulate a loop. This ensures proper synchronization between monitor, browser and our application. However, it requires control over our program execution, hence the weird limitations.

18. Why not lua for scripting language?

Initially I used lua to facilitate scripting. However, I was unable to bind the access proxies of ecs registry to lua. There are 2 problems.

In lua, we want read proxy to be semantically const, as in it will not allow mutating the real component. This is similiar but not the same to immutable table. It's const in the sense that this specific variable won't be able to mutate the value, but the value can still be mutated by other non-const variable. Either way, it's also slower if we want to enforce that via metamethod `__newindex`. And finally, it's not trivial to me how we deal with transitive const-ness of table inside a table, especially when the hierarchy can go deep.

Apart from read proxy, write proxy also has a problem. In C++ they notify change in their destructor. In lua, the only way to execute something when a variable goes out of its initial scope is via keyword `<close>`. However, this one is declared by caller not callee, so it's dependent on the user, not the library provider. We can use some macro trick to change every `let var` statement that retrieves a proxy to `let var <const>`. That works and I don't have a problem with it. But it feels flakey, moreover `<close>` is a controversal feature of lua v5.4.

The first reason is why I steered away and decided to use angelscript for this framework.

# Build instructions

Requirements: C++ toolchains capable of compiling C++23 and CMake version 3.31 or higher. Including but not limited to (May 30th 2025):

* `clang >= 19`
* `emscripten>=4`
* <del>`msvc >= v17.14`
* <del>`gcc>=14`

Currently, most major compilers cannot build and I'm developing with Clang.

(Linux only) Install dependencies:
```
sudo apt-get install libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev mesa-common-dev libx11-xcb-dev pkg-config nodejs npm libwayland-dev libxkbcommon-dev xorg-dev
```

```sh
# root directory
# Get build dependencies
git submodule update --init --remote
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

More build and run commands can be found in `.github/workflows` directory.
