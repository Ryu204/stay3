export module stay3.system.script:error;
import stay3.core;

export namespace st {
struct script_error: public error {
    using error::error;
};
} // namespace st