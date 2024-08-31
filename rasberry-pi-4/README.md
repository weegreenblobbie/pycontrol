User the Raspberry PI Imager tool to configure PI OS
====================================================

Install via snap on Unbutu:

    sudo snap install rpi-imager

Then run rpi-imager

Configure the following things:

1) Set a hostname, I used pycontrol

2) Set a username, I used nhilton

3) Set a user password

4) Set up wifi network, so we can ssh into the pi after it boots

Mount the resulting SD card once rpi-image is done.

Edit rootfs/etc/networking/interfaces:

    auto eth0
    allow-hotplug eth0
    iface eth0 inet dhcp

Save and and unmount.

Verify it boots, gets an IP over ethernet, and you can ssh in via the password.

Once that works, shutdown so we can snapshot the image.

Mount on Linux and dump disk image:

    # unmount first if it auto mounted.
    sudo umount /dev/sdb{1,2}

Dump image:

    sudo dd if=/dev/sdb of=rpi4-64-bit-fresh.img status=progress

Power up
========

Plug in the RAPI and wait approx 10 minutes for the first boot to finish.  Eventually 'pycontrol' will show up on the
network and you should be able to ssh into the RAPI:

Now you can ssh into your PI:

    ssh nhilton@192.168.1.173
    password: *****

    uname -a
    Linux raspberrypi 6.6.31+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.6.31-1+rpt1 (2024-05-29) aarch64 GNU/Linux

Confifuring via Ansible
=======================

Add your rasberry pi's IP to /etc/hosts, then add your host to the inventory:

    cd ansible
    nano inventory
    [pycontrol]
    192.168.1.173    pycontrol

Now run the playbook:

    . venv/bin/activate
    ansible-playbook -i inventory.ini playbook.yaml -k


Removing desktop environment:
    sudo apt purge xserver* lightdm* raspberrypi-ui-mods vlc* lxde* chromium* desktop* gnome* gstreamer* gtk* hicolor-icon-theme* lx* mesa*
    sudo apt autoremove


Power down and create a backup image for the fresh OS image to start from:

    # On the RAPI:
    sudo shutdown -h

    # On your linux box, wait for the RAPI to stop responding:
    ping 192.168.1.173

No insert the SD card into your card reader, then make a backup and compress on the fly:

    sudo bash -c "dd if=/dev/sdb | bzeip2 > rapi4-fresh-configured.img.bz2"
