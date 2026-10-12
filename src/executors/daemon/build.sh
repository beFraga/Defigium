set -e

BINARY_DIR="/tmp/Defigium/build_daemon"

cmake -B "$BINARY_DIR" -S .
cmake --build "$BINARY_DIR"
