#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include <vector>
#include <array>
#include <random>
#include <string>
#include <chrono>

namespace tetris {

// 方块类型
enum class TetrominoType {
    I, O, T, S, Z, J, L, NONE
};

// 游戏状态
enum class GameState {
    WAITING,    // 等待开始
    PLAYING,    // 游戏中
    PAUSED,     // 暂停
    GAME_OVER   // 游戏结束
};

// 方块定义
struct Tetromino {
    TetrominoType type;
    int x, y;           // 位置
    int rotation;       // 旋转状态 (0-3)
    
    // 获取当前形状
    std::vector<std::pair<int, int>> getShape() const;
    
    // 获取颜色代码
    int getColor() const;
};

// 游戏板
class GameBoard {
public:
    static constexpr int WIDTH = 10;
    static constexpr int HEIGHT = 20;
    
    GameBoard();
    
    // 检查位置是否有效
    bool isValidPosition(const Tetromino& piece) const;
    
    // 锁定方块到板上
    void lockPiece(const Tetromino& piece);
    
    // 清除完整行，返回清除的行数
    int clearLines();
    
    // 获取板状态
    const std::array<std::array<int, WIDTH>, HEIGHT>& getBoard() const { return board_; }
    
    // 重置板
    void reset();

private:
    std::array<std::array<int, WIDTH>, HEIGHT> board_;
};

// 游戏类
class Game {
public:
    Game();
    
    // 游戏控制
    void start();
    void pause();
    void resume();
    void reset();
    
    // 方块控制
    bool moveLeft();
    bool moveRight();
    bool moveDown();
    bool rotate();
    void hardDrop();
    
    // 游戏更新（返回是否有变化）
    bool update();
    
    // 获取游戏状态JSON
    std::string getStateJson() const;
    
    // 获取当前状态
    GameState getState() const { return state_; }
    int getScore() const { return score_; }
    int getLevel() const { return level_; }
    int getLines() const { return linesCleared_; }

private:
    void spawnNewPiece();
    TetrominoType getRandomType();
    int getDropInterval() const;
    
    GameBoard board_;
    Tetromino currentPiece_;
    TetrominoType nextPiece_;
    GameState state_;
    
    int score_;
    int level_;
    int linesCleared_;
    
    std::mt19937 rng_;
    std::chrono::steady_clock::time_point lastDropTime_;
};

} // namespace tetris

#endif // TETRIS_GAME_H
