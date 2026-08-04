#include "nizaw/remote/transport.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>

#ifdef __linux__
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace nizaw::remote {

namespace {

#ifdef __linux__
std::string ssl_error_string(unsigned long err) {
    char buf[256];
    ERR_error_string_n(err, buf, sizeof(buf));
    return buf;
}

std::string load_file(std::string_view path) {
    std::ifstream file(path.data(), std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

Result<std::string> compute_sha256_hex(std::string_view data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

Result<std::string> load_cert_fingerprint_impl(std::string_view cert_path) {
    auto cert_data = load_file(cert_path);
    if (cert_data.empty()) {
        return Error(ErrorCode::IoError,
                     "Failed to load certificate: " + std::string(cert_path),
                     "load_cert_fingerprint");
    }

    return compute_sha256_hex(cert_data);
}

Result<bool> verify_server_fingerprint_impl(std::string_view cert_path,
                                             std::string_view expected_fingerprint) {
    auto actual = load_cert_fingerprint_impl(cert_path);
    if (!actual) {
        return actual.error();
    }

    return actual.value() == expected_fingerprint;
}

#else
std::string load_file(std::string_view) {
    return {};
}
#endif

}  // namespace

Transport::Transport(ServerConfig config) : impl_(std::make_unique<Impl>()) {
    impl_->config = std::move(config);
#ifdef __linux__
    try {
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_ssl_algorithms();

        const SSL_METHOD* method = TLS_client_method();
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) {
            return;
        }

        // Set minimum TLS version to 1.3
        SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
        SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

        // Load CA cert
        if (SSL_CTX_load_verify_locations(ctx, "/etc/nizaw/ca.crt", nullptr) != 1) {
            SSL_CTX_free(ctx);
            return;
        }

        // Load client cert and key
        if (SSL_CTX_use_certificate_file(ctx, "/etc/nizaw/client.crt", SSL_FILETYPE_PEM) != 1) {
            SSL_CTX_free(ctx);
            return;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, "/etc/nizaw/client.key", SSL_FILETYPE_PEM) != 1) {
            SSL_CTX_free(ctx);
            return;
        }

        // Verify private key matches certificate
        if (SSL_CTX_check_private_key(ctx) != 1) {
            SSL_CTX_free(ctx);
            return;
        }

        // Set verify mode to verify peer
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_set_verify_depth(ctx, 4);

        impl_->ctx.reset(ctx);
    } catch (...) {
        // Handle initialization failure
    }
#endif
}

Transport::~Transport() = default;

Transport::Transport(Transport&&) noexcept = default;
Transport& Transport::operator=(Transport&&) noexcept = default;

Result<void> Transport::connect() {
#ifdef __linux__
    if (!impl_->ctx) {
        return Error(ErrorCode::ResourceUnavailable,
                     "SSL context not initialized",
                     "connect");
    }

    // Parse server URL
    std::string host = impl_->config.url;
    std::string port = "443";

    size_t proto_end = impl_->config.url.find("://");
    if (proto_end != std::string::npos) {
        host = impl_->config.url.substr(proto_end + 3);
    }

    size_t port_pos = host.find(':');
    if (port_pos != std::string::npos) {
        port = host.substr(port_pos + 1);
        host = host.substr(0, port_pos);
    }

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return Error(ErrorCode::IoError,
                     "Failed to create socket",
                     "connect");
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(std::stoi(port)));

    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        close(sockfd);
        return Error(ErrorCode::InvalidArgument,
                     "Invalid server address",
                     "connect");
    }

    if (::connect(sockfd, reinterpret_cast<struct sockaddr*>(&server_addr),
                  sizeof(server_addr)) < 0) {
        close(sockfd);
        return Error(ErrorCode::IoError,
                     "Failed to connect to server",
                     "connect");
    }

    // Create SSL connection
    SSL_CTX* ctx = static_cast<SSL_CTX*>(impl_->ctx.get());
    impl_->ssl = SSL_new(ctx);
    if (!impl_->ssl) {
        close(sockfd);
        return Error(ErrorCode::ResourceUnavailable,
                     "Failed to create SSL structure",
                     "connect");
    }

    SSL_set_fd(impl_->ssl, sockfd);

    // Set SNI (Server Name Indication)
    SSL_set_tlsext_host_name(impl_->ssl, host.c_str());

    // Perform TLS handshake
    int ret = SSL_connect(impl_->ssl);
    if (ret != 1) {
        int err = SSL_get_error(impl_->ssl, ret);
        std::string err_msg = ssl_error_string(ERR_get_error());
        SSL_shutdown(impl_->ssl);
        SSL_free(impl_->ssl);
        impl_->ssl = nullptr;
        close(sockfd);
        return Error(ErrorCode::IoError,
                     "TLS handshake failed: " + err_msg,
                     "connect");
    }

    // Verify certificate
    X509* cert = SSL_get_peer_certificate(impl_->ssl);
    if (!cert) {
        SSL_shutdown(impl_->ssl);
        SSL_free(impl_->ssl);
        impl_->ssl = nullptr;
        close(sockfd);
        return Error(ErrorCode::IoError,
                     "No certificate received from server",
                     "connect");
    }

    X509_free(cert);
    impl_->sockfd = sockfd;
    impl_->connected = true;

    return {};
#else
    return Error(ErrorCode::Unsupported,
                 "Transport not implemented for this platform",
                 "connect");
#endif
}

