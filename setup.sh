#!/bin/bash
vercomp() {
    if [[ $1 == $2 ]]
    then
        return 0
    fi
    local IFS=.
    local i ver1=($1) ver2=($2)
    # fill empty fields in ver1 with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
    do
        ver1[i]=0
    done
    for ((i=0; i<${#ver1[@]}; i++))
    do
        if [[ -z ${ver2[i]} ]]
        then
            # fill empty fields in ver2 with zeros
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]}))
        then
            return 1
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]}))
        then
            return 2
        fi
    done
    return 0
}

CMAKE_LEAST="2.8.12"
GCC_LEAST="4.4.7"
GCC_VER=`gcc --version | head -n1 | cut -d" " -f4`
CMAKE_VER=`cmake --version | head -n1 | cut -d" " -f3`
vercomp $GCC_VER $GCC_LEAST
if [ $? -eq 2 ]
then
    echo "gcc version too low (current:$GCC_VER,require:$GCC_LEAST)"
else
    echo "gcc version check pass (current:$GCC_VER,require:$GCC_LEAST)"
fi

vercomp $CMAKE_VER $CMAKE_LEAST
if [ $? -eq 2 ]
then
    echo "cmake version too low (current:$CMAKE_VER,require:$CMAKE_LEAST)"
else
    echo "cmake version check pass (current:$CMAKE_VER,require:$CMAKE_LEAST)"
fi


grep -q -F 'deb http://ppa.launchpad.net/aziotsdklinux/ppa-azureiot/ubuntu vivid main' /etc/apt/sources.list || sudo sh -c "echo 'deb http://ppa.launchpad.net/aziotsdklinux/ppa-azureiot/ubuntu vivid main' >> /etc/apt/sources.list"
grep -q -F 'deb-src http://ppa.launchpad.net/aziotsdklinux/ppa-azureiot/ubuntu vivid main' /etc/apt/sources.list || sudo sh -c "echo 'deb-src http://ppa.launchpad.net/aziotsdklinux/ppa-azureiot/ubuntu vivid main' >> /etc/apt/sources.list"
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys FDA6A393E4C2257F
sudo apt-get update
sudo apt-get install -y azure-iot-sdk-c-dev cmake libcurl4-openssl-dev

git clone git://git.drogon.net/wiringPi
cd ./wiringPi
./build
cd ..


if [[ $1 == "--simulated-data" ]]; then
    echo -e "Using simulated data"
    sed -i 's/#define SIMULATED_DATA 0/#define SIMULATED_DATA 1/' config.h
fi

cmake . && make

