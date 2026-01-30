# 俄罗斯方块游戏 (Tetris)

## How to Run

```bash
# 构建并启动所有服务
docker compose up --build -d

# 查看日志
docker compose logs -f

# 停止服务
docker compose down
```

## Services

| 服务 | 端口 | 描述 |
|------|------|------|
| frontend-user | http://localhost:8081 | 游戏前端界面 |
| backend | ws://localhost:8080 | C++ WebSocket 游戏服务器 |

## 测试账号

本项目为单机游戏，无需登录账号。

## 题目内容

**项目要求**: 使用 C++ 语言开发一个俄罗斯方块游戏，要求交互性好。

**实现方案**:
- 后端使用 C++ 实现游戏核心逻辑，通过 WebSocket 提供实时通信
- 前端使用 HTML5 Canvas 实现流畅的游戏渲染
- 使用 Docker 容器化部署，支持 ARM 和 X86 架构

---

## 项目介绍

这是一个使用 C++ 开发的俄罗斯方块游戏。游戏后端使用 C++ 实现核心逻辑，前端使用 HTML5 Canvas 进行渲染，两者通过 WebSocket 进行实时通信。

### 功能特性

- 🎮 经典俄罗斯方块玩法
- ⌨️ 流畅的键盘控制
- 🎯 分数和等级系统
- 👁️ 下一方块预览
- ⏸️ 游戏暂停/继续功能
- 🎨 现代化 UI 设计

### 操作说明

| 按键 | 功能 |
|------|------|
| ← / A | 左移 |
| → / D | 右移 |
| ↑ / W | 旋转 |
| ↓ / S | 软降落 |
| Space | 硬降落 |
| P | 暂停/继续 |
| R | 重新开始 |

### 技术栈

- **后端**: C++ 17, WebSocket
- **前端**: HTML5 Canvas, JavaScript, CSS3
- **容器化**: Docker, Docker Compose

### 项目结构

```
.
├── backend/                 # C++ 游戏服务器
│   ├── src/
│   │   ├── main.cpp
│   │   ├── game.cpp
│   │   ├── game.h
│   │   ├── websocket_server.cpp
│   │   └── websocket_server.h
│   ├── CMakeLists.txt
│   └── Dockerfile
├── frontend-user/           # Web 前端
│   ├── index.html
│   ├── style.css
│   ├── game.js
│   ├── nginx.conf
│   └── Dockerfile
├── docs/
│   ├── Requirements.md
│   └── Roadmap.md
├── docker-compose.yml
├── .gitignore
└── README.md
```
