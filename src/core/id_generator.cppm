module;

#include <cassert>
#include <concepts>
#include <limits>
#include <unordered_set>

export module stay3.core:id_generator;

export namespace st {
template<std::unsigned_integral type>
class id_generator {
public:
    [[nodiscard]] type create() {
        if(m_available_ids.empty()) {
            assert(m_next_max_id < std::numeric_limits<type>::max() && "Id limit exceeded");
            return m_next_max_id++;
        }
        const auto result = *m_available_ids.begin();
        m_available_ids.erase(result);
        return result;
    }
    void recycle(type id) {
        assert(id < m_next_max_id && "Invalid id");
        assert(!m_available_ids.contains(id) && "Id was recycled");
        m_available_ids.insert(id);
    }
    [[nodiscard]] bool is_id_active(type id) const {
        return id < m_next_max_id && !m_available_ids.contains(id);
    }

private:
    std::unordered_set<type> m_available_ids;
    type m_next_max_id{};
};
} // namespace st
