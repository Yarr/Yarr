ARG FELIX_VERSION=latest

FROM gitlab-registry.cern.ch/atlas-itk-pixel-systemtest/itk-demo-sw/containers/felix-release:${FELIX_VERSION}
RUN mkdir /yarr/
COPY bin /yarr/bin
COPY lib /yarr/lib
COPY python /yarr/python

ENV YARR_ROOT=/yarr
ENV PATH=${YARR_ROOT}/bin:${PATH}
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${YARR_ROOT}/lib
ENV PYTHONPATH=${YARR_ROOT}/python:${YARR_ROOT}/lib

WORKDIR ${HOME}
