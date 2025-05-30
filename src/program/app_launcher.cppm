module;

#include <memory>

export module stay3.program:app_launcher;

import stay3.ecs;
import stay3.node;
import :app;

export namespace st {
class app_launcher {
public:
    app_launcher(const app_config &config = {})
        : m_exit_main{config.web.exit_main}, m_app{std::make_unique<struct app>(config)} {}

    app &app() {
        return *m_app;
    }

    system_manager<tree_context> &systems() {
        return m_app->systems();
    }

    void launch() {
#ifndef __EMSCRIPTEN__
        m_app->run();
#else
        if(m_exit_main) {
            auto *app = m_app.get();
            // We leak the app intentionally so it will persist after emscripten clean the stack
            // Emscripten cleans the stack because `app::run` uses `emscripten_set_main_loop_arg`,
            // which will run the loop after we exit `int main()`
            m_app.release();
            app->run();
        } else {
            m_app->run();
        }
#endif
    }

private:
    bool m_exit_main;
    std::unique_ptr<struct app> m_app;
};
} // namespace st