FROM ubuntu:latest
# This Dockerfile creates https://hub.docker.com/r/insufficientlycaffeinated/bob
# Currently there's no automatic build set up for this, so changes to this file
# should be followed by rebuilding the container and pushing it to dockerhub.
# Luckily that won't happen very often (because changes can be made in Circle
# faster)
ARG DEBIAN_FRONTEND=noninteractive


RUN apt update \
    && apt -y install \
        llvm-10-dev \
        libz-dev \
        build-essential \
        gcc-9 \
        g++-9 \
        git \
        make \
        cmake \
        libgtest-dev \
        python3-distutils \
        libfmt-dev \
        libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# This is all done in 1 run command so that we don't end up storing the
# intermediate layers in the final dockerfile
RUN git clone https://github.com/Z3Prover/z3.git && \
    mkdir -p z3/build && \
    cd z3/build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && \
    rm -rf z3
