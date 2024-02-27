FROM almalinux:9
RUN dnf -y groupinstall "Development Tools"
RUN dnf config-manager --set-enabled crb
RUN dnf -y install epel-release
RUN dnf -y install make cmake gnuplot git zeromq-devel root python3-devel boost-devel
			
ENV PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
ARG branch=devel
RUN gcc --version
RUN git clone   --depth 1 --branch ${branch}  https://:@gitlab.cern.ch:8443/YARR/YARR.git
RUN cmake -S YARR -B build  -DBUILD_TESTS=on -DYARR_CONTROLLERS_TO_BUILD=all -DENABLE_PYTHON=1
RUN cmake --build build -j$(nproc)
RUN cmake --install build
RUN cmake --build build --target test
