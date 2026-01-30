# 多阶段构建 - 编译阶段
FROM --platform=$BUILDPLATFORM alpine:3.19 AS builder

# 安装编译工具
RUN apk add --no-cache \
    g++ \
    cmake \
    make \
    linux-headers

# 复制源代码
WORKDIR /app
COPY CMakeLists.txt .
COPY src/ src/

# 编译
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# 运行阶段 - 使用最小镜像
FROM alpine:3.19

# 安装运行时依赖
RUN apk add --no-cache libstdc++ libgcc

# 从构建阶段复制可执行文件
COPY --from=builder /app/build/tetris_server /usr/local/bin/

# 暴露端口
EXPOSE 8080

# 启动服务
CMD ["tetris_server"]
