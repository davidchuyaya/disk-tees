#!/bin/bash
sudo apt update
sudo apt -y install cmake build-essential libssl-dev pkg-config libfuse3-dev

# Install Azure CLI.
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash

# Install a newer GCC to use C++20 features.
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt -y install gcc-11 g++-11
# Change default g++ and gcc to 11.
sudo update-alternatives --remove-all cpp
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 --slave /usr/bin/g++ g++ /usr/bin/g++-9 --slave /usr/bin/gcov gcov /usr/bin/gcov-9 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-9 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-9  --slave /usr/bin/cpp cpp /usr/bin/cpp-9 && \
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 --slave /usr/bin/g++ g++ /usr/bin/g++-11 --slave /usr/bin/gcov gcov /usr/bin/gcov-11 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11  --slave /usr/bin/cpp cpp /usr/bin/cpp-11