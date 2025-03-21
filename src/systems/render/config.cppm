module;

#include <cstdint>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:config;

export namespace st {

struct render_config {
    enum class power_preference : std::uint8_t {
        low,
        high,
        undefined
    };
    static wgpu::PowerPreference from_enum(power_preference pref) {
        switch(pref) {
        case power_preference::high:
            return wgpu::PowerPreference::HighPerformance;
        case power_preference::low:
            return wgpu::PowerPreference::LowPower;
        case power_preference::undefined:
            return wgpu::PowerPreference::Undefined;
        }
    }

    power_preference power_pref{power_preference::low};
};

} // namespace st