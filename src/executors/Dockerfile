FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
	build-essential \
	cmake \
	git \
	libyaml-cpp-dev \
	libhiredis-dev \
	libcurl4-openssl-dev \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN git clone https://github.com/sewenew/redis-plus-plus.git && \
	cd redis-plus-plus && \
	mkdir build && cd build && \
	cmake -DREDIS_PLUS_PLUS_BUILD_TEST=OFF .. && \
	make && make install && \
	rm -rf /tmp/redis-plus-plus

WORKDIR /app

COPY . .

RUN make redis && mv executor executor_redis && make clean
RUN make http && mv executor executor_http && make clean

FROM ubuntu:22.04 AS runtime

WORKDIR /app

RUN apt-get update && apt-get install -y \
	libyaml-cpp0.7 \
	libhiredis0.14 \
	libcurl4 \
	python3 \
	python3-yaml \
	&& rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libredis++.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /app/executor_redis /app/executor_redis
COPY --from=builder /app/executor_http /app/executor_http

RUN printf '#!/bin/sh\n\
TYPE=$(python3 -c "import yaml; print(yaml.safe_load(open(\"/app/config.yaml\")[\"target\"][\"type\"]")\n\
exec /app/executor_${TYPE} "$@"\n' > /app/entrypoint.sh && chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
