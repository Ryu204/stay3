module;

#include <any>
#include <cassert>
#include <typeindex>
#include <unordered_map>
#include <utility>

export module stay3.core:any_map;

namespace st {

using key_type = std::size_t;

template<typename id>
concept any_map_key = requires(id key) { static_cast<key_type>(key); };

export class any_map {
public:
    using key_type = st::key_type;

    template<typename type, typename... args>
    type &emplace(args &&...arguments) {
        auto type_id = std::type_index{typeid(type)};
        assert(!m_variables.contains(type_id) && "Type already exists!");
        return std::any_cast<type &>(m_variables.emplace(type_id, std::make_any<type>(std::forward<args>(arguments)...)).first->second);
    }

    template<typename type, any_map_key id, typename... args>
    type &emplace_as(id key, args &&...arguments) {
        auto &type_map = m_named_variables[static_cast<key_type>(key)];
        auto type_id = std::type_index{typeid(type)};
        assert(!type_map.contains(type_id) && "Named type already exists!");
        return std::any_cast<type &>(type_map.emplace(type_id, std::make_any<type>(std::forward<args>(arguments)...)).first->second);
    }

    template<typename type>
    type &get() {
        return std::any_cast<type &>(m_variables.at(std::type_index{typeid(type)}));
    }

    template<typename type, any_map_key id>
    const type &get(id key) const {
        const auto &type_map = m_named_variables.at(static_cast<key_type>(key));
        return std::any_cast<const type &>(type_map.at(std::type_index{typeid(type)}));
    }

    template<typename type>
    void erase() {
        m_variables.erase(std::type_index{typeid(type)});
    }

    template<typename type, any_map_key id>
    void erase(id key) {
        auto it = m_named_variables.find(static_cast<key_type>(key));
        if(it != m_named_variables.end()) {
            it->second.erase(std::type_index{typeid(type)});
            if(it->second.empty()) {
                m_named_variables.erase(it);
            }
        }
    }

    void clear() {
        m_variables.clear();
        m_named_variables.clear();
    }

private:
    std::unordered_map<std::type_index, std::any> m_variables;
    std::unordered_map<key_type, std::unordered_map<std::type_index, std::any>> m_named_variables;
};
} // namespace st
