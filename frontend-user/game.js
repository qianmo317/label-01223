// 游戏配置
const CONFIG = {
    BOARD_WIDTH: 10,
    BOARD_HEIGHT: 20,
    CELL_SIZE: 30,
    WS_URL: `ws://${window.location.hostname}:8080`,
    COLORS: {
        0: 'transparent',
        1: '#00f5ff', // I - 青色
        2: '#ffd700', // O - 金色
        3: '#9400d3', // T - 紫色
        4: '#00ff00', // S - 绿色
        5: '#ff0000', // Z - 红色
        6: '#0000ff', // J - 蓝色
        7: '#ff8c00'  // L - 橙色
    },
    GHOST_ALPHA: 0.3
};

// 方块形状（用于预览）
const SHAPES = {
    0: [[0,-1], [0,0], [0,1], [0,2]],      // I
    1: [[0,0], [0,1], [1,0], [1,1]],       // O
    2: [[0,-1], [0,0], [0,1], [1,0]],      // T
    3: [[0,0], [0,1], [1,-1], [1,0]],      // S
    4: [[0,-1], [0,0], [1,0], [1,1]],      // Z
    5: [[0,-1], [0,0], [0,1], [1,1]],      // J
    6: [[0,-1], [0,0], [0,1], [1,-1]]      // L
};

class TetrisGame {
    constructor() {
        this.canvas = document.getElementById('gameCanvas');
        this.ctx = this.canvas.getContext('2d');
        this.nextCanvas = document.getElementById('nextCanvas');
        this.nextCtx = this.nextCanvas.getContext('2d');
        
        this.ws = null;
        this.gameState = null;
        this.isConnected = false;
        
        this.initUI();
        this.initKeyboard();
        this.initMobileControls();
        this.connect();
        
        // 游戏循环
        this.render();
    }
    
    initUI() {
        this.overlay = document.getElementById('gameOverlay');
        this.overlayTitle = document.getElementById('overlayTitle');
        this.overlayMessage = document.getElementById('overlayMessage');
        this.startButton = document.getElementById('startButton');
        this.connectionDot = document.getElementById('connectionDot');
        this.connectionText = document.getElementById('connectionText');
        this.scoreEl = document.getElementById('score');
        this.levelEl = document.getElementById('level');
        this.linesEl = document.getElementById('lines');
        
        this.startButton.addEventListener('click', () => this.startGame());
    }
    
    initKeyboard() {
        document.addEventListener('keydown', (e) => {
            if (!this.isConnected) return;
            
            switch(e.key) {
                case 'ArrowLeft':
                case 'a':
                case 'A':
                    this.send('left');
                    e.preventDefault();
                    break;
                case 'ArrowRight':
                case 'd':
                case 'D':
                    this.send('right');
                    e.preventDefault();
                    break;
                case 'ArrowDown':
                case 's':
                case 'S':
                    this.send('down');
                    e.preventDefault();
                    break;
                case 'ArrowUp':
                case 'w':
                case 'W':
                    this.send('rotate');
                    e.preventDefault();
                    break;
                case ' ':
                    this.send('drop');
                    e.preventDefault();
                    break;
                case 'p':
                case 'P':
                    this.togglePause();
                    e.preventDefault();
                    break;
                case 'r':
                case 'R':
                    this.send('reset');
                    e.preventDefault();
                    break;
                case 'Enter':
                    if (this.gameState?.state === 'waiting' || this.gameState?.state === 'gameover') {
                        this.startGame();
                    }
                    e.preventDefault();
                    break;
            }
        });
    }
    
    initMobileControls() {
        document.querySelectorAll('.mobile-btn').forEach(btn => {
            btn.addEventListener('touchstart', (e) => {
                e.preventDefault();
                const action = btn.dataset.action;
                if (action) {
                    this.send(action);
                }
            });
            
            btn.addEventListener('click', (e) => {
                const action = btn.dataset.action;
                if (action) {
                    this.send(action);
                }
            });
        });
    }
    
