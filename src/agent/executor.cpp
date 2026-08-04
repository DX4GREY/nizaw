#include "nizaw/agent/executor.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <array>

#ifdef __linux__
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <signal.h>
#include <zlib.h>
#include <openssl/sha.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "nizaw/remote/transport.hpp"

namespace nizaw::agent {

namespace {

std::string exec_command(std::string_view cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(cmd.data(), "r");
    if (!pipe) {
        return {};
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    pclose(pipe);
    return result;
}

std::string base64_decode(std::string_view encoded) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string result;
    result.reserve(encoded.size() * 3 / 4);
    
    int val = 0, val_bits = 0;
    for (unsigned char c : encoded) {
        if (c == '=') break;
        
        const char* ptr = strchr(base64_chars, c);
        if (!ptr) continue;
        
        val = (val << 6) + (ptr - base64_chars);
        val_bits += 6;
        
        if (val_bits >= 8) {
            val_bits -= 8;
            result.push_back((val >> val_bits) & 0xFF);
        }
    }
    
    return result;
}

std::string base64_encode(const std::string& data) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    
    int val = 0, val_bits = 0;
    for (unsigned char c : data) {
        val = (val << 8) | c;
        val_bits += 8;
        
        while (val_bits >= 6) {
            val_bits -= 6;
            result.push_back(base64_chars[(val >> val_bits) & 0x3F]);
        }
    }
    
    if (val_bits > 0) {
        val <<= (6 - val_bits);
        result.push_back(base64_chars[val & 0x3F]);
    }
    
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
}

}  // namespace

TaskExecutor::TaskExecutor(const AgentConfig& config) : config_(config) {
}

TaskExecutor::~TaskExecutor() = default;

TaskResult TaskExecutor::execute(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    if (!is_allowed(task.type)) {
        result.success = false;
        result.error_message = "Task type not allowed by configuration";
        result.exit_code = -1;
        return result;
    }
    
    for (const auto& [type, handler] : custom_handlers_) {
        if (type == task.type) {
            return handler(task);
        }
    }
    
    switch (task.type) {
        case nizaw::remote::TaskType::ExecCmd:
            return exec_cmd(task);
        case nizaw::remote::TaskType::ExecScript:
            return exec_script(task);
        case nizaw::remote::TaskType::FetchFile:
            return fetch_file(task);
        case nizaw::remote::TaskType::PushFile:
            return push_file(task);
        case nizaw::remote::TaskType::SysProbe:
            return sys_probe(task);
        case nizaw::remote::TaskType::Sleep:
            return sleep_task(task);
        case nizaw::remote::TaskType::Upgrade:
            return upgrade(task);
        default:
            result.success = false;
            result.error_message = "Unknown task type";
            result.exit_code = -1;
            return result;
    }
}

bool TaskExecutor::is_allowed(nizaw::remote::TaskType type) const {
    switch (type) {
        case nizaw::remote::TaskType::ExecCmd:
        case nizaw::remote::TaskType::ExecScript:
            return config_.allow_exec;
        case nizaw::remote::TaskType::FetchFile:
            return config_.allow_fetch;
        case nizaw::remote::TaskType::PushFile:
            return config_.allow_push;
        case nizaw::remote::TaskType::SysProbe:
        case nizaw::remote::TaskType::Sleep:
        case nizaw::remote::TaskType::Upgrade:
            return true;
        default:
            return false;
    }
}

void TaskExecutor::register_handler(nizaw::remote::TaskType type, CustomHandler handler) {
    custom_handlers_.emplace_back(type, std::move(handler));
}

TaskResult TaskExecutor::exec_cmd(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    if (task.payload.empty()) {
        result.success = false;
        result.error_message = "Empty command payload";
        result.exit_code = -1;
        return result;
    }
    
    result.output = exec_command(task.payload);
    result.exit_code = 0;
    result.success = true;
    
    return result;
}

TaskResult TaskExecutor::exec_script(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    if (task.payload.empty()) {
        result.success = false;
        result.error_message = "Empty script payload";
        result.exit_code = -1;
        return result;
    }
    
    std::string script_content = base64_decode(task.payload);
    if (script_content.empty()) {
        result.success = false;
        result.error_message = "Failed to decode script";
        result.exit_code = -1;
        return result;
    }
    
    std::string temp_path = config_.temp_dir + "/script_" + task.task_id + ".sh";
    
    std::ofstream script_file(temp_path);
    if (!script_file.is_open()) {
        result.success = false;
        result.error_message = "Failed to create script file";
        result.exit_code = -1;
        return result;
    }
    
    script_file << "#!/bin/bash\n";
    script_file << script_content;
    script_file.close();
    
    chmod(temp_path.c_str(), 0755);
    
    result.output = exec_command(temp_path);
    result.exit_code = 0;
    result.success = true;
    
    unlink(temp_path.c_str());
    
    return result;
}

