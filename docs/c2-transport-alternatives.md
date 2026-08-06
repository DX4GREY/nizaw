# C2 Transport Alternatives for Nizaw Agent

Current implementation uses mTLS over HTTPS (HTTP/1.1). This document explores alternative transport protocols for the agent-orchestrator communication channel.

## Current Implementation

- **Protocol:** HTTPS/1.1 over TCP
- **Security:** mTLS with TLS 1.3 (client + server certificates)
- **Pattern:** Request/response (agent polls orchestrator)
- **Dependencies:** OpenSSL, zlib

## Recommended Alternatives

### 1. WebSocket (Recommended)

**Why:** Full-duplex, low-overhead, firewall-friendly, standard library support via `libwebsockets` or `Boost.Beast`.

**Pros:**
- Persistent connection eliminates polling overhead
- Real-time bidirectional messaging
- Works through most firewalls (single port 443)
- Can leverage existing mTLS certificates
- Lower latency for task push vs. pull

**Cons:**
- Requires connection keep-alive management
- More complex state machine than HTTP request/response
- Additional dependency (or larger code surface if custom impl)

**CMake option:** `NIZAW_TRANSPORT_WEBSOCKET`

```cpp
// Conceptual API
class WebSocketTransport {
    Result<void> connect(ServerConfig);
    Result<void> send(const json&);
    Result<json> receive();
};
```

### 2. MQTT (IoT-Style Broker)

**Why:** Lightweight pub/sub, designed for constrained devices, many broker options (Mosquitto, EMQX).

**Pros:**
- Extremely lightweight protocol
- Built-in QoS levels (0, 1, 2)
- Last Will and Testament for agent failure detection
- Topic-based addressing simplifies multi-tenant orchestration
- Mature C++ clients (Eclipse Paho)

**Cons:**
- Requires separate broker infrastructure
- Additional operational component to manage
- Not ideal for large message payloads

**Use case:** Large fleets (1000+ agents), intermittent connectivity, IoT-style deployments.

**CMake option:** `NIZAW_TRANSPORT_MQTT`

### 3. gRPC (Structured RPC)

**Why:** Type-safe, efficient binary protocol, streaming support, HTTP/2 based.

**Pros:**
- Strongly typed service contracts via protobuf
- Bi-directional streaming (server push)
- Built-in connection management and retries
- Excellent C++ support
- Can use existing mTLS

**Cons:**
- Requires protobuf compiler (build-time dependency)
- Larger binary size
- Steeper learning curve for orchestrator side

**Use case:** Type-safe service APIs, streaming telemetry, gRPC ecosystem integration.

**CMake option:** `NIZAW_TRANSPORT_GRPC`

### 4. ZeroMQ (High-Performance Messaging)

**Why:** Extremely fast, brokerless topology options, C-native library.

**Pros:**
- Zero-copy messaging where possible
- Multiple patterns: req/rep, pub/sub, push/pull, pipeline
- No central broker required (peer-to-peer capable)
- Very mature, battle-tested

**Cons:**
- No built-in security (must use CurveZMQ or tunnel through TLS)
- Different programming model than HTTP
- Smaller community than gRPC/WebSocket

**Use case:** High-throughput, low-latency requirements, trusted networks.

**CMake option:** `NIZAW_TRANSPORT_ZMQ`

### 5. QUIC/HTTP3

**Why:** Modern transport, faster handshake than TCP+TLS, better mobile/NAT behavior.

**Pros:**
- 0-RTT and 1-RTT connection establishment
- Improved congestion control
- Multiplexing without head-of-line blocking
- Built-in encryption (TLS 1.3)

**Cons:**
- Less mature C++ server libraries (nginx quiche, msquic)
- Not yet universally deployed on servers
- More complex implementation

**Use case:** Future-forward deployments, mobile agents, high-latency networks.

**CMake option:** `NIZAW_TRANSPORT_QUIC`

## Comparison Matrix

| Transport | Latency | Throughput | Complexity | Dependencies | Firewall | Stateful |
|-----------|---------|------------|------------|--------------|----------|----------|
| HTTPS/1.1 (current) | Medium | Medium | Low | OpenSSL | Yes | No |
| WebSocket | Low | High | Medium | libwebsockets/Boost | Yes | Yes |
| MQTT | Low | Low-Medium | Low | Paho MQTT | Yes | Yes |
| gRPC | Low | High | High | protobuf, gRPC | Yes | Yes |
| ZeroMQ | Very Low | Very High | Medium | libzmq | Partial | Yes |
| QUIC | Very Low | High | High | msquic/quiche | Yes | Yes |

## Recommendation

**Primary:** Add **WebSocket** as `NIZAW_TRANSPORT_WEBSOCKET`.
- Minimal new dependencies (can use existing OpenSSL)
- Solves polling inefficiency
- Standard, widely supported
- Gradual migration path from current HTTP implementation

**Secondary:** Add **MQTT** as `NIZAW_TRANSPORT_MQTT`.
- For large-scale fleet deployments
- When broker infrastructure already exists
- Lower bandwidth consumption

**Future:** Evaluate **gRPC** when protobuf definitions stabilize.
- When type safety becomes critical
- For streaming telemetry at scale

## Implementation Strategy

1. Abstract transport layer behind `Transport` interface in `include/nizaw/remote/transport.hpp`
2. Implement `HttpTransport` (current) as default
3. Add `WebSocketTransport` behind CMake option
4. Maintain backward compatibility — default remains HTTP
5. Add integration tests for each transport

```cpp
// transport.hpp
class Transport {
public:
    virtual ~Transport() = default;
    virtual Result<void> connect(ServerConfig) = 0;
    virtual Result<void> send(const json&) = 0;
    virtual Result<json> receive(std::chrono::milliseconds timeout) = 0;
    virtual void disconnect() = 0;
};
```

## Security Considerations

All transports should:
- Use mTLS with the same certificate material
- Validate server certificate fingerprint
- Implement connection timeouts and retry limits
- Sanitize all inbound messages before parsing
- Limit message sizes to prevent memory exhaustion

## Questions for Design Discussion

1. Should the orchestrator support multiple transports simultaneously?
2. Should agents fall back to HTTP if WebSocket fails?
3. Should transport selection be agent-side, orchestrator-side, or both?
4. Do we need transport-specific metrics (latency, jitter)?
5. Should we support authenticated proxies for restricted networks?