    connect() {
        this.updateConnectionStatus(false, '连接中...');
        
        // 清除之前的心跳定时器
        if (this.heartbeatInterval) {
            clearInterval(this.heartbeatInterval);
            this.heartbeatInterval = null;
        }
        
        try {
            this.ws = new WebSocket(CONFIG.WS_URL);
            
            this.ws.onopen = () => {
                this.isConnected = true;
                this.updateConnectionStatus(true, '已连接');
                this.send('state');
                
                // 启动心跳，每 15 秒请求一次状态保持连接
                this.heartbeatInterval = setInterval(() => {
                    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                        this.send('state');
                    }
                }, 15000);
            };
            
            this.ws.onmessage = (event) => {
                try {
                    this.gameState = JSON.parse(event.data);
                    this.updateUI();
                } catch (e) {
                    console.error('Failed to parse game state:', e);
                }
            };
            
            this.ws.onclose = () => {
                this.isConnected = false;
                this.updateConnectionStatus(false, '已断开');
                // 清除心跳
                if (this.heartbeatInterval) {
                    clearInterval(this.heartbeatInterval);
                    this.heartbeatInterval = null;
                }
                // 重连
                setTimeout(() => this.connect(), 2000);
            };
            
            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.updateConnectionStatus(false, '连接错误');
            };
        } catch (e) {
            console.error('Failed to connect:', e);
            setTimeout(() => this.connect(), 2000);
        }
    }
    
    send(message) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(message);
        }
    }
    
    updateConnectionStatus(connected, text) {
        this.connectionDot.className = 'status-dot ' + (connected ? 'connected' : 'disconnected');
        this.connectionText.textContent = text;
    }
    
    startGame() {
        this.send('start');
    }
    
    togglePause() {
        if (this.gameState?.state === 'playing') {
            this.send('pause');
        } else if (this.gameState?.state === 'paused') {
            this.send('resume');
        }
    }
    
    updateUI() {
        if (!this.gameState) return;
        
        // 更新分数
        this.scoreEl.textContent = this.gameState.score.toLocaleString();
        this.levelEl.textContent = this.gameState.level;
        this.linesEl.textContent = this.gameState.lines;
        
        // 更新覆盖层
        switch (this.gameState.state) {
            case 'waiting':
                this.overlay.classList.remove('hidden');
                this.overlayTitle.textContent = '俄罗斯方块';
                this.overlayMessage.textContent = '按 Enter 或点击开始游戏';
                this.startButton.textContent = '开始游戏';
                break;
            case 'playing':
                this.overlay.classList.add('hidden');
                break;
            case 'paused':
                this.overlay.classList.remove('hidden');
                this.overlayTitle.textContent = '游戏暂停';
                this.overlayMessage.textContent = '按 P 继续游戏';
                this.startButton.textContent = '继续游戏';
                break;
            case 'gameover':
                this.overlay.classList.remove('hidden');
                this.overlayTitle.textContent = '游戏结束';
                this.overlayMessage.textContent = `最终得分: ${this.gameState.score.toLocaleString()}`;
                this.startButton.textContent = '再来一局';
                document.querySelector('.game-main').classList.add('game-over');
                setTimeout(() => {
                    document.querySelector('.game-main').classList.remove('game-over');
                }, 300);
                break;
        }
    }
    
    render() {
        this.renderBoard();
        this.renderNextPiece();
        requestAnimationFrame(() => this.render());
    }
    
    renderBoard() {
        const ctx = this.ctx;
        const cellSize = CONFIG.CELL_SIZE;
        
        // 清空画布
        ctx.fillStyle = '#0a0a1a';
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        // 绘制网格
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
        ctx.lineWidth = 1;
        for (let x = 0; x <= CONFIG.BOARD_WIDTH; x++) {
            ctx.beginPath();
            ctx.moveTo(x * cellSize, 0);
            ctx.lineTo(x * cellSize, this.canvas.height);
            ctx.stroke();
        }
        for (let y = 0; y <= CONFIG.BOARD_HEIGHT; y++) {
            ctx.beginPath();
            ctx.moveTo(0, y * cellSize);
            ctx.lineTo(this.canvas.width, y * cellSize);
            ctx.stroke();
        }
        
        if (!this.gameState) return;
        
        // 绘制已放置的方块
        const board = this.gameState.board;
        if (board) {
            for (let y = 0; y < CONFIG.BOARD_HEIGHT; y++) {
                for (let x = 0; x < CONFIG.BOARD_WIDTH; x++) {
                    const cell = board[y][x];
                    if (cell > 0) {
                        this.drawCell(ctx, x, y, CONFIG.COLORS[cell]);
                    }
                }
            }
        }
        
        // 绘制当前方块
        const current = this.gameState.current;
        if (current && current.shape) {
            const color = CONFIG.COLORS[current.color];
            
            // 绘制幽灵方块（预览落点）
            const ghostY = this.calculateGhostY(current);
            if (ghostY !== current.y) {
                for (const [row, col] of current.shape) {
                    const gy = row - current.y + ghostY;
                    this.drawCell(ctx, col, gy, color, CONFIG.GHOST_ALPHA);
                }
            }
            
            // 绘制当前方块
            for (const [row, col] of current.shape) {
                this.drawCell(ctx, col, row, color);
            }
        }
    }
    
    calculateGhostY(current) {
        if (!this.gameState?.board) return current.y;
        
        let ghostY = current.y;
        const board = this.gameState.board;
        
        while (true) {
            let canMove = true;
            for (const [row, col] of current.shape) {
                const newRow = row - current.y + ghostY + 1;
                if (newRow >= CONFIG.BOARD_HEIGHT || 
                    (newRow >= 0 && col >= 0 && col < CONFIG.BOARD_WIDTH && board[newRow][col] > 0)) {
                    canMove = false;
                    break;
                }
            }
            if (!canMove) break;
            ghostY++;
        }
        
        return ghostY;
    }
    
    drawCell(ctx, x, y, color, alpha = 1) {
        const cellSize = CONFIG.CELL_SIZE;
        const padding = 2;
        
        ctx.globalAlpha = alpha;
        
        // 主体
        ctx.fillStyle = color;
        ctx.fillRect(
            x * cellSize + padding, 
            y * cellSize + padding, 
            cellSize - padding * 2, 
            cellSize - padding * 2
        );
        
        // 高光效果
        const gradient = ctx.createLinearGradient(
            x * cellSize, y * cellSize,
            x * cellSize + cellSize, y * cellSize + cellSize
        );
        gradient.addColorStop(0, 'rgba(255, 255, 255, 0.3)');
        gradient.addColorStop(0.5, 'rgba(255, 255, 255, 0.1)');
        gradient.addColorStop(1, 'rgba(0, 0, 0, 0.2)');
        
        ctx.fillStyle = gradient;
        ctx.fillRect(
            x * cellSize + padding, 
            y * cellSize + padding, 
            cellSize - padding * 2, 
            cellSize - padding * 2
        );
        
        // 边框
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
        ctx.lineWidth = 1;
        ctx.strokeRect(
            x * cellSize + padding, 
            y * cellSize + padding, 
            cellSize - padding * 2, 
            cellSize - padding * 2
        );
        
        ctx.globalAlpha = 1;
    }
    
    renderNextPiece() {
        const ctx = this.nextCtx;
        const size = 25;
        
        // 清空
        ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
        ctx.fillRect(0, 0, this.nextCanvas.width, this.nextCanvas.height);
        
        if (!this.gameState || this.gameState.next === undefined) return;
        
        const nextType = this.gameState.next;
        const shape = SHAPES[nextType];
        const color = CONFIG.COLORS[nextType + 1];
        
        if (!shape) return;
        
        // 计算居中偏移
        const offsetX = this.nextCanvas.width / 2;
        const offsetY = this.nextCanvas.height / 2;
        
        // 绘制方块
        for (const [row, col] of shape) {
            const x = offsetX + col * size - size / 2;
            const y = offsetY + row * size;
            
            ctx.fillStyle = color;
            ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
            
            // 高光
            const gradient = ctx.createLinearGradient(x, y, x + size, y + size);
            gradient.addColorStop(0, 'rgba(255, 255, 255, 0.3)');
            gradient.addColorStop(1, 'rgba(0, 0, 0, 0.2)');
            ctx.fillStyle = gradient;
            ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
            
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
            ctx.strokeRect(x + 2, y + 2, size - 4, size - 4);
        }
    }
}

// 启动游戏
window.addEventListener('DOMContentLoaded', () => {
    new TetrisGame();
});
