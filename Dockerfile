FROM ubuntu:latest
# This Dockerfile creates https://hub.docker.com/r/insufficientlycaffeinated/bob
# Currently there's no automatic build set up for this, so changes to this file
# should be followed by rebuilding the container and pushing it to dockerhub.
# Luckily that won't happen very often (because changes can be made in Circle
# faster)
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt -y install \
    llvm-dev \
    libz-dev \
    build-essential \
    gcc \
    g++ \
    git \
    make \
    cmake \
    libgtest-dev \
    python3-distutils \
    libfmt-dev \
    libboost-all-dev
RUN git clone https://github.com/Z3Prover/z3.git
RUN mkdir z3/build && cd z3/build && cmake .. && make && make install
RUN rm -rf z3
