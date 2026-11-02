FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    ca-certificates \
    libcap2-bin \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY tests/ ./tests/

RUN cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    && cmake --build build -j$(nproc) \
    && ./build/edgelink_tests

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    libcap2-bin \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/edgelink_gateway /usr/local/bin/
COPY --from=builder /app/build/edgelink_simulator /usr/local/bin/
COPY --from=builder /app/build/edgelink_diagnostics /usr/local/bin/
COPY --from=builder /app/build/edgelink_tests /usr/local/bin/

RUN setcap cap_net_raw+ep /usr/local/bin/edgelink_diagnostics

EXPOSE 9001/tcp
EXPOSE 9002/udp

ENTRYPOINT ["/usr/local/bin/edgelink_gateway"]
CMD ["9001", "9002"]
