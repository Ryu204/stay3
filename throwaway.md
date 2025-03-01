# About scene hierachy

Questions:

1. Why do we need a scene hierachy?

* Logically group related objects
    * For organization
* Make an object's transform dependent on another
    * Bones in animation
    * Particles in local space
    * Convenient when no physics is involved

1. Why don't we need a scene hierachy?

* It's possible to group things without node hierachy, but it ends up doing the same thing
* performance overhead: transform update, mayyyybe scene traversal
* It does not sync naturally with physics system

-> what if the does not contain transform

1. Is a node required to include transform?

No. If a node's parent does not have transform the node's transform will be relative to origin.

1. Does a node own or reference its children?

I like it to own the children. Either directly holding an instance or via `std::unique_ptr`.

1. If it does, do we allocate on heap? Does that even matter?

This I am not so sure. Maybe it's best to abstract away that detail and allow later iteration.

1. Do we even need an ECS?

Probably.

# ECS design

1. Can entity have multiple components?

Implementation-wise yes, I can use something like:
```cpp
template <typename T, std::size_t Ver>
class Wrapper {
    private:
        T m_data;
    public:
        template <typename... Args>
        Wrapper(Args&&... args) : T{std::forward<Args>(args)...} {}
        T& get();
        const T& get();
};
```

However, it would be hard for system to iterate over all components, since we don't know beforehand the max `Ver`. So No.

2. Who own the registry? Entity and node relationship?

A Node owns some entities. Something like:

```
node& a = /*somehow make it*/;
a.entity.create(2);
# Changed: entity is just id, registry does the heavy lifting
a.entity[0].add<position>(10, 10, 10);
a.entity[1].add<render>(/*...*/);
```

4. How Systems fits in the app loop flow?

Let's go do some research on other frameworks.

Okay, it seems all reasonable solutions predefined some place a system can run, like Bevy or Entitas.

Entitas also have reactive system that is executed once a group of entities changes.

Seems okay to me.

Are system classes or functions?

System will be classes, reason:
Sometimes they need to store their internal states, i.e timer, 3rd party resources (like world in physics) and that is just members. It's possible to make functions access their own states via parameter, but that feels like poor man's OOP to me.

So how do we define a system's execution time? Via inheritance from `class update_system;`? yeah sure. It feels verbose, though...

One possible solutions is to use static interface via concepts and `std::any` to store the object.

Okay that's pretty neat.

5. How does concrete implementation look like?

Node: entities, other Node

entities: Used to index components from registry

(component) Registry: Belong to the tree.

Tree contains component registry and node registry, they are in no way related.

A tree exposes `system()` method that handle system managements.

Potential registration api:

```cpp
tree.system()
    .add<render_system>(ctor_args)
    .run_as<update>(priority::normal);
```

---
System definition needs to query on the tree, so they need tree_context&
System should not be stored in tree :) 
^this