module;

export module stay3.core:variant_helper;

export namespace st {
template<typename... ts>
struct visit_helper: ts... {
    using ts::operator()...;
};
} // namespace st