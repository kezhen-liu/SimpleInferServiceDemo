#!/bin/bash

echo "Installing from apt"
sudo -E apt-get update
sudo -E apt-get install cmake gcc g++

echo "Checking if openvino installed"
if [ ! -d /opt/intel/openvino ]; then
    echo "Checking openvino installer"
    openvinoInstaller=$(find . -maxdepth 1 -name "*openvino_toolkit_p_*" -type f -printf "%f\n")
    if [ ! -f ${openvinoInstaller} ]; then
        echo ""
        echo "Please download openvino installer and place at where this installer resides"
        exit 0
    fi

    echo "extract openvino"

    mkdir -p OpenVinoInstaller &&
    tar -xvf ${openvinoInstaller} -C OpenVinoInstaller --strip-components 1 && 
    cd OpenVinoInstaller &&
    sudo -E ./install_openvino_dependencies.sh &&
    sudo -E ./install.sh --cli-mode -s silent.cfg --accept_eula --components ALL &&
    sudo usermod -a -G video $(whoami)

    echo "Done openvino"
fi


