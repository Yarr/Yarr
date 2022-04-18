FROM centos:8
RUN yum -y groupinstall "Development Tools"
RUN yum -y install epel-release
RUN yum -y install \
    make cmake \
    gnuplot \
    gcc-toolset-9-gcc-c++ \
    gcc-toolset-9-gcc-c++ \
    gcc-toolset-9-annobin \
    git \
    zeromq-devel \
    bzip2-devel \
    boost-devel \
    root \
    python36-devel

ARG branch=devel_pyyarr
ENV PATH=/opt/rh/gcc-toolset-9/root/usr/bin:$PATH
RUN git clone --depth 1 --branch ${branch}  https://:@gitlab.cern.ch:8443/YARR/YARR.git
RUN gcc --version
RUN cmake -S YARR -B build  -DBUILD_TESTS=on -DYARR_CONTROLLERS_TO_BUILD=all -DENABLE_PYTHON=1
RUN cmake --build build -j${nproc}
RUN cmake --install build
RUN cmake --build build --target test
