module;

#include <memory>
#include <angelscript.h>

export module stay3.system.script.angelscript:objects;

namespace st::ags {

template<typename as_type>
class wrapper {
private:
    as_type *raw{nullptr};

public:
    wrapper(const wrapper &) = delete;
    wrapper(wrapper &&other) noexcept: wrapper{other.raw} {
        other.raw = nullptr;
    }
    wrapper &operator=(const wrapper &) = delete;
    wrapper &operator=(wrapper &&other) noexcept {
        if(this != std::addressof(other)) {
            if(raw != nullptr) { raw->Release(); }
            raw = other.raw;
            other.raw = nullptr;
        }
        return *this;
    }

    void acquire(as_type *ptr) {
        this->operator=(wrapper{ptr});
    }

    wrapper(as_type *obj = nullptr): raw{obj} {
        if(obj != nullptr) { obj->AddRef(); }
    }
    ~wrapper() {
        if(raw != nullptr) { raw->Release(); }
    }
    as_type *get() {
        return raw;
    }
    [[nodiscard]] const as_type *get() const {
        return raw;
    }
    as_type *operator->() {
        return raw;
    }
};

using object = wrapper<asIScriptObject>;
using function = wrapper<asIScriptFunction>;

} // namespace st::ags
