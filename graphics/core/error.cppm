export module stay3.graphics.core:error;

import stay3.core;

export namespace st {
struct graphics_error: public st::error {
    using error::error;
};
} // namespace st