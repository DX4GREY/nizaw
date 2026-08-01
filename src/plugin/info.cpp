#include "nizaw/plugin.hpp"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <filesystem>
#include <system_error>
#include <vector>

namespace nizaw::plugin {
namespace {

bool has_so_extension(const std::filesystem::path& path) {
    return path.extension() == ".so";
}

std::vector<std::string> split_commands(std::string_view commands) {
    std::vector<std::string> result;
    std::string current;
    for (char ch : commands) {
        if (ch == ',' || ch == ' ' || ch == ';') {
            if (!current.empty()) {
                result.emplace_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        result.emplace_back(current);
    }
    return result;
}

Result<PluginInfo> load_plugin_file(const std::filesystem::path& path, void*& handle_out) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return Error(ErrorCode::IoError, std::string("dlopen() failed: ") + dlerror(), "plugin");
    }

    dlerror();
    auto descriptor = reinterpret_cast<const PluginDescriptor*>(dlsym(handle, "nizaw_plugin_descriptor"));
    const char* load_error = dlerror();
    if (load_error != nullptr || descriptor == nullptr) {
        dlclose(handle);
        return Error(ErrorCode::Unsupported, "Shared object is not a Nizaw plugin", "plugin");
    }

    if (descriptor->name == nullptr || descriptor->name[0] == '\0') {
        dlclose(handle);
        return Error(ErrorCode::InvalidArgument, "Plugin descriptor missing name", "plugin");
    }

    if (descriptor->api_version == nullptr || std::string_view(descriptor->api_version) != PluginApiVersion) {
        dlclose(handle);
        return Error(ErrorCode::Unsupported, "Plugin API version mismatch", "plugin");
    }

    PluginInfo info;
    info.name = descriptor->name;
    info.version = descriptor->version ? descriptor->version : std::string();
    info.description = descriptor->description ? descriptor->description : std::string();
    info.api_version = descriptor->api_version ? descriptor->api_version : std::string();
    info.path = path.string();
    if (descriptor->commands != nullptr) {
        info.commands = split_commands(descriptor->commands);
    }

    handle_out = handle;
    return info;
}

}  // namespace

Registry::~Registry() {
    for (void* handle : handles_) {
        if (handle) {
            dlclose(handle);
        }
    }
}

Result<void> Registry::load_directory(std::string_view directory) {
    const std::filesystem::path dir_path(directory);
    std::error_code ec;
    if (!std::filesystem::exists(dir_path, ec) || ec) {
        return Error(ErrorCode::NotFound, "Plugin directory does not exist", "plugin");
    }
    if (!std::filesystem::is_directory(dir_path, ec) || ec) {
        return Error(ErrorCode::InvalidArgument, "Plugin directory is not a directory", "plugin");
    }

    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return Error::from_errno(errno, ErrorCode::IoError, "plugin", "opendir() failed");
    }

    struct DirCloser {
        DIR* dir;
        ~DirCloser() { if (dir) { closedir(dir); } }
    } closer{dir};

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        const std::filesystem::path entry_path = dir_path / entry->d_name;
        if (!has_so_extension(entry_path)) {
            continue;
        }

        if (!std::filesystem::is_regular_file(entry_path, ec) || ec) {
            continue;
        }

        void* handle = nullptr;
        const auto plugin_result = load_plugin_file(entry_path, handle);
        if (!plugin_result) {
            if (plugin_result.error().code() == ErrorCode::Unsupported) {
                continue;
            }
            return plugin_result.error();
        }

        handles_.push_back(handle);
        plugins_.push_back(plugin_result.value());
    }

    return {};
}

const std::vector<PluginInfo>& Registry::plugins() const noexcept {
    return plugins_;
}

bool Registry::empty() const noexcept {
    return plugins_.empty();
}

Result<std::vector<PluginInfo>> discover(std::string_view directory) {
    Registry registry;
    const auto load_result = registry.load_directory(directory);
    if (!load_result) {
        return load_result.error();
    }
    return registry.plugins();
}

}  // namespace nizaw::plugin
