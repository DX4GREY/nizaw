#include "nizaw/filesystem.hpp"

#include <filesystem>
#include <iostream>

int run_filesystem_tests() {
    const auto usage = nizaw::filesystem::usage("/");
    if (!usage) {
        std::cerr << "Filesystem usage failed: " << usage.error().message() << std::endl;
        return 1;
    }

    const auto& disk_usage = usage.value();
    if (disk_usage.total_bytes == 0 || disk_usage.available_bytes == 0) {
        std::cerr << "Filesystem usage reported invalid sizes" << std::endl;
        return 1;
    }

    const auto info = nizaw::filesystem::info(std::filesystem::current_path());
    if (!info) {
        std::cerr << "Filesystem info failed: " << info.error().message() << std::endl;
        return 1;
    }

    const auto& entry = info.value();
    if (entry.path.empty()) {
        std::cerr << "Filesystem info path was empty" << std::endl;
        return 1;
    }
    if (entry.type.empty()) {
        std::cerr << "Filesystem info type was empty" << std::endl;
        return 1;
    }

    const auto missing = nizaw::filesystem::info("/definitely/does/not/exist");
    if (missing) {
        std::cerr << "Filesystem info unexpectedly succeeded for a nonexistent path" << std::endl;
        return 1;
    }

    return 0;
}
