#!/usr/bin/env bash

# 遇到错误、未定义变量或管道失败时立即退出。
set -euo pipefail

# 允许通过命令行参数覆盖默认的两个虚拟串口路径。
LEFT_PORT="${1:-/tmp/ttyMCU}"
RIGHT_PORT="${2:-/tmp/ttyHOST}"

# 该脚本依赖的外部工具只有 `socat`。
if ! command -v socat >/dev/null 2>&1; then
  echo "socat is not installed. Please install it first, for example: sudo apt install socat" >&2
  exit 1
fi

# 重新创建前先删除上一次运行遗留的软链接。
rm -f "$LEFT_PORT" "$RIGHT_PORT"

echo "Creating virtual serial pair:"
echo "  $LEFT_PORT"
echo "  $RIGHT_PORT"
echo
echo "Press Ctrl+C to stop."

# 保持进程在前台运行，这样虚拟串口对会一直有效。
exec socat -d -d \
  pty,raw,echo=0,link="$LEFT_PORT" \
  pty,raw,echo=0,link="$RIGHT_PORT"
