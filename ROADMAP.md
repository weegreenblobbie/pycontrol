Roadmap
=======

Make it work
^^^^^^^^^^^^

- Add burstnumber property.

- Turns out not all cameras support shutterspeed2, avialablephotos.
  - I'll have to make a map for fraction shutter string shutterspeed.
  - Or make a branch of libgphoto2 to add these props to the Z8 camera.

- Loading a camera sequence that fails, is not fed back to UI

- Add message about when it's okay to turn off camera

- Look into how many files on the memory card

- Add the trigger command to unit tests.

- Disable camera table buttons if state == executing

- Fix these astropy warnings:
    WARNING: TimeDeltaMissingUnitWarning: Numerical value without unit or explicit format passed to TimeDelta, assuming days [astropy.time.core]
    WARNING: Tried to get polar motions for times after IERS data is valid. Defaulting to polar motion from the 50-yr mean for those. This may affect precision at the arcsec level. Please check your astropy.utils.iers.conf.iers_auto_url and point it to a newer version if necessary. [astropy.coordinates.builtin_frames.utils]
    WARNING: TimeDeltaMissingUnitWarning: Numerical value without unit or explicit format passed to TimeDelta, assuming days [astropy.time.core]

- Add gps time and system time delta to the gps status line, to help verify if chrony
  is correctly syncing system time.

- Add camera connection counter so one can observe if it's constantly reconnecting (see this
  with my own eyes, rebooting the pi fixed it.)

- Add and test more camera channels:
    z7.autofocus         manual
    z7.capturetarget     memorycard
    z7.date              now
    z7.mode              manual

    I don't think we need any of the above.  Syncing the date would be nice, it would be
    < 100 milliseconds from GPS given the latencies.

    chronyc -n sources
    MS Name/IP address         Stratum Poll Reach LastRx Last sample
    ===============================================================================
    #* GPS                           0   4   377    13   -168us[-1511us] +/-   50ms

    So we are about +/- 51 milliseconds from GPS time.  So that's pretty darn good.

- Implement and test these workflows:
    - pick event, clear any camera schedule
    - pick camera schedule file, validate

- Detect if camera is in Movie Mode and make camera status work while in that mode

- Raspberry PI Health, like /api/gps, add a /api/pi-health for memory, cpu, and temp stats

- Add automated camera frames-per-second auto testing to find limit.

- Auto start app on rpi boot

- Connect phone to WPA2 secure, ad-hoc, wireless network provided by the pi

- Upload event and sequnce files using the webapp.


Make it work well
^^^^^^^^^^^^^^^^^

- unit tests for everything

- Add a graphical system diagram for documentation.

- Add documentation.

- Figure out an easy and smooth software update straegy.

- Consider installing the Adafruit GPS modules with PPS:
    https://austinsnerdythings.com/2021/04/19/microsecond-accurate-ntp-with-a-raspberry-pi-and-pps-gps/

    Although, according to chronyc -n sources, were only about 50 milliseconds off:

    chronyc -n sources

    MS Name/IP address         Stratum Poll Reach LastRx Last sample
    ===============================================================================
    #* GPS                           0   4   377    13   -168us[-1511us] +/-   50ms

MAke it work well for others
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- self signed certs for https comms with raspberry pi ?  Maybe adhoc wireless with WPA2 is
  all that's needed.

- Official deb packages for raspberry pi distribution ?

- mobile phone apps for user UI control

- phone and rpi auto-detect and enrollment to auto setup https cert
    - enable random ip & port assignments for multi pycontrols in the field to not
      conflict

- Host event files from github.com?  Or from anywhere on the web.

- Add support for Canon cameras
- Add support for Sony cameras


Past Items Completed
====================

* Persist the event and camera selection in a config file, so reloading the app
  auto loads the last selected event and sequence file.


* Display camera id instead of serial when clicking on camera settings to change.

* Camera Event sequence ETA should include number of days.

* Preserve event ordering on the web UI.

* Consider using https://github.com/nlohmann/json/tree/develop for sending telem in JSON format
* Use a json parser in the unit test objects (uto) files so each filed can be
  asserted to provide more precise error messages.

* Print a Use make VERBOSE=1 to turn on verbose builds message in my Makefiles

* Add a manual trigger button to the camera table.

* Add a memory cache on PycontrolApp.read_choices

* Add intelligent sorting of camera properties for read_choices
  - shutterspeed2
  - iso
  - fstop
  I just stopped iterating over the unordered_map and instead always populate a vector
  that's stored in libgphoto2's memory.

* When I load a camera sequence, i expected the sequence table to get populated in about 1
  second.  Currently takes a bunch of time.
  Solved by immediatlety sending telemtry in response to commanding.

* When control state != executig, allow manual manipulation of the camera settings
** easy to view what choices a field has

* Display the top 10 camera sequence events.

* CameraControl commanding is currently kind of ad hock, each type of command on
  it's own port, unify this:
  CameraControl listents for commands on a single port
  cammands must specify a uniue sequence ids so if the command has aleady been
  processed can be ignored
  commands must specify a command type, followed by command parameters so based on
  the type, CameraControl can process it.
  Add a reandme in docs to document the commands.

* Convert all channels and values to lowercase
* Internally map lowecase choice maps to camera case.
* Internally convert all event ids to lowercase.

* Run Sim button should work multiple times when Event Id is an empty string
** Also fix the integer intput widgest, no silly up/down buttons, allow one to type in
   and fix the issue if the mouse up event happens outside the diaglog, don't close it.

* Add event simulation mode for local testing
** override location
** override time or event

* Cache read_property widgets.

* See if I can speed up scan_cameras() & gphoto2cpp.h.
** remove as many memory allocations as possible.
** cache choices for setting widget values using a hash map in write_property() to
   change it from a O(N) operation to O(1)

X Try reusing the widget cache when reconnecting a camera, are widgets tied to
  the camera pointer or do they work with any camera pointer?
  If it can't be reused, need to add a update_cache(old_ptr, new_ptr);

  X - this is not how libgphoto2 works, see the gphoto2cpp diagram in the docs
  folder.

* Can no omit the Event Id in the Run Sim dialog, this computes the contact times for the
  real event!  Dialog still needs to javascript love to fix reuse case.

* Many fixes to gphoto2cpp.h, now can do basic settings and trigger capture error free!

* Much improved GNU make setup, sparse, colorful output, added common install prefix for
  external libs, simplying include and link flags.

* Fixed a bug in gps_reader, where the TPV report from the GPS antenna had zeros for
  latitude, longitude but non-zero ECEF x,y,z, so to work around this, used pyproj to
  convert ECEF x,y,z to lat, long, alt if lat, long were zero.

* When we reach the end of all camera sequences, we should emit messages with empty events so
  the webapp clears the event table.

* When I stop the sim by pressing Stop Sim, this should send the reset signal to
  CameraControl.

* Plumb computed contact times into camera control, and run though the camera sequence,
  emitting udp packets.

* Add simulation buttons below the event table, start and abort
  * simulate: location: (lat, log, alt), time: -60.0 C2, persist these in a local file.
  * takes a snapshot of the current gps data, so residuals can be added for
  realistic changes over time
  * camera_control will continue to use system time, so in simulation mode, we
  apply a time offset to the calculated event times so camera control believe the events are
  about to fire.

* Define camera schedule format

* Cache computed contact times (IN MEMORY)

* Define event file format
* Add event selection UI to select 1 of these:
 -- 2026-03 linar eclipse
 -- 2026-08 total solar eclipse
 -- Custom

* Plumb computed contact times into UI

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
