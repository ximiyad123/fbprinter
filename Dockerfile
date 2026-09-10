FROM debian:trixie

RUN dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        gcc-aarch64-linux-gnu \
        libpng-dev \
        libjpeg-dev \
        libgif-dev \
        zlib1g-dev \
        libpng-dev:arm64 \
        libjpeg-dev:arm64 \
        libgif-dev:arm64 \
        zlib1g-dev:arm64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

CMD ["bash"]
