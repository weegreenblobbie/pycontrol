Roadmap
=======

Make it work
^^^^^^^^^^^^

- Port some of my scripts from https://github.com/weegreenblobbie/eoscript into this repo under scripts/
  - for example gemini ai and I wrote a complicated script that reads my sequence file and the exif data from
    the captured raw files and caught a bug in my sequence!  libgphoto2 doesn't have a z8 shutter speed of
    1/4000!  So my set shutter speed called must have failed and was ignored at runtime.  maybe when loading
    a sequence, i have to validate every channel setting with the camera?

    python verify-sequence.py spain-2026.seq /tmp/z8-capture3.txt
    --- Sequence Alignment ---
    Total Exif Files:        142
    Ignored Pre-shots:       0
    Expected Sequence Shots: 142
    Actual Sequence Shots:   142

    SEQUENCE TRIGGER LINE                  EXIF LOG LINE                                                                   DELTA (s)
    -------------------------------------  ----------------------------------------------------------------------------  -----------
    c2    37.75    z8.trigger           1  _NIK3426.NEF, 2026-05-27 15:12:31.045000, NIKON Z 8, 1/250 , 8   , 64  , RAW        0.000
    c2    39.00    z8.trigger           1  _NIK3427.NEF, 2026-05-27 15:12:32.072000, NIKON Z 8, 1/1000, 8   , 64  , RAW       -0.223
    c2    39.25    z8.trigger           1  _NIK3428.NEF, 2026-05-27 15:12:32.095000, NIKON Z 8, 1/1000, 8   , 64  , RAW       -0.450
    c2    39.50    z8.trigger           1  _NIK3429.NEF, 2026-05-27 15:12:33.018000, NIKON Z 8, 1/1000, 8   , 64  , RAW        0.223
    c2    39.75    z8.trigger           1  _NIK3430.NEF, 2026-05-27 15:12:33.044000, NIKON Z 8, 1/1000, 8   , 64  , RAW       -0.001
    *** UNPLANNED EXTRA SHOT ***           _NIK3431.NEF, 2026-05-27 15:12:34.071000, NIKON Z 8, 1/1000, 8   , 64  , RAW          ---
    *** UNPLANNED EXTRA SHOT ***           _NIK3432.NEF, 2026-05-27 15:12:34.093000, NIKON Z 8, 1/1000, 8   , 64  , RAW          ---
    *** UNPLANNED EXTRA SHOT ***           _NIK3433.NEF, 2026-05-27 15:12:35.019000, NIKON Z 8, 1/1000, 8   , 64  , RAW          ---
    *** UNPLANNED EXTRA SHOT ***           _NIK3434.NEF, 2026-05-27 15:12:35.046000, NIKON Z 8, 1/1000, 8   , 64  , RAW          ---
    c2    41.00    z8.trigger           1  *** MISSING SHOT DETECTED ***                                                         ---
    c2    41.25    z8.trigger           1  *** MISSING SHOT DETECTED ***                                                         ---
    c2    41.50    z8.trigger           1  *** MISSING SHOT DETECTED ***                                                         ---
    c2    41.75    z8.trigger           1  *** MISSING SHOT DETECTED ***                                                         ---
    c2    43.00    z8.trigger           1  _NIK3435.NEF, 2026-05-27 15:12:36.068000, NIKON Z 8, 8     , 8   , 64  , RAW       -0.227
    mid   1.00     z8.trigger           1  _NIK3436.NEF, 2026-05-27 15:12:47.046000, NIKON Z 8, 1/2000, 8   , 64  , RAW       -0.022


- Test booting without gps antenna works.

