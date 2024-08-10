ARG FELIX_VERSION=latest

FROM gitlab-registry.cern.ch/atlas-itk-pixel-systemtest/itk-demo-sw/containers/felix-release:${FELIX_VERSION}
RUN mkdir /yarr/
COPY bin /yarr/bin
COPY lib /yarr/lib
COPY python /yarr/python

RUN echo -e "\n# Export yarr ENV variables" >> /config/.bashrc && \
    echo -e "export YARR_ROOT=/yarr" >> /config/.bashrc && \
    echo -e "export PATH=\${YARR_ROOT}/bin:\${PATH}" >> /config/.bashrc && \
    echo -e "export LD_LIBRARY_PATH=\${YARR_ROOT}/lib:\${LD_LIBRARY_PATH}" >> /config/.bashrc && \
    echo -e "export PYTHONPATH=\${YARR_ROOT}/python:\${YARR_ROOT}/lib" >> /config/.bashrc

WORKDIR ${HOME}
