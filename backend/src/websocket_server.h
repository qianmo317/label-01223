#ifndef TETRIS_WEBSOCKET_SERVER_H
#define TETRIS_WEBSOCKET_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

namespace tetris {

class WebSocketServer {
public:
    using MessageHandler = std::function<void(int clientId, const std::string& message)>;
    using DisconnectHandler = std::function<void(int clientId)>;
    
    WebSocketServer(int port);
    ~WebSocketServer();
    
    void setMessageHandler(MessageHandler handler) { messageHandler_ = handler; }
    void setDisconnectHandler(DisconnectHandler handler) { disconnectHandler_ = handler; }
    
    void start();
    void stop();
    
    void sendMessage(int clientId, const std::string& message);
    void broadcast(const std::string& message);
    
    bool isRunning() const { return running_; }

private:
    void acceptLoop();
    void clientLoop(int clientId);
    bool performHandshake(int clientFd);
    std::string readFrame(int clientFd, bool& closed);
    void sendFrame(int clientFd, const std::string& message);
    
    std::string base64Encode(const unsigned char* data, size_t len);
    std::string sha1(const std::string& input);
    
    int port_;
    int serverFd_;
    std::atomic<bool> running_;
    std::atomic<int> nextClientId_;
    
    std::map<int, int> clients_; // clientId -> fd
    std::mutex clientsMutex_;
    
    std::thread acceptThread_;
    std::map<int, std::thread> clientThreads_;
    
    MessageHandler messageHandler_;
    DisconnectHandler disconnectHandler_;
};

} // namespace tetris

#endif // TETRIS_WEBSOCKET_SERVER_H
