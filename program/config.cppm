module;

#include <string>

export module stay3.program:config;

import stay3.graphics.core;
import stay3.system.render;
import stay3.physics;

namespace st {

struct app_web_config {
    static constexpr unsigned int default_sleep_milli{100};
    unsigned int sleep_milli{default_sleep_milli};
};

export struct app_config {
    window_config window{};
    float updates_per_second{60.F};
    render_config render{};
    physics_config physics{};
    std::string assets_dir{"assets/stay3"};
    app_web_config web{};
    bool use_default_systems{true};
};
}; // namespace st