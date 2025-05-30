module;

#include <string>

export module stay3.program:config;

import stay3.graphics.core;
import stay3.system.render;
import stay3.physics;

namespace st {

struct web_app_config {
    /**
     * @brief If set to true, the application will run after `main` exits.
     * Else, the behavior is similar to native build. However rendering performance is very poor.
     */
    bool exit_main{true};
};

export struct app_config {
    window_config window{};
    float updates_per_second{60.F};
    render_config render{};
    physics_config physics{};
    web_app_config web{};
    std::string assets_dir{"assets/stay3"};
    bool use_default_systems{true};
};
}; // namespace st