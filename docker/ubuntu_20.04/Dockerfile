FROM ubuntu:focal
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
RUN apt-get update && apt-get install -y \
    apt-utils \
    bash \
    curl \
    build-essential \
    git \
    make \
    cmake \
    libbz2-dev \
    gnuplot \
    libboost1.71-all-dev \
    libzmq3-dev

RUN curl -o- https://root.cern/download/root_v6.22.02.Linux-ubuntu20-x86_64-gcc9.3.tar.gz | tar xvfz -
ENV ROOT_DIR=/root/cmake
ARG branch=devel
RUN gcc --version
RUN git clone --depth 1 --branch ${branch} https://:@gitlab.cern.ch:8443/YARR/YARR.git
RUN cmake -S YARR -B build  -DBUILD_TESTS=on -DYARR_CONTROLLERS_TO_BUILD=all
RUN cmake --build build -j$(nproc)
RUN cmake --install build
RUN cmake --build build --target test
