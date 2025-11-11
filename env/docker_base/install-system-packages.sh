#!/bin/bash

# Install system packages for RTA Data Processor build environment

set -e

echo "Installing system packages..."

# Configure apt to retry downloads and use alternative mirrors
echo 'Acquire::Retries "3";' > /etc/apt/apt.conf.d/80-retries
echo "deb mirror://mirrors.ubuntu.com/mirrors.txt jammy main restricted universe multiverse" > /etc/apt/sources.list
echo "deb mirror://mirrors.ubuntu.com/mirrors.txt jammy-updates main restricted universe multiverse" >> /etc/apt/sources.list

# Update and install packages
apt-get update
apt-get install -y \
    build-essential \
    gcc-11 \
    g++-11 \
    cmake \
    git \
    python3-dev \
    python3-pip \
    libboost-all-dev \
    libboost-iostreams-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-regex-dev \
    libzmq3-dev \
    libavro-dev \
    libjsoncpp-dev \
    libhdf5-dev \
    libhdf5-hl-cpp-100 \
    libtinyxml2-dev \
    libssl-dev \
    liblzma-dev \
    wget \
    curl \
    rsync \
    vim \
    netcat \
    chrony \
    pkg-config \
    ninja-build

# Clean up
apt-get clean
rm -rf /var/lib/apt/lists/*

# Set GCC 11 as default
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

echo "System packages installed successfully"

