FROM python:3.11-slim

# Install basic system utilities if your script needs them (compiled extensions, curl, etc.)
RUN apt-get update && apt-get install -y \
    curl \
    gcc \
    && rm -rf /var/lib/apt/lists/*

FROM --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

WORKDIR /app

COPY pyproject.toml uv.lock /app/

RUN uv sync --frozen --no-cache

ENV PATH="/app/.venv/bin:$PATH"

COPY . /app