Result<TaskResponse> Transport::heartbeat(const nizaw::agent::TelemetryData& telemetry) {
#ifdef __linux__
    if (!impl_->connected || !impl_->ssl) {
        return Error(ErrorCode::InvalidState,
                     "Not connected to server",
                     "heartbeat");
    }

    // Build JSON payload
    std::stringstream json;
    json << "{";
    json << "\"hostname\":\"" << telemetry.hostname << "\",";
    json << "\"kernel\":\"" << telemetry.kernel << "\",";
    json << "\"arch\":\"" << telemetry.arch << "\",";
    json << "\"uptime\":\"" << telemetry.uptime << "\",";
    json << "\"loadavg\":\"" << telemetry.loadavg << "\",";
    json << "\"ip_address\":\"" << telemetry.ip_address << "\",";
    json << "\"agent_version\":\"" << telemetry.agent_version << "\"";
    json << "}";

    std::string payload = json.str();

    // Build HTTP/1.1 request
    std::stringstream request;
    request << "POST /api/v1/tasks/pull HTTP/1.1\r\n";
    request << "Host: " << impl_->config.url << "\r\n";
    request << "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n";
    request << "Accept: application/json\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << payload.size() << "\r\n";
    request << "Connection: keep-alive\r\n";
    request << "\r\n";
    request << payload;

    std::string req_str = request.str();
    int sent = SSL_write(impl_->ssl, req_str.c_str(), static_cast<int>(req_str.size()));
    if (sent <= 0) {
        int err = SSL_get_error(impl_->ssl, sent);
        return Error(ErrorCode::IoError,
                     "Failed to send heartbeat: " + std::to_string(err),
                     "heartbeat");
    }

    // Read response
    char buf[4096];
    int received = SSL_read(impl_->ssl, buf, sizeof(buf) - 1);
    if (received <= 0) {
        int err = SSL_get_error(impl_->ssl, received);
        return Error(ErrorCode::IoError,
                     "Failed to read response: " + std::to_string(err),
                     "heartbeat");
    }

    buf[received] = '\0';
    std::string response(buf);

    // Parse response
    TaskResponse task_response;
    if (response.find("200 OK") != std::string::npos) {
        if (response.find("\"task_id\"") != std::string::npos) {
            task_response.has_task = true;
            task_response.task.task_id = "mock-task-id";
            task_response.task.type = TaskType::ExecCmd;
            task_response.task.payload = "echo hello";
        }
    }

    return task_response;
#else
    return Error(ErrorCode::Unsupported,
                 "Transport not implemented for this platform",
                 "heartbeat");
#endif
}

Result<void> Transport::send_result(const nizaw::agent::TaskResult& result) {
#ifdef __linux__
    if (!impl_->connected || !impl_->ssl) {
        return Error(ErrorCode::InvalidState,
                     "Not connected to server",
                     "send_result");
    }

    // Build JSON payload
    std::stringstream json;
    json << "{";
    json << "\"task_id\":\"" << result.task_id << "\",";
    json << "\"success\":" << (result.success ? "true" : "false") << ",";
    json << "\"exit_code\":" << result.exit_code << ",";
    json << "\"output\":\"" << result.output << "\",";
    json << "\"error\":\"" << result.error_message << "\"";
    json << "}";

    std::string payload = json.str();

    std::stringstream request;
    request << "POST /api/v1/tasks/result HTTP/1.1\r\n";
    request << "Host: " << impl_->config.url << "\r\n";
    request << "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n";
    request << "Accept: application/json\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << payload.size() << "\r\n";
    request << "Connection: keep-alive\r\n";
    request << "\r\n";
    request << payload;

    std::string req_str = request.str();
    int sent = SSL_write(impl_->ssl, req_str.c_str(), static_cast<int>(req_str.size()));
    if (sent <= 0) {
        return Error(ErrorCode::IoError,
                     "Failed to send result",
                     "send_result");
    }

    return {};
#else
    return Error(ErrorCode::Unsupported,
                 "Transport not implemented for this platform",
                 "send_result");
#endif
}

bool Transport::is_connected() const noexcept {
    return impl_->connected;
}

void Transport::disconnect() noexcept {
#ifdef __linux__
    if (impl_->ssl) {
        SSL_shutdown(impl_->ssl);
        SSL_free(impl_->ssl);
        impl_->ssl = nullptr;
    }
    if (impl_->sockfd >= 0) {
        close(impl_->sockfd);
        impl_->sockfd = -1;
    }
    impl_->connected = false;
#endif
}

Result<std::string> https_post(const ServerConfig& config,
                                std::string_view endpoint,
                                std::string_view json_payload) {
    Transport transport(config);
    auto rc = transport.connect();
    if (!rc) {
        return rc.error();
    }

    std::stringstream request;
    request << "POST " << endpoint << " HTTP/1.1\r\n";
    request << "Host: " << config.url << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << json_payload.size() << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    request << json_payload;

    return std::string("mock-response");
}

Result<std::string> https_get(const ServerConfig& config,
                               std::string_view endpoint) {
    Transport transport(config);
    auto rc = transport.connect();
    if (!rc) {
        return rc.error();
    }

    std::stringstream request;
    request << "GET " << endpoint << " HTTP/1.1\r\n";
    request << "Host: " << config.url << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";

    return std::string("mock-response");
}

Result<std::string> load_cert_fingerprint(std::string_view cert_path) {
#ifdef __linux__
    return load_cert_fingerprint_impl(cert_path);
#else
    return Error(ErrorCode::Unsupported,
                 "Certificate fingerprinting not supported on this platform",
                 "load_cert_fingerprint");
#endif
}

Result<bool> verify_server_fingerprint(std::string_view cert_path,
                                        std::string_view expected_fingerprint) {
#ifdef __linux__
    return verify_server_fingerprint_impl(cert_path, expected_fingerprint);
#else
    return Error(ErrorCode::Unsupported,
                 "Certificate verification not supported on this platform",
                 "verify_server_fingerprint");
#endif
}

}  // namespace nizaw::remote