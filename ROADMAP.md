Roadmap
=======

Make it work
^^^^^^^^^^^^

- Define event file format
- Add event selection UI to select 1 of these:
 -- 2026-03 linar eclipse
 -- 2026-08 total solar eclipse
 -- Custom

- Plumb computed contact times into UI, camera control

- Define camera schedule format

- Use pydandic to validate all yaml config files

- Implement and test these workflows:
    - pick event, clear any camera schedule
    - pick camera schedule file, validate

- Add a timeout to camera_info_reader.py so if the camera_montitor_bin stops emitting
  telemetry, we assume no cameras are detected.

- Detect if camera is in Movie Mode and make camera status work while in that mode

- Raspberry PI Health, like /api/gps, add a /api/pi-health for memory, cpu, and temp stats

- Add automated camera frames-per-second auto testing to find limit.

- Add event simulation mode for local testing
    - override location
    - override time or event

- Auto start app on rpi boot


Make it work well
^^^^^^^^^^^^^^^^^

- unit tests for everything

- Add a graphical system diagram for documentation.

- Add documentation.

- Figure out an easy and smooth software update straegy.


MAke it work well for others
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- self signed certs for https comms with raspberry pi

- mobile phone apps for user UI control

- phone and rpi auto-detect and enrollment to auto setup https cert
    - enable random ip & port assignments for multi pycontrols in the field to not
      conflict

- Host event files from github.com?  Or from anywhere on the web.

- Add support for Canon cameras
- Add support for Sony cameras


Past Items Completed
====================

* Update camera_contro_bin to handle mapping serial number to alias.
** webapp/cam_info_reader.py to command serial:alias updates

* Add a yaml config file for mapping camera serial numbers to short names
* Add flask UI to edit mapping.

* Configure the RAPI to use gpsd as it's time source and configure timezone to be UTC.
  Update ansible/playbook with gpsd and chrony config settings.
  Manually verified by:
  1) Removing NTP from chrony config and stopping chronyd
  2) Setting pi's time to 1 day off
  3) Configuring gpsd and chronyd
  4) Starting chronyd and watching the system time sync with gps

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
