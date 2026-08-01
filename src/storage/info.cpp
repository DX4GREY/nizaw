#include "nizaw/storage.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace nizaw::storage {
namespace {

bool read_sys_attr(const std::filesystem::path& path, std::string& out) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    std::getline(input, out);
    out = out.empty() ? out : out.substr(0, out.find_last_not_of(" \t\n\r") + 1);
    return true;
}

std::optional<std::uint64_t> parse_u64(const std::string& value) {
    try {
        return std::stoull(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::uint32_t> parse_u32(const std::string& value) {
    try {
        return static_cast<std::uint32_t>(std::stoul(value));
    } catch (...) {
        return std::nullopt;
    }
}

DeviceType detect_device_type(const std::string& name) {
    if (name.rfind("loop", 0) == 0) {
        return DeviceType::Loop;
    }
    if (name.rfind("ram", 0) == 0) {
        return DeviceType::Ram;
    }
    if (name.rfind("sd", 0) == 0 || name.rfind("nvme", 0) == 0 || name.rfind("vd", 0) == 0 || name.rfind("mmcblk", 0) == 0) {
        return DeviceType::Disk;
    }
    if (name.find_first_not_of("0123456789", 0) != std::string::npos && name.find_first_not_of("0123456789", 0) > 0) {
        return DeviceType::Partition;
    }
    return DeviceType::Unknown;
}

Result<Device> read_device(const std::filesystem::path& device_path) {
    Device device;
    device.sys_path = device_path.string();
    device.name = device_path.filename().string();
    device.dev_node = "/dev/" + device.name;
    device.type = detect_device_type(device.name);

    std::string value;
    if (read_sys_attr(device_path / "size", value)) {
        if (const auto parsed = parse_u64(value)) {
            device.size_bytes = *parsed * 512ull;
        }
    }
    if (read_sys_attr(device_path / "queue/logical_block_size", value)) {
        if (const auto parsed = parse_u32(value)) {
            device.logical_block_size = *parsed;
        }
    }
    if (read_sys_attr(device_path / "queue/physical_block_size", value)) {
        if (const auto parsed = parse_u32(value)) {
            device.physical_block_size = *parsed;
        }
    }
    if (read_sys_attr(device_path / "removable", value)) {
        device.removable = (value == "1");
    }
    if (read_sys_attr(device_path / "ro", value)) {
        device.read_only = (value == "1");
    }
    if (read_sys_attr(device_path / "queue/rotational", value)) {
        device.rotational = (value == "1");
    }
    if (read_sys_attr(device_path / "device/model", value)) {
        device.model = value;
    }
    if (read_sys_attr(device_path / "device/vendor", value)) {
        device.vendor = value;
    }

    return device;
}

}  // namespace

Result<std::vector<Device>> enumerate() {
    std::vector<Device> devices;
    const std::filesystem::path sys_block("/sys/block");
    std::error_code ec;

    if (!std::filesystem::exists(sys_block, ec) || ec) {
        return Error(ErrorCode::NotFound, "/sys/block is unavailable", "storage");
    }

    for (const auto& entry : std::filesystem::directory_iterator(sys_block, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec)) {
            continue;
        }
        const auto result = read_device(entry.path());
        if (result) {
            devices.push_back(result.value());
        }
    }

    return devices;
}

Result<Device> inspect(const std::string& device) {
    const std::filesystem::path dev_node(device);
    if (dev_node.filename().empty()) {
        return Error(ErrorCode::InvalidArgument, "Invalid device path", "storage");
    }

    const auto sys_block = std::filesystem::path("/sys/block") / dev_node.filename();
    if (!std::filesystem::exists(sys_block)) {
        return Error(ErrorCode::NotFound, "Device not found in /sys/block", "storage");
    }

    return read_device(sys_block);
}

}  // namespace nizaw::storage
