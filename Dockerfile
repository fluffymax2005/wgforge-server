# ==========================================
# STAGE 1: Download and build dependecies
# ==========================================
FROM ubuntu:26.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/local/lib/pkgconfig

# 1. Install deps via apt
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    ninja-build \
    pkg-config \
    wget \
    git \
    libfmt-dev \
    libspdlog-dev \
    libcurl4-openssl-dev \
    libeconf-dev \
    libnl-3-dev \
    libnl-route-3-dev \
    libsqlite3-dev \
    libcli11-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Download and install Boost 1.92.0
RUN wget https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz -O /tmp/boost.tar.gz \
    && tar -xzf /tmp/boost.tar.gz -C /tmp

RUN cd /tmp/boost_1_92_0 \
    && ./bootstrap.sh --prefix=/usr/local/ \
    && ./b2 install --with-json --with-url -j$(nproc) \
    && mkdir -p /app \
    && cd /app && rm -rf /tmp/boost*    

# 3. Download and install ODB 2.6.0
RUN mkdir -p /tmp/odb-debs && cd /tmp/odb-debs \
    && wget https://codesynthesis.com/download/odb/2.6.0/ubuntu/ubuntu26.04/x86_64/odb_2.6.0-0~ubuntu26.04_amd64.deb \
    && wget https://codesynthesis.com/download/odb/2.6.0/ubuntu/ubuntu26.04/x86_64/libodb_2.6.0-0~ubuntu26.04_amd64.deb \
    && wget https://codesynthesis.com/download/odb/2.6.0/ubuntu/ubuntu26.04/x86_64/libodb-dev_2.6.0-0~ubuntu26.04_amd64.deb \
    && wget https://codesynthesis.com/download/odb/2.6.0/ubuntu/ubuntu26.04/x86_64/libodb-sqlite_2.6.0-0~ubuntu26.04_amd64.deb \
    && wget https://codesynthesis.com/download/odb/2.6.0/ubuntu/ubuntu26.04/x86_64/libodb-sqlite-dev_2.6.0-0~ubuntu26.04_amd64.deb \
    && dpkg -i *.deb \
    && rm -rf /tmp/odb-debs

ENV CC=gcc
ENV CXX=g++

# 4. Download and install libwgcpp (custom library)
RUN git clone https://github.com/fluffymax2005/libwgcpp.git /tmp/libwgcpp \
    && cmake -B /tmp/libwgcpp/build -S /tmp/libwgcpp -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=MinSizeRel \
    && cmake --build /tmp/libwgcpp/build -j $(nproc) \
    && cmake --install /tmp/libwgcpp/build -j $(nproc) \
    && rm -rf /tmp/libwgcpp

# 5. Build wgforge-server
WORKDIR /app
COPY . .

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DDOCKER_BUILD=ON

RUN cmake --build build --target wgforge-server -j$(nproc)

# ==========================================
# STAGE 2: Runtime executor
# ==========================================
FROM ubuntu:26.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    wireguard-tools \
    iproute2 \
    libsqlite3-0 \
    libfmt-dev \
    libspdlog-dev \
    libcurl4 \
    libeconf-dev \
    libnl-3-dev \
    libnl-route-3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy builder's results into runtime
COPY --from=builder /usr/local/ /usr/local/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libodb* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /app/build/daemon/wgforge-server /usr/bin/wgforge-server

# Verify linker config
RUN ldconfig

# Create wgforge-server dirs
WORKDIR /etc/wgforge
WORKDIR /var/lib/wgforge

# Volumes to store important data to
VOLUME ["/etc/wgforge", "/var/lib/wgforge"]

# Execute
ENTRYPOINT ["sh", "-c", "exec /usr/bin/wgforge-server ${INTERFACE}"]
