#include "websocket_server.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <fcntl.h>

namespace tetris {

// Base64 编码表
static const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string WebSocketServer::base64Encode(const unsigned char* data, size_t len) {
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];
        
        result += BASE64_CHARS[(n >> 18) & 0x3F];
        result += BASE64_CHARS[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? BASE64_CHARS[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? BASE64_CHARS[n & 0x3F] : '=';
    }
    
    return result;
}

// 简单 SHA1 实现
std::string WebSocketServer::sha1(const std::string& input) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;
    
    std::string msg = input;
    size_t originalLen = msg.length();
    
    msg += (char)0x80;
    while ((msg.length() % 64) != 56) {
        msg += (char)0x00;
    }
    
    uint64_t bitLen = originalLen * 8;
    for (int i = 7; i >= 0; --i) {
        msg += (char)((bitLen >> (i * 8)) & 0xFF);
    }
    
    auto leftRotate = [](uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    };
    
    for (size_t chunk = 0; chunk < msg.length(); chunk += 64) {
        uint32_t w[80];
        
        for (int i = 0; i < 16; ++i) {
            w[i] = ((uint32_t)(unsigned char)msg[chunk + i*4] << 24) |
                   ((uint32_t)(unsigned char)msg[chunk + i*4 + 1] << 16) |
                   ((uint32_t)(unsigned char)msg[chunk + i*4 + 2] << 8) |
                   ((uint32_t)(unsigned char)msg[chunk + i*4 + 3]);
        }
        
        for (int i = 16; i < 80; ++i) {
            w[i] = leftRotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }
        
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftRotate(b, 30);
            b = a;
            a = temp;
        }
        
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    
    unsigned char hash[20];
    for (int i = 0; i < 4; ++i) {
        hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
        hash[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
        hash[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
        hash[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
        hash[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
    }
    
    return std::string((char*)hash, 20);
}

WebSocketServer::WebSocketServer(int port) 
    : port_(port), serverFd_(-1), running_(false), nextClientId_(1) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(serverFd_);
        throw std::runtime_error("Failed to bind socket");
    }
    
    if (listen(serverFd_, 10) < 0) {
        close(serverFd_);
        throw std::runtime_error("Failed to listen");
    }
    
    running_ = true;
    acceptThread_ = std::thread(&WebSocketServer::acceptLoop, this);
    
    std::cout << "WebSocket server started on port " << port_ << std::endl;
}

void WebSocketServer::stop() {
    running_ = false;
    
    if (serverFd_ >= 0) {
        close(serverFd_);
        serverFd_ = -1;
    }
    
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& [id, fd] : clients_) {
        close(fd);
    }
    clients_.clear();
    
    for (auto& [id, thread] : clientThreads_) {
        if (thread.joinable()) {
            thread.detach();
        }
    }
    clientThreads_.clear();
}

void WebSocketServer::acceptLoop() {
    while (running_) {
        pollfd pfd{serverFd_, POLLIN, 0};
        int ret = poll(&pfd, 1, 100);
        
        if (ret <= 0) continue;
        
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientFd < 0) continue;
        
        if (performHandshake(clientFd)) {
            int clientId = nextClientId_++;
            
            {
                std::lock_guard<std::mutex> lock(clientsMutex_);
                clients_[clientId] = clientFd;
            }
            
            clientThreads_[clientId] = std::thread(&WebSocketServer::clientLoop, this, clientId);
            
            std::cout << "Client " << clientId << " connected" << std::endl;
        } else {
            close(clientFd);
        }
    }
}

bool WebSocketServer::performHandshake(int clientFd) {
    char buffer[4096];
    ssize_t len = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) return false;
    buffer[len] = '\0';
    
    std::string request(buffer);
    
    // 查找 Sec-WebSocket-Key
    std::string keyHeader = "Sec-WebSocket-Key: ";
    size_t keyPos = request.find(keyHeader);
    if (keyPos == std::string::npos) return false;
    
    size_t keyStart = keyPos + keyHeader.length();
    size_t keyEnd = request.find("\r\n", keyStart);
    if (keyEnd == std::string::npos) return false;
    
    std::string key = request.substr(keyStart, keyEnd - keyStart);
    
    // 生成 Accept 值
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string hash = sha1(key + magic);
    std::string accept = base64Encode((unsigned char*)hash.c_str(), hash.length());
    
    // 发送握手响应
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept << "\r\n";
    response << "\r\n";
    
    std::string responseStr = response.str();
    send(clientFd, responseStr.c_str(), responseStr.length(), 0);
    
    return true;
}

