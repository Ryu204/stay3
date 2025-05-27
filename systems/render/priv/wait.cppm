module;

#include <atomic>
#include <webgpu/webgpu_cpp.h>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif
export module stay3.system.render.priv:wait;

export namespace st {

bool wgpu_wait(
    [[maybe_unused]] const wgpu::Instance &instance,
    [[maybe_unused]] const wgpu::Future &fut,
    [[maybe_unused]] std::atomic_bool &is_done

) {
#ifdef __EMSCRIPTEN__
    constexpr auto sleep_ms = 100;
    while(!is_done.load()) {
        instance.ProcessEvents();
        emscripten_sleep(sleep_ms);
    }
    return true;
#else
    return instance.WaitAny(fut, 0) == wgpu::WaitStatus::Success;
#endif
}
} // namespace st