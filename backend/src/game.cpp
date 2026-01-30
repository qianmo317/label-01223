#include "game.h"
#include <sstream>
#include <algorithm>

namespace tetris {

// 方块形状定义 (每种方块4个旋转状态，每个状态4个格子的相对偏移)
// 格式: [type][rotation][block_index] = {row_offset, col_offset}
struct ShapeData {
    int shapes[7][4][4][2] = {
        // I
        {
            {{0,-1}, {0,0}, {0,1}, {0,2}},
            {{-1,0}, {0,0}, {1,0}, {2,0}},
            {{0,-1}, {0,0}, {0,1}, {0,2}},
            {{-1,0}, {0,0}, {1,0}, {2,0}}
        },
        // O
        {
            {{0,0}, {0,1}, {1,0}, {1,1}},
            {{0,0}, {0,1}, {1,0}, {1,1}},
            {{0,0}, {0,1}, {1,0}, {1,1}},
            {{0,0}, {0,1}, {1,0}, {1,1}}
        },
        // T
        {
            {{0,-1}, {0,0}, {0,1}, {1,0}},
            {{-1,0}, {0,0}, {1,0}, {0,1}},
            {{-1,0}, {0,-1}, {0,0}, {0,1}},
            {{0,-1}, {-1,0}, {0,0}, {1,0}}
        },
        // S
        {
            {{0,0}, {0,1}, {1,-1}, {1,0}},
            {{-1,0}, {0,0}, {0,1}, {1,1}},
            {{0,0}, {0,1}, {1,-1}, {1,0}},
            {{-1,0}, {0,0}, {0,1}, {1,1}}
        },
        // Z
        {
            {{0,-1}, {0,0}, {1,0}, {1,1}},
            {{-1,1}, {0,0}, {0,1}, {1,0}},
            {{0,-1}, {0,0}, {1,0}, {1,1}},
            {{-1,1}, {0,0}, {0,1}, {1,0}}
        },
        // J
        {
            {{0,-1}, {0,0}, {0,1}, {1,1}},
            {{-1,0}, {0,0}, {1,0}, {-1,1}},
            {{-1,-1}, {0,-1}, {0,0}, {0,1}},
            {{1,-1}, {-1,0}, {0,0}, {1,0}}
        },
        // L
        {
            {{0,-1}, {0,0}, {0,1}, {1,-1}},
            {{-1,-1}, {-1,0}, {0,0}, {1,0}},
            {{-1,1}, {0,-1}, {0,0}, {0,1}},
            {{-1,0}, {0,0}, {1,0}, {1,1}}
        }
    };
};

static ShapeData shapeData;

// 颜色代码
static const int COLORS[] = {1, 2, 3, 4, 5, 6, 7}; // I, O, T, S, Z, J, L

std::vector<std::pair<int, int>> Tetromino::getShape() const {
    if (type == TetrominoType::NONE) return {};
    int typeIdx = static_cast<int>(type);
    int rot = rotation % 4;
    
    std::vector<std::pair<int, int>> shape;
    for (int i = 0; i < 4; i++) {
        int rowOff = shapeData.shapes[typeIdx][rot][i][0];
        int colOff = shapeData.shapes[typeIdx][rot][i][1];
        shape.push_back(std::make_pair(y + rowOff, x + colOff));
    }
    return shape;
}

int Tetromino::getColor() const {
    if (type == TetrominoType::NONE) return 0;
    return COLORS[static_cast<int>(type)];
}

// GameBoard 实现
GameBoard::GameBoard() {
    reset();
}

void GameBoard::reset() {
    for (auto& row : board_) {
        row.fill(0);
    }
}

bool GameBoard::isValidPosition(const Tetromino& piece) const {
    auto shape = piece.getShape();
    for (const auto& pos : shape) {
        int row = pos.first;
        int col = pos.second;
        if (col < 0 || col >= WIDTH || row < 0 || row >= HEIGHT) {
            return false;
        }
        if (board_[row][col] != 0) {
            return false;
        }
    }
    return true;
}

void GameBoard::lockPiece(const Tetromino& piece) {
    auto shape = piece.getShape();
    int color = piece.getColor();
    for (const auto& pos : shape) {
        int row = pos.first;
        int col = pos.second;
        if (row >= 0 && row < HEIGHT && col >= 0 && col < WIDTH) {
            board_[row][col] = color;
        }
    }
}

int GameBoard::clearLines() {
    int linesCleared = 0;
    
    for (int row = HEIGHT - 1; row >= 0; --row) {
        bool full = true;
        for (int col = 0; col < WIDTH; ++col) {
            if (board_[row][col] == 0) {
                full = false;
                break;
            }
        }
        
        if (full) {
            linesCleared++;
            // 将上面的行下移
            for (int r = row; r > 0; --r) {
                board_[r] = board_[r - 1];
            }
            board_[0].fill(0);
            row++; // 重新检查当前行
        }
    }
    
    return linesCleared;
}

// Game 实现
Game::Game() : rng_(std::random_device{}()) {
    reset();
}

void Game::reset() {
    board_.reset();
    state_ = GameState::WAITING;
    score_ = 0;
    level_ = 1;
    linesCleared_ = 0;
    nextPiece_ = getRandomType();
    currentPiece_.type = TetrominoType::NONE;
}

void Game::start() {
    if (state_ == GameState::WAITING || state_ == GameState::GAME_OVER) {
        reset();
        state_ = GameState::PLAYING;
        spawnNewPiece();
        lastDropTime_ = std::chrono::steady_clock::now();
    }
}

void Game::pause() {
    if (state_ == GameState::PLAYING) {
        state_ = GameState::PAUSED;
    }
}

void Game::resume() {
    if (state_ == GameState::PAUSED) {
        state_ = GameState::PLAYING;
        lastDropTime_ = std::chrono::steady_clock::now();
    }
}

TetrominoType Game::getRandomType() {
    std::uniform_int_distribution<int> dist(0, 6);
    return static_cast<TetrominoType>(dist(rng_));
}

void Game::spawnNewPiece() {
    currentPiece_.type = nextPiece_;
    currentPiece_.x = GameBoard::WIDTH / 2;
    currentPiece_.y = 0;
    currentPiece_.rotation = 0;
    nextPiece_ = getRandomType();
    
    // 检查是否可以放置新方块
    if (!board_.isValidPosition(currentPiece_)) {
        state_ = GameState::GAME_OVER;
    }
}

bool Game::moveLeft() {
    if (state_ != GameState::PLAYING) return false;
    
    Tetromino test = currentPiece_;
    test.x--;
    if (board_.isValidPosition(test)) {
        currentPiece_ = test;
        return true;
    }
    return false;
}

bool Game::moveRight() {
    if (state_ != GameState::PLAYING) return false;
    
    Tetromino test = currentPiece_;
    test.x++;
    if (board_.isValidPosition(test)) {
        currentPiece_ = test;
        return true;
    }
    return false;
}

bool Game::moveDown() {
    if (state_ != GameState::PLAYING) return false;
    
    Tetromino test = currentPiece_;
    test.y++;
    if (board_.isValidPosition(test)) {
        currentPiece_ = test;
        return true;
    }
    
    // 无法下移，锁定方块
    board_.lockPiece(currentPiece_);
    int lines = board_.clearLines();
    
    if (lines > 0) {
        linesCleared_ += lines;
        // 分数计算：1行100分，2行300分，3行500分，4行800分
        static const int SCORES[] = {0, 100, 300, 500, 800};
        score_ += SCORES[lines] * level_;
        // 每10行升一级
        level_ = (linesCleared_ / 10) + 1;
    }
    
    spawnNewPiece();
    return false;
}

bool Game::rotate() {
    if (state_ != GameState::PLAYING) return false;
    
    Tetromino test = currentPiece_;
    test.rotation = (test.rotation + 1) % 4;
    
    // 尝试基本旋转
    if (board_.isValidPosition(test)) {
        currentPiece_ = test;
        return true;
    }
    
    // 墙踢尝试
    static const int kicks[][2] = {{-1, 0}, {1, 0}, {0, -1}, {-2, 0}, {2, 0}};
    for (int i = 0; i < 5; i++) {
        Tetromino kicked = test;
        kicked.x += kicks[i][0];
        kicked.y += kicks[i][1];
        if (board_.isValidPosition(kicked)) {
            currentPiece_ = kicked;
            return true;
        }
    }
    
    return false;
}

void Game::hardDrop() {
    if (state_ != GameState::PLAYING) return;
    
    while (moveDown()) {
        score_ += 2; // 硬降落每格2分
    }
}

int Game::getDropInterval() const {
    // 等级越高，下落越快
    int baseInterval = 1000; // 基础1秒
    int interval = baseInterval - (level_ - 1) * 80;
    return std::max(interval, 100); // 最快100ms
}

bool Game::update() {
    if (state_ != GameState::PLAYING) return false;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDropTime_).count();
    
