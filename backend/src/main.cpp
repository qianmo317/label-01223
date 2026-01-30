#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "game.h"
#include "websocket_server.h"

using namespace tetris;

// 全局变量
std::atomic<bool> g_running(true);
std::map<int, std::unique_ptr<Game>> g_games;
std::mutex g_gamesMutex;
WebSocketServer* g_server = nullptr;

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    g_running = false;
}

void handleMessage(int clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_gamesMutex);
    
    // 获取或创建游戏实例
    auto it = g_games.find(clientId);
    if (it == g_games.end()) {
        g_games[clientId] = std::make_unique<Game>();
        it = g_games.find(clientId);
    }
    
    Game& game = *it->second;
    
    // 处理命令
    if (message == "start") {
        game.start();
    } else if (message == "pause") {
        game.pause();
    } else if (message == "resume") {
        game.resume();
    } else if (message == "reset") {
        game.reset();
    } else if (message == "left") {
        game.moveLeft();
    } else if (message == "right") {
        game.moveRight();
    } else if (message == "down") {
        game.moveDown();
    } else if (message == "rotate") {
        game.rotate();
    } else if (message == "drop") {
        game.hardDrop();
    } else if (message == "state") {
        // 只请求状态，不执行操作
    }
    
    // 发送当前状态
    g_server->sendMessage(clientId, game.getStateJson());
}

void handleDisconnect(int clientId) {
    std::lock_guard<std::mutex> lock(g_gamesMutex);
    g_games.erase(clientId);
}

void gameLoop() {
    while (g_running) {
        {
            std::lock_guard<std::mutex> lock(g_gamesMutex);
            for (auto& [clientId, game] : g_games) {
                if (game->getState() == GameState::PLAYING) {
                    if (game->update()) {
                        g_server->sendMessage(clientId, game->getStateJson());
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "    Tetris Game Server v1.0      " << std::endl;
    std::cout << "==================================" << std::endl;
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        WebSocketServer server(8080);
        g_server = &server;
        
        server.setMessageHandler(handleMessage);
        server.setDisconnectHandler(handleDisconnect);
        
        server.start();
        
        // 启动游戏循环线程
        std::thread gameThread(gameLoop);
        
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        server.stop();
        gameThread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Server stopped." << std::endl;
    return 0;
}
