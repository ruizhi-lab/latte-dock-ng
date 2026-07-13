#!/bin/bash
# ============================================================
# latte-dock-ng 开发者复测 debug 流程（自动化）
# 用户只需要在界面交互验证即可
# ============================================================
# 用法:
#   bash scripts/dev-debug.sh              # 默认 Debug 构建
#   bash scripts/dev-debug.sh Release      # Release 构建
#   bash scripts/dev-debug.sh Debug 16     # Debug + 16 jobs
# ============================================================
set -e

BUILD_TYPE="${1:-Debug}"
JOBS="${2:-8}"

echo "============================================"
echo "  latte-dock-ng Dev Debug Flow"
echo "  Build: ${BUILD_TYPE}, Jobs: ${JOBS}"
echo "============================================"

# Step 1: 杀掉旧 latte 进程
echo "[1/4] 杀掉旧 latte-dock-ng 进程..."
pkill -f latte-dock-ng 2>/dev/null || true
sleep 1

# Step 2: 用户态安装修改的代码
echo "[2/4] 构建 & 安装 (${BUILD_TYPE})..."
bash install.sh --user --jobs "${JOBS}" "${BUILD_TYPE}"

# Step 3: 移除旧的 latte 日志
echo "[3/4] 清理旧日志..."
rm -f /tmp/latte-ng.log

# Step 4: 启动新的 latte 进程
echo "[4/4] 启动 latte-dock-ng (debug 模式)..."
source ~/.config/latte-dock-ng/dev-env.sh
nohup ~/.local/bin/latte-dock-ng --replace --debug 2>&1 | tee /tmp/latte-ng.log &

echo ""
echo "============================================"
echo "  latte-dock-ng 已启动!"
echo "  日志文件: /tmp/latte-ng.log"
echo "  用户请在界面进行交互验证"
echo "============================================"
