FROM centos:7
RUN yum -y groupinstall "Development Tools"
RUN yum -y install epel-release centos-release-scl
RUN yum -y install \
    make \
    cmake3 \
    gnuplot \
    git \
    devtoolset-9 \
    zeromq-devel \
    bzip2-devel \
    boost-devel \
    root \
    python3-devel
			
ENV PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
ARG branch=devel
RUN gcc --version
RUN git clone   --depth 1 --branch ${branch}  https://:@gitlab.cern.ch:8443/YARR/YARR.git
RUN cmake3 -S YARR -B build  -DBUILD_TESTS=on -DYARR_CONTROLLERS_TO_BUILD=all -DENABLE_PYTHON=1
RUN cmake3 --build build -j$(nproc)
RUN cmake3 --install build
RUN cmake3 --build build --target test