void WebSocketServer::clientLoop(int clientId) {
    int fd;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(clientId);
        if (it == clients_.end()) return;
        fd = it->second;
    }
    
    while (running_) {
        pollfd pfd{fd, POLLIN, 0};
        int ret = poll(&pfd, 1, 100);
        
        if (ret < 0) break;
        if (ret == 0) continue;
        
        bool closed = false;
        std::string message = readFrame(fd, closed);
        
        if (closed) break;
        
        if (!message.empty() && messageHandler_) {
            messageHandler_(clientId, message);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(clientId);
        if (it != clients_.end()) {
            close(it->second);
            clients_.erase(it);
        }
    }
    
    if (disconnectHandler_) {
        disconnectHandler_(clientId);
    }
    
    std::cout << "Client " << clientId << " disconnected" << std::endl;
}

std::string WebSocketServer::readFrame(int clientFd, bool& closed) {
    closed = false;
    unsigned char header[2];
    
    ssize_t len = recv(clientFd, header, 2, 0);
    if (len <= 0) {
        closed = true;
        return "";
    }
    
    bool fin = (header[0] & 0x80) != 0;
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;
    
    // Pong 帧 - 忽略
    if (opcode == 0x0A) {
        // 读取并丢弃 pong payload
        if (payloadLen > 0 && payloadLen < 126) {
            unsigned char mask[4] = {0};
            if (masked) recv(clientFd, mask, 4, 0);
            std::string tmp(payloadLen, '\0');
            recv(clientFd, &tmp[0], payloadLen, 0);
        }
        return "";
    }
    
    // 连接关闭帧
    if (opcode == 0x08) {
        closed = true;
        return "";
    }
    
    // Ping 帧 - 发送 Pong (需要回复相同的 payload)
    if (opcode == 0x09) {
        // 读取 ping payload
        unsigned char pingMask[4] = {0};
        if (masked) recv(clientFd, pingMask, 4, 0);
        
        std::string pingPayload;
        if (payloadLen > 0 && payloadLen < 126) {
            pingPayload.resize(payloadLen);
            recv(clientFd, &pingPayload[0], payloadLen, 0);
            if (masked) {
                for (size_t i = 0; i < pingPayload.size(); ++i) {
                    pingPayload[i] ^= pingMask[i % 4];
                }
            }
        }
        
        // 发送 pong 帧 (带相同 payload)
        std::vector<unsigned char> pong;
        pong.push_back(0x8A); // FIN + Pong opcode
        pong.push_back((unsigned char)pingPayload.size());
        pong.insert(pong.end(), pingPayload.begin(), pingPayload.end());
        send(clientFd, pong.data(), pong.size(), 0);
        return "";
    }
    
    // 读取扩展长度
    if (payloadLen == 126) {
        unsigned char extLen[2];
        recv(clientFd, extLen, 2, 0);
        payloadLen = (extLen[0] << 8) | extLen[1];
    } else if (payloadLen == 127) {
        unsigned char extLen[8];
        recv(clientFd, extLen, 8, 0);
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | extLen[i];
        }
    }
    
    // 读取掩码
    unsigned char mask[4] = {0};
    if (masked) {
        recv(clientFd, mask, 4, 0);
    }
    
    // 读取数据
    std::string payload(payloadLen, '\0');
    size_t received = 0;
    while (received < payloadLen) {
        ssize_t r = recv(clientFd, &payload[received], payloadLen - received, 0);
        if (r <= 0) {
            closed = true;
            return "";
        }
        received += r;
    }
    
    // 解码
    if (masked) {
        for (size_t i = 0; i < payload.length(); ++i) {
            payload[i] ^= mask[i % 4];
        }
    }
    
    return payload;
}

void WebSocketServer::sendFrame(int clientFd, const std::string& message) {
    std::vector<unsigned char> frame;
    
    // FIN + Text opcode
    frame.push_back(0x81);
    
    // 长度
    if (message.length() < 126) {
        frame.push_back((unsigned char)message.length());
    } else if (message.length() < 65536) {
        frame.push_back(126);
        frame.push_back((message.length() >> 8) & 0xFF);
        frame.push_back(message.length() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((message.length() >> (i * 8)) & 0xFF);
        }
    }
    
    // 数据
    frame.insert(frame.end(), message.begin(), message.end());
    
    send(clientFd, frame.data(), frame.size(), 0);
}

void WebSocketServer::sendMessage(int clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        sendFrame(it->second, message);
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& [id, fd] : clients_) {
        sendFrame(fd, message);
    }
}

} // namespace tetris