    if (elapsed >= getDropInterval()) {
        lastDropTime_ = now;
        moveDown();
        return true;
    }
    
    return false;
}

std::string Game::getStateJson() const {
    std::ostringstream oss;
    oss << "{";
    
    // 游戏状态
    oss << "\"state\":";
    switch (state_) {
        case GameState::WAITING: oss << "\"waiting\""; break;
        case GameState::PLAYING: oss << "\"playing\""; break;
        case GameState::PAUSED: oss << "\"paused\""; break;
        case GameState::GAME_OVER: oss << "\"gameover\""; break;
    }
    
    // 分数信息
    oss << ",\"score\":" << score_;
    oss << ",\"level\":" << level_;
    oss << ",\"lines\":" << linesCleared_;
    
    // 游戏板
    oss << ",\"board\":[";
    const auto& board = board_.getBoard();
    for (int row = 0; row < GameBoard::HEIGHT; ++row) {
        if (row > 0) oss << ",";
        oss << "[";
        for (int col = 0; col < GameBoard::WIDTH; ++col) {
            if (col > 0) oss << ",";
            oss << board[row][col];
        }
        oss << "]";
    }
    oss << "]";
    
    // 当前方块
    if (currentPiece_.type != TetrominoType::NONE) {
        oss << ",\"current\":{";
        oss << "\"type\":" << static_cast<int>(currentPiece_.type);
        oss << ",\"x\":" << currentPiece_.x;
        oss << ",\"y\":" << currentPiece_.y;
        oss << ",\"rotation\":" << currentPiece_.rotation;
        oss << ",\"color\":" << currentPiece_.getColor();
        oss << ",\"shape\":[";
        auto shape = currentPiece_.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "[" << shape[i].first << "," << shape[i].second << "]";
        }
        oss << "]}";
    }
    
    // 下一个方块
    oss << ",\"next\":" << static_cast<int>(nextPiece_);
    
    oss << "}";
    return oss.str();
}

} // namespace tetris
