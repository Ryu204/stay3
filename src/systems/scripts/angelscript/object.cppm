module;

#include <memory>
#include <angelscript.h>

export module stay3.system.script.angelscript:object;

namespace st {
export class ags_object {
    asIScriptObject *object;

public:
    ags_object(const ags_object &) = delete;
    ags_object(ags_object &&other) noexcept: ags_object{other.object} {
        other.object = nullptr;
    }
    ags_object &operator=(const ags_object &) = delete;
    ags_object &operator=(ags_object &&other) noexcept {
        if(this != std::addressof(other)) {
            if(object != nullptr) { object->Release(); }
            object = other.object;
            other.object = nullptr;
        }
        return *this;
    }

    ags_object(asIScriptObject *object = nullptr): object{object} {
        if(object != nullptr) { object->AddRef(); }
    }
    ~ags_object() {
        if(object != nullptr) { object->Release(); }
    }
    asIScriptObject *get() {
        return object;
    }
    [[nodiscard]] const asIScriptObject *get() const {
        return object;
    }
    asIScriptObject *operator->() {
        return object;
    }
};
} // namespace st
