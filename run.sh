#!/bin/bash
# =============================================================================
# run.sh — Build and launch NeonHeap (backend + frontend)
# =============================================================================
# Usage:
#   bash run.sh          — Build backend, start server, then start frontend
#   bash run.sh --backend — Build and start only the C backend server
#   bash run.sh --frontend — Start only the React frontend
# =============================================================================

set -e

PORT=8080
BACKEND_DIR="$(cd "$(dirname "$0")" && pwd)"
FRONTEND_DIR="$BACKEND_DIR/UI for MAV"

# --- Functions ---------------------------------------------------------------

build_backend() {
    echo "🔨 Compiling C backend..."
    make -C "$BACKEND_DIR" all
    echo ""
}

start_backend() {
    echo "🚀 Starting backend server on port $PORT..."
    "$BACKEND_DIR/build/memory_visualizer" --server $PORT &
    BACKEND_PID=$!
    echo "   Backend PID: $BACKEND_PID"
    echo ""
}

start_frontend() {
    echo "🎨 Starting React frontend..."
    cd "$FRONTEND_DIR"
    if [ ! -d "node_modules" ]; then
        echo "   Installing dependencies..."
        npm install
    fi
    npm run dev
}

cleanup() {
    echo ""
    echo "🛑 Shutting down..."
    if [ -n "$BACKEND_PID" ]; then
        kill $BACKEND_PID 2>/dev/null || true
    fi
    exit 0
}

# --- Main --------------------------------------------------------------------

trap cleanup INT TERM

case "${1:-all}" in
    --backend)
        build_backend
        start_backend
        wait $BACKEND_PID
        ;;
    --frontend)
        start_frontend
        ;;
    all|*)
        build_backend
        start_backend
        sleep 1
        start_frontend
        ;;
esac
