module;

export module stay3.program:config;

import stay3.graphics;

export namespace st {

struct app_config {
    window_config window;
    float updates_per_second{60.F};
};
}; // namespace st