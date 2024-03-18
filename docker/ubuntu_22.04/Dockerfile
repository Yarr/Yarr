FROM ubuntu:jammy
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
    gnuplot \
    libboost1.74-all-dev \
    python3-dev


RUN curl -o- https://root.cern/download/root_v6.26.14.Linux-ubuntu22-x86_64-gcc11.4.tar.gz | tar xvfz -
ENV ROOT_DIR=/root/cmake
ARG branch=devel
RUN gcc --version
RUN git clone --depth 1 --branch ${branch} https://:@gitlab.cern.ch:8443/YARR/YARR.git
RUN cmake -S YARR -B build  -DBUILD_TESTS=on -DENABLE_PYTHON=1 -DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;StarEmu;Fei4Emu;Rd53aEmu;Bdaq;ItsdaqFW"
RUN cmake --build build -j$(nproc)
RUN cmake --install build
RUN cmake --build build --target test
