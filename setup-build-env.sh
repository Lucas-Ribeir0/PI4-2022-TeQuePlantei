#!/bin/bash

set -e

shopt -s globstar
sudo apt-get update
cd ~
#Install curl

sudo apt-get install curl -y

export PATH="/home/gitlab-runner/bin:$PATH"

#Install Arduino cli
sudo curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
echo $(ls /home/gitlab-runner/bin) 
arduino-cli config init --overwrite
arduino-cli core update-index
#Install arduino avr core
arduino-cli core install arduino:avr
sudo mv /home/gitlab-runner/bin/arduino-cli /usr/bin