- Make the flask app work without a gps antenna installed:

    May 21 05:18:13 pycontrol python[5890]: WARNING: AstropyDeprecationWarning: products involving a unit and a 'str' instance are deprecated since v7.1. Convert 'N/A' to a unit explicitly. [webapp.solar_eclipse_conta>
    May 21 05:18:13 pycontrol python[5890]: Exception in thread EventSolverThread:
    May 21 05:18:13 pycontrol python[5890]: Traceback (most recent call last):
    May 21 05:18:13 pycontrol python[5890]:   File "/usr/lib/python3.11/threading.py", line 1038, in _bootstrap_inner
    May 21 05:18:14 pycontrol python[5890]:     self.run()
    May 21 05:18:14 pycontrol python[5890]:   File "/usr/lib/python3.11/threading.py", line 975, in run
    May 21 05:18:14 pycontrol python[5890]:     self._target(*self._args, **self._kwargs)
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 100, in _run
    May 21 05:18:14 pycontrol python[5890]:     self._calculate_and_store()
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 117, in _calculate_and_store
    May 21 05:18:14 pycontrol python[5890]:     solution = self._solve(params)
    May 21 05:18:14 pycontrol python[5890]:                ^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 228, in _solve
    May 21 05:18:14 pycontrol python[5890]:     solution = self._solve_solar(
    May 21 05:18:14 pycontrol python[5890]:                ^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 54, in wrapper
    May 21 05:18:14 pycontrol python[5890]:     return cached_func(*processed_args, **processed_kwargs)
    May 21 05:18:14 pycontrol python[5890]:            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 34, in cached_func
    May 21 05:18:14 pycontrol python[5890]:     return func(*args, **kwargs)
    May 21 05:18:14 pycontrol python[5890]:            ^^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/event_solver.py", line 281, in _solve_solar
    May 21 05:18:14 pycontrol python[5890]:     solution = solar_contact_times(
    May 21 05:18:14 pycontrol python[5890]:                ^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/solar_eclipse_contact_times.py", line 197, in find_contact_times
    May 21 05:18:14 pycontrol python[5890]:     location=astropy.coordinates.EarthLocation.from_geodetic(
    May 21 05:18:14 pycontrol python[5890]:              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/venv/lib/python3.11/site-packages/astropy/coordinates/earth.py", line 318, in from_geodetic
    May 21 05:18:14 pycontrol python[5890]:     lon = Angle(lon, u.degree, copy=COPY_IF_NEEDED).wrap_at(180 * u.degree)
    May 21 05:18:14 pycontrol python[5890]:           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/venv/lib/python3.11/site-packages/astropy/coordinates/angles/core.py", line 192, in __new__
    May 21 05:18:14 pycontrol python[5890]:     return super().__new__(cls, angle, unit, dtype=dtype, copy=copy, **kwargs)
    May 21 05:18:14 pycontrol python[5890]:            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    May 21 05:18:14 pycontrol python[5890]:   File "/home/nhilton/pycontrol/webapp/venv/lib/python3.11/site-packages/astropy/units/quantity.py", line 539, in __new__
    May 21 05:18:14 pycontrol python[5890]:     raise TypeError("The value must be a valid Python or Numpy numeric type.")
    May 21 05:18:14 pycontrol python[5890]: TypeError: The value must be a valid Python or Numpy numeric type.

- Running a Sim doesn't skip camera events in the past (i.e, we're not jummping forward in the sequency to
  the specified sim time.

- Validate all channels in a camera sequence with the camera?
  - What if the camera currently isn't detected?  Refuse to load?

- Can we recover from this camera_control failure?

    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: UdpSocket.cc(111): ERROR: ABORT_IF: res < 0 or (errno != 0 and errno != EAGAIN): sendto() failed on port: 10018, errno: No such device
    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: CameraControl.cc(337): ERROR: ABORT_ON_FILURE: _telem_socket.send(_telem_message.str()): UdpSocket::send() failed
    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: CameraControl.cc(1201): ERROR: _send_telemetry() failed, ignoring
    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: UdpSocket.cc(111): ERROR: ABORT_IF: res < 0 or (errno != 0 and errno != EAGAIN): sendto() failed on port: 10018, errno: No such device
    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: CameraControl.cc(337): ERROR: ABORT_ON_FILURE: _telem_socket.send(_telem_message.str()): UdpSocket::send() failed
    Feb 24 20:56:05 pycontrol camera_control_bin[13759]: CameraControl.cc(1201): ERROR: _send_telemetry() failed, ignoring
    Feb 24 20:56:06 pycontrol camera_control_bin[13759]: UdpSocket.cc(111): ERROR: ABORT_IF: res < 0 or (errno != 0 and errno != EAGAIN): sendto() failed on port: 10018, errno: No such device
    Feb 24 20:56:06 pycontrol camera_control_bin[13759]: CameraControl.cc(337): ERROR: ABORT_ON_FILURE: _telem_socket.send(_telem_message.str()): UdpSocket::send() failed
    Feb 24 20:56:06 pycontrol camera_control_bin[13759]: CameraControl.cc(1201): ERROR: _send_telemetry() failed, ignoring

- loading the eclipse event button doesn't delete the previous table rows

- speed test with z7, z8 together to see if they hit 2.33 and 4.66 FPS.

- Report command latency to UI to detect bad raspberry pi behavior

- Count and report number of errors to UI

- Failing to load a camera sequence is not fed back to UI

- Add message about when it's okay to turn off camera

- Disable camera table buttons if state == executing

- Fix these astropy warnings:
    WARNING: TimeDeltaMissingUnitWarning: Numerical value without unit or explicit format passed to TimeDelta, assuming days [astropy.time.core]
    WARNING: Tried to get polar motions for times after IERS data is valid. Defaulting to polar motion from the 50-yr mean for those. This may affect precision at the arcsec level. Please check your astropy.utils.iers.conf.iers_auto_url and point it to a newer version if necessary. [astropy.coordinates.builtin_frames.utils]
    WARNING: TimeDeltaMissingUnitWarning: Numerical value without unit or explicit format passed to TimeDelta, assuming days [astropy.time.core]

- Add camera connection counter so one can observe if it's constantly reconnecting (saw this
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

- Upload event and sequnce files using the webapp.


Make it work well
^^^^^^^^^^^^^^^^^

- Add link to Xjubier's website for the given gps location for cross checking.
    http://xjubier.free.fr/en/site_pages/solar_eclipses/TSE_2026_GoogleMapFull.html?Lat=42.97578&Lng=-5.01321&Elv=1095.0&Zoom=15&LC=1

- unit tests for everything

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

* Fixed this obvious typo:

    Camera.cc(43): INFO: Checking for some specific camera properties...
    May 21 05:37:55 pycontrol camera_control_bin[9439]: ../../external/include/gphoto2cpp/gphoto2cpp.h(675): ERROR: Failed to look up widget 'capturetagret', aborting
    May 21 05:37:55 pycontrol camera_control_bin[9439]:      capturetarget: 0

* Added emergency sync mode when the gps antenna doesn't work, so one preses the Emergency Sync and the phone's position and
  time are used to update PyControls position and sytem time.

* Created a noarmalized shutterspeed map in external/gphoto2cpp/include/gphoto2cpp.h that will take whatever
  choice list for the camera's shutterspeed property and creates the normal photographer shutters speeds, i.e.
      0.0006s --> 1/1600
      25/10   --> 2.5

* Added timelapse mode, (only for single camera), based on percent of histogram for Holy Grail timelapses.

* Connect phone to WPA2 secure, ad-hoc, wireless network provided by the pi

* Turns out not all cameras support shutterspeed2, avialablephotos.
  - I'll have to make a map for fraction shutter string shutterspeed.

* Conditional use shutterspeed2, burstnumb, num_avail, but dropped shutterspeed2 support.

* self signed certs for https comms with raspberry pi ?  Maybe adhoc wireless with WPA2 is
  all that's needed.  I've configured the rpi to hose a WIFI access point using WPA2 security.

* Add burst_number property.

* Add the trigger command to unit tests.

* Look into how many files on the memory card
  - draining camera events, adds num shots to the gui for easy confirmation of
  number of images taken

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

* Add gps time and system time delta to the gps status line, to help verify if chrony
  is correctly syncing system time.

* Auto start app on rpi boot

* Add a graphical system diagram for documentation.


Past Items Rejected
====================
