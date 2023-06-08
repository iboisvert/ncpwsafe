# FROM mcr.microsoft.com/devcontainers/base:ubuntu
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get install -y --no-install-recommends cmake gcc g++ gdb libncurses-dev libicu-dev nettle-dev \
#     && rm -rf /var/lib/apt/lists/*
# ENV DEBUGINFOD_URLS=https://debuginfod.ubuntu.com/
# FROM mcr.microsoft.com/devcontainers/base:alpine
# RUN apk update \
#     && apk add cmake gcc g++ gdb ncurses-dev ncurses-static icu-dev icu-static nettle-dev nettle-static
FROM fedora:38
RUN dnf makecache \
    && dnf install -y git cmake gcc gcc-c++ libasan libubsan gdb ncurses-devel libicu-devel nettle-devel glog-devel cpptoml-devel gtest-devel \
    && dnf clean -y all \
    && groupadd --gid 1000 vscode \
    && useradd --uid 1000 --gid 1000 -G wheel -s /bin/bash -m vscode \
    && sh -c "echo \"%wheel ALL=(ALL) NOPASSWD: ALL\" > /etc/sudoers.d/10-wheel"
USER 1000
