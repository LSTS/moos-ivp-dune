#!/bin/bash

# get all packages required for compiling
sudo apt-get install subversion git g++ cmake make

# clone and compile dune
git clone https://github.com/LSTS/dune.git && mkdir -p dune/build && cd dune/build && cmake .. && make -j4 && sudo make install

# checkout MOOS-IVP
cd ~ && svn co https://oceanai.mit.edu/svn/moos-ivp-aro/releases/moos-ivp-14.7.1 moos-ivp

# compile all headless binaries
export IVP_BUILD_GUI_CODE=OFF
cd moos-ivp && ./build-moos.sh -j4 && ./build-ivp.sh -j4

# add MOOS to system path
echo export PATH=$PATH:~/moos-ivp/ivp/bin >> ~/.bashrc
echo export PATH=$PATH:~/moos-ivp/MOOS/MOOSBin >> ~/.bashrc

# clone moos-ivp-dune
git clone https://zepinto@bitbucket.org/zepinto/moos-ivp-dune.git
cd moos-ivp-dune && cmake . && make -j4
