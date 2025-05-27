#!/usr/bin/env bash

# Setup passwordless ssh to your raspberry PI:
#
# 1) Genrate a ssh key specific for your PI:
#
#    ssh-keygen -t rsa ~/.ssh/id_rsa.rapi4
#
# 2) Copy the identify to your PI:
#
#    ssh-copy-id -i ~/.ssh/id_rsa.rapi4 user@host
#

# Rsync source code to the raspberry pi and buid.

RAPI_IP=192.168.1.173

rsync -avrt --delete \
    --exclude={".git","venv","*.o","*_bin","lib*.a","cactus-rt","*img.bz2","__pycache__"} \
    . $RAPI_IP:pycontrol

if [ "$1" = "make" ]; then
    ssh -t $RAPI_IP "cd pycontrol && make"
else
    echo "rsync only, to build pass 'make' on the command line."
fi
