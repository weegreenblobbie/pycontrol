#!/usr/bin/env bash

# This script assume passwordless ssh works and your pi is on the local network
# as pycontrol.local.
#
# Both will be true if you built a fresh PI OS image by following
#     raspberry-pi-4/README.md
#

# Rsync source code to the raspberry pi and build.

# Sync time manually.
echo "Syncing time with the PI"
ssh pycontrol.local "sudo date -s @$(date +%s) >/dev/null 2>&1"

echo "    System time: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
# Wrap the SSH command in single quotes so the Pi receives the double quotes.
echo "    Pi time:     $(ssh pycontrol.local 'date -u +"%Y-%m-%d %H:%M:%S UTC"')"

# See .rsyncignore for excluded patterns.
rsync -avrt --delete \
    --include='external/gphoto2cpp' \
    --include='external/Makefile' \
    --exclude-from='.rsyncignore' \
    . pycontrol.local:pycontrol

if [ $# -eq 0 ]; then
    echo "rsync only, to build pass 'make' on the command line."
    echo "    ./remote-build.bash make -j4"
else
    ssh -t pycontrol.local "cd pycontrol && $@"
fi
