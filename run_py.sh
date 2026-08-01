#!/bin/bash
set -e

IMAGE_NAME="python-executor"
CONTAINER_NAME="my-python-run"
SCRIPT_NAME="main.py"

PROJECT_DIR=$(pwd)

echo "    Cleaning up any old container instances..."
docker rm -f $CONTAINER_NAME 2>/dev/null || true

echo "    Building Python Docker Image ($IMAGE_NAME)..."
docker build -t $IMAGE_NAME .

echo "    Launching Python Program inside the container..."
docker run --rm -it \
    --name $CONTAINER_NAME \
    --network host \
    -v "$PROJECT_DIR:/app" \
	-v /app/.venv \
    $IMAGE_NAME python $SCRIPT_NAME "$@"

echo "    Execution complete."
