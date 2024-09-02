Roadmap
=======

1) Add a timeout to camera_info_reader.py so if the camera_montitor_bin stops emitting telemetry, we assume no cameras
   are detected.

2) Raspberry PI Health, like /api/gps, add a /api/pi-health for memory, cpu, and temp stats

3) Configure the RAPI to use gpsd as it's time source and configure timezone to be UTC.



100) Use protobuf messages everywhere for encoding and decoding telemetry.


Past Items Completed
====================

* flask api/gps service the javascript can hit for ~"real-time" updates without
reloading the page.

* Add camera detection (via camera_monitor_bin)

* Emit telemetry for detected cameras for the webapp to display, expose /api/cameras

* Use config files for defining udp addresses and ports.

* Add an SVG icon for cameras we've received packets for, and a dead camera icon.  For example, suppose the camera
  goes to sleep and we stop detecting it, we should detect this condition and alert the operator of our app. For now,
  detect the camera went missing in camera_info_reader and add a connected flag to the results we send to the webapp.


Past Items Rejected
====================
