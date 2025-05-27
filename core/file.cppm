module;

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

export module stay3.core:file;

import :logger;
import :error;

export namespace st {

struct file_error: public error {
    using error::error;
};

std::string read_file_as_str(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if(!file) {
        throw file_error{"Failed to open: " + path.string()};
    }

    try {
        // Get file size
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if(size < 0) {
            throw file_error{"Failed to determine file size: " + path.string()};
        }

        // Reserve space and read the file content
        std::string result;
        result.reserve(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);

        // Using this approach instead of std::getline to handle all types of line endings consistently
        result.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        if(file.bad()) {
            throw file_error{"Failed to read: " + path.string()};
        }

        return result;
    } catch(std::exception &e) {
        throw file_error{"Unexpected error reading " + path.string() + ": " + e.what()};
    }
};

std::optional<std::string> read_file_as_str_nothrow(const std::filesystem::path &path) {
    try {
        return read_file_as_str(path);
    } catch(file_error &e) {
        log::warn(e.message);
        return std::nullopt;
    }
}

bool is_normal_file(const std::filesystem::path &filename, std::string_view file_type = "") {
    if(!std::filesystem::is_regular_file(filename)) {
        log::warn(file_type, " file is not a regular file: ", filename);
        return false;
    }
    return true;
}

} // namespace st