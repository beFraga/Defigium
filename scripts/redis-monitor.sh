#!/usr/bin/env bash
# =============================================================================
# redis-monitor.sh
# Starts a Redis server on a given port and opens redis-cli MONITOR in a
# second terminal pane (or fallback to the same shell).
#
# Usage:
#   ./scripts/redis-monitor.sh [PORT]
#
# Examples:
#   ./scripts/redis-monitor.sh          # uses default port 6379
#   ./scripts/redis-monitor.sh 6380     # uses port 6380
# =============================================================================

set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────────
DEFAULT_PORT=6379
PORT="${1:-$DEFAULT_PORT}"
REDIS_PID_FILE="/tmp/redis-monitor-${PORT}.pid"
REDIS_LOG_FILE="/tmp/redis-monitor-${PORT}.log"

# ── Helpers ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RESET='\033[0m'

log()  { echo -e "${CYAN}[redis-monitor]${RESET} $*"; }
ok()   { echo -e "${GREEN}[redis-monitor]${RESET} $*"; }
warn() { echo -e "${YELLOW}[redis-monitor]${RESET} $*"; }
err()  { echo -e "${RED}[redis-monitor]${RESET} $*" >&2; }

# ── Dependency check ──────────────────────────────────────────────────────────
check_deps() {
    local missing=()
    for cmd in redis-server redis-cli; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        err "Missing required commands: ${missing[*]}"
        err ""
        err "Install Redis with one of:"
        err "  Ubuntu/Debian : sudo apt install redis-server"
        err "  Fedora/RHEL   : sudo dnf install redis"
        err "  Arch          : sudo pacman -S redis"
        err "  macOS         : brew install redis"
        exit 1
    fi
}

# ── Port validation ───────────────────────────────────────────────────────────
validate_port() {
    if ! [[ "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
        err "Invalid port: '$PORT'. Must be an integer between 1 and 65535."
        exit 1
    fi

    if lsof -iTCP:"$PORT" -sTCP:LISTEN -t &>/dev/null 2>&1; then
        warn "Port $PORT is already in use."
        # Check if it's already a Redis instance
        if redis-cli -p "$PORT" ping 2>/dev/null | grep -q "PONG"; then
            ok "Detected an existing Redis on port $PORT — attaching monitor only."
            REDIS_ALREADY_RUNNING=true
        else
            err "Port $PORT is occupied by a non-Redis process. Choose a different port."
            exit 1
        fi
    else
        REDIS_ALREADY_RUNNING=false
    fi
}

# ── Cleanup on exit ───────────────────────────────────────────────────────────
cleanup() {
    echo ""
    log "Shutting down…"

    # Kill the monitor process if alive
    if [[ -n "${MONITOR_PID:-}" ]] && kill -0 "$MONITOR_PID" 2>/dev/null; then
        kill "$MONITOR_PID" 2>/dev/null || true
    fi

    # Shut down Redis only if we started it
    if [[ "${REDIS_ALREADY_RUNNING:-false}" == "false" ]]; then
        if [[ -f "$REDIS_PID_FILE" ]]; then
            local pid
            pid=$(cat "$REDIS_PID_FILE")
            if kill -0 "$pid" 2>/dev/null; then
                log "Stopping Redis (PID $pid) on port $PORT…"
                redis-cli -p "$PORT" shutdown nosave 2>/dev/null || kill "$pid" 2>/dev/null || true
            fi
            rm -f "$REDIS_PID_FILE"
        fi
    fi

    ok "Done. Goodbye!"
}

trap cleanup EXIT INT TERM

# ── Start Redis ───────────────────────────────────────────────────────────────
start_redis() {
    log "Starting Redis on port $PORT…"
    redis-server \
        --port "$PORT" \
        --daemonize yes \
        --logfile "$REDIS_LOG_FILE" \
        --pidfile "$REDIS_PID_FILE" \
        --loglevel notice \
        --protected-mode no

    # Wait until Redis responds
    local retries=10
    while (( retries-- > 0 )); do
        if redis-cli -p "$PORT" ping 2>/dev/null | grep -q "PONG"; then
            ok "Redis is up on port $PORT  (PID $(cat "$REDIS_PID_FILE"))"
            return 0
        fi
        sleep 0.3
    done

    err "Redis failed to start. Check logs: $REDIS_LOG_FILE"
    exit 1
}

# ── Run monitor ───────────────────────────────────────────────────────────────
run_monitor() {
    echo ""
    ok "============================================================"
    ok "  Redis Monitor  —  port $PORT"
    ok "  All commands sent to Redis will appear below."
    ok "  Press Ctrl+C to stop."
    ok "============================================================"
    echo ""

    # redis-cli MONITOR streams every command processed by the server
    redis-cli -p "$PORT" monitor
}

# ── Main ──────────────────────────────────────────────────────────────────────
main() {
    log "redis-monitor.sh — port=${PORT}"

    check_deps
    validate_port

    if [[ "$REDIS_ALREADY_RUNNING" == "false" ]]; then
        start_redis
    fi

    log "Redis log  : $REDIS_LOG_FILE"
    log "Redis pid  : $REDIS_PID_FILE"

    run_monitor
}

main