TaskResult TaskExecutor::fetch_file(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    std::string file_path = task.payload;
    if (is_path_restricted(file_path)) {
        result.success = false;
        result.error_message = "Path is restricted";
        result.exit_code = -1;
        return result;
    }
    
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        result.success = false;
        result.error_message = "Failed to open file";
        result.exit_code = -1;
        return result;
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    
    std::string file_data = ss.str();
    uLongf compressed_size = compressBound(file_data.size());
    std::vector<char> compressed(compressed_size);
    
    if (compress(reinterpret_cast<Bytef*>(compressed.data()), &compressed_size,
                 reinterpret_cast<const Bytef*>(file_data.data()), file_data.size()) != Z_OK) {
        result.success = false;
        result.error_message = "Compression failed";
        result.exit_code = -1;
        return result;
    }
    
    result.output = base64_encode(std::string(compressed.data(), compressed_size));
    result.success = true;
    result.exit_code = 0;
    
    return result;
}

TaskResult TaskExecutor::push_file(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    size_t sep_pos = task.payload.find('|');
    if (sep_pos == std::string::npos) {
        result.success = false;
        result.error_message = "Invalid payload format";
        result.exit_code = -1;
        return result;
    }
    
    std::string target_path = task.payload.substr(0, sep_pos);
    std::string base64_data = task.payload.substr(sep_pos + 1);
    
    if (is_path_restricted(target_path)) {
        result.success = false;
        result.error_message = "Path is restricted";
        result.exit_code = -1;
        return result;
    }
    
    if (access(target_path.c_str(), F_OK) == 0) {
        std::string backup_path = target_path + ".bak";
        rename(target_path.c_str(), backup_path.c_str());
    }
    
    std::string file_data = base64_decode(base64_data);
    std::ofstream file(target_path, std::ios::binary);
    if (!file.is_open()) {
        result.success = false;
        result.error_message = "Failed to create file";
        result.exit_code = -1;
        return result;
    }
    
    file.write(file_data.data(), file_data.size());
    file.close();
    
    result.success = true;
    result.exit_code = 0;
    
    return result;
}

TaskResult TaskExecutor::sys_probe(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    if (task.payload == "system_info") {
        result = system_info_probe();
    } else if (task.payload == "process_list") {
        result = process_list_probe();
    } else {
        result.success = false;
        result.error_message = "Unknown sys_probe command";
        result.exit_code = -1;
    }
    
    return result;
}

TaskResult TaskExecutor::sleep_task(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    try {
        int new_interval = std::stoi(task.payload);
        if (new_interval > 0) {
            result.success = true;
            result.exit_code = 0;
        }
    } catch (...) {
        result.success = false;
        result.error_message = "Invalid sleep interval";
        result.exit_code = -1;
    }
    
    return result;
}

TaskResult TaskExecutor::upgrade(const nizaw::remote::RemoteTask& task) {
    TaskResult result;
    result.task_id = task.task_id;
    
    size_t sep_pos = task.payload.find('|');
    if (sep_pos == std::string::npos) {
        result.success = false;
        result.error_message = "Invalid upgrade payload";
        result.exit_code = -1;
        return result;
    }
    
    std::string url = task.payload.substr(0, sep_pos);
    std::string expected_sha256 = task.payload.substr(sep_pos + 1);
    
    std::string temp_path = config_.temp_dir + "/upgrade_binary";
    
    result.success = true;
    result.exit_code = 0;
    result.output = "Upgrade not fully implemented";
    
    return result;
}

bool TaskExecutor::is_path_restricted(std::string_view path) const {
    std::string path_str(path);
    
    for (const auto& restricted : config_.restricted_paths) {
        if (path_str.size() >= restricted.size() &&
            path_str.substr(0, restricted.size()) == restricted) {
            return true;
        }
    }
    
    return false;
}

std::string TaskExecutor::compute_sha256(std::string_view file_path) {
#ifdef __linux__
    std::ifstream file(std::string(file_path), std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        SHA256_Update(&ctx, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
#else
    return {};
#endif
}

TaskResult TaskExecutor::system_info_probe() {
    TaskResult result;
    result.task_id = "sys_probe";
    
#ifdef __linux__
    std::stringstream json;
    struct sysinfo info;
    
    json << "{";
    json << "\"hostname\":\"" << TelemetryData{}.hostname << "\",";
    json << "\"uptime\":" << info.uptime << ",";
    json << "\"loadavg\":\"" 
         << info.loads[0] / 65536.0 << " "
         << info.loads[1] / 65536.0 << " "
         << info.loads[2] / 65536.0 << "\",";
    json << "\"totalram\":" << info.totalram << ",";
    json << "\"freeram\":" << info.freeram << ",";
    json << "\"processes\":" << info.procs;
    json << "}";
    
    result.output = json.str();
    result.success = true;
    result.exit_code = 0;
#else
    result.success = false;
    result.error_message = "Not supported on this platform";
    result.exit_code = -1;
#endif
    
    return result;
}

TaskResult TaskExecutor::process_list_probe() {
    TaskResult result;
    result.task_id = "sys_probe";
    
    result.output = exec_command("ps aux --no-headers 2>/dev/null || echo 'not available'");
    result.success = true;
    result.exit_code = 0;
    
    return result;
}

}  // namespace nizaw::agent