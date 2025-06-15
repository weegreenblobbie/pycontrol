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
    --include='external/gphoto2cpp/**' \
    --include='external/Makefile' \
    --exclude='external/*' \
    --exclude={'.git/','venv/','*.o','*_bin','lib*.a','*img.bz2','__pycache__/'} \
    . $RAPI_IP:pycontrol

if [ $# -eq 0 ]; then
    echo "rsync only, to build pass 'make' on the command line."
else
    ssh -t $RAPI_IP "cd pycontrol && $@"
fi
