module;

#include <cstdint>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.config;

export namespace st {

enum class filter_mode : std::uint8_t {
    nearest,
    linear,
};

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
        default:
            return wgpu::PowerPreference::Undefined;
        }
    }
    static wgpu::FilterMode from_enum(filter_mode mode) {
        switch(mode) {
        case filter_mode::linear:
            return wgpu::FilterMode::Linear;
        case filter_mode::nearest:
            return wgpu::FilterMode::Nearest;
        default:
            return wgpu::FilterMode::Undefined;
        }
    }

    power_preference power_pref{power_preference::low};
    filter_mode filter{filter_mode::linear};
    bool culling{true};
};

} // namespace st