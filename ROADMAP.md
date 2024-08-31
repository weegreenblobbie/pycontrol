Roadmap
=======

1) Add a timeout to camera_info_reader.py so if the camera_montitor_bin stops emitting telemetry, we assume no cameras
   are detected.

2) Add an SVG icon for cameras we've received packets for, and a dead camera icon.  For example, suppose the camera
   goes to sleep and we stop detecting it, we should detect this condition and alert the operator of our app. For now,
   detect the camera went missing in camera_info_reader and add a connected flag to the results we send to the webapp.

3) Look into how to get notified of USB unplug, plug events to trigger camera scan insted of using
   a loop.  We could spawn a process and monitor it's stdout:

        udevadm monitor --kernel --property

        KERNEL[28943.355864] add      /devices/pci0000:00/0000:00:12.2/usb1/1-3/1-3:1.0 (usb)
        ACTION=add
        DEVPATH=/devices/pci0000:00/0000:00:12.2/usb1/1-3/1-3:1.0
        SUBSYSTEM=usb
        DEVTYPE=usb_interface
        PRODUCT=4b0/44c/162
        TYPE=0/0/0
        INTERFACE=6/1/1
        MODALIAS=usb:v04B0p044Cd0162dc00dsc00dp00ic06isc01ip01in00
        SEQNUM=12802

        KERNEL[28943.356478] bind     /devices/pci0000:00/0000:00:12.2/usb1/1-3 (usb)
        ACTION=bind
        DEVPATH=/devices/pci0000:00/0000:00:12.2/usb1/1-3
        SUBSYSTEM=usb
        DEVNAME=/dev/bus/usb/001/047
        DEVTYPE=usb_device
        DRIVER=usb
        PRODUCT=4b0/44c/162
        TYPE=0/0/0
        BUSNUM=001
        DEVNUM=047
        SEQNUM=12803
        MAJOR=189
        MINOR=46

        KERNEL[28954.719632] remove   /devices/pci0000:00/0000:00:12.2/usb1/1-3/1-3:1.0 (usb)
        ACTION=remove
        DEVPATH=/devices/pci0000:00/0000:00:12.2/usb1/1-3/1-3:1.0
        SUBSYSTEM=usb
        DEVTYPE=usb_interface
        PRODUCT=4b0/44c/162
        TYPE=0/0/0
        INTERFACE=6/1/1
        MODALIAS=usb:v04B0p044Cd0162dc00dsc00dp00ic06isc01ip01in00
        SEQNUM=12804

        KERNEL[28954.720299] unbind   /devices/pci0000:00/0000:00:12.2/usb1/1-3 (usb)
        ACTION=unbind
        DEVPATH=/devices/pci0000:00/0000:00:12.2/usb1/1-3
        SUBSYSTEM=usb
        DEVNAME=/dev/bus/usb/001/047
        DEVTYPE=usb_device
        PRODUCT=4b0/44c/162
        TYPE=0/0/0
        BUSNUM=001
        DEVNUM=047
        SEQNUM=12805
        MAJOR=189
        MINOR=46

        KERNEL[28954.720879] remove   /devices/pci0000:00/0000:00:12.2/usb1/1-3 (usb)
        ACTION=remove
        DEVPATH=/devices/pci0000:00/0000:00:12.2/usb1/1-3
        SUBSYSTEM=usb
        DEVNAME=/dev/bus/usb/001/047
        DEVTYPE=usb_device
        PRODUCT=4b0/44c/162
        TYPE=0/0/0
        BUSNUM=001
        DEVNUM=047
        SEQNUM=12806
        MAJOR=189
        MINOR=46

udevadm source lives here:
    https://github.com/systemd/systemd/blob/main/src/udev/udevadm-monitor.c

4) Raspberry PI Health, like /api/gps, add a /api/pi-health for memory, cpu, and temp stats

5) Use protobuf messages everywhere for encoding and decoding telemetry.

6) Use config files for defining udp addresses and ports.

7) Configure the RAPI to use gpsd as it's time source and configure timezone to be UTC.



Past Items Completed
====================

* flask api/gps service the javascript can hit for ~"real-time" updates without
reloading the page.

* Add camera detection (via camera_monitor_bin)

* Emit telemetry for detected cameras for the webapp to display, expose /api/cameras

Past Items Rejected
====================
