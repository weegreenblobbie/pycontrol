CameraControl.cc command IP and PORT
====================================

Multicast Address: `239.192.168.1`
Commading Port = `10017`
Telemetry Port = `10018`

To command CameraControl, a UDP packet should be sent to the following IP:PROT:
```
239.192.168.1:10017
```

All telemetry will be returned on the following IP:PORT:
```
239.192.168.1:10018
```

CameraControl Telemetry
-----------------------

Camera control will emit telemetry at 1 Hz, as a JSON string for easy parsing by
javascript or python, a pretty printed example:

```
{
    "state": "execute_ready",
    "time": 1000,
    "command_response": {
        "id": 1,
        "success": true,
    },
    "detected_cameras": [
        {
            "connected": true,
            "serial": "001",
            "port": "usb:001,004",
            "desc": "z7",
            "mode": "M",
            "shutter": "1/1000",
            "fstop": "F/10",
            "iso": "1000",
            "quality": "NEF (RAW)",
            "batt": "90",
            "num_photos": 854
        },
        {
            "connected": true,
            "serial": "002",
            "port": "usb:001,005",
            "desc": "z8",
            "mode": "M",
            "shutter": "1/1000",
            "fstop": "F/10",
            "iso": "1000",
            "quality": "NEF (RAW)",
            "batt": "70",
            "num_photos": 674
        }
    ],
    "events": {
        "c1": 1750627393194,
        "c2": 1750627397194,
        "mid": 1750627397254,
        "c3": 1750627397314,
        "c4": 1750627401314
    },
    "sequence": "spain-2026.seq",
    "sequence_state": [
        {
            "num_events": 20,
            "id": "z7",
            "events": [
                {
                    "pos": 1,
                    "event_id", "c2",
                    "event_time_offset": "-10",
                    "eta": "1 day 12:16:01",
                    "channel": "z7.trigger",
                    "value": "1"
                },
                {
                    "pos": 2,
                    "event_id", "c2",
                    "event_time_offset": "-9",
                    "eta": "1 day 12:16:00",
                    "channel": "z7.trigger",
                    "value": "1"
                }
            ]
        },
        {
            "num_events": 30,
            "id": "z8",
            "events": [
                {
                    "pos": 1,
                    "event_id", "c2",
                    "event_time_offset": "-10",
                    "eta": "1 day 12:16:01",
                    "channel": "z8.trigger",
                    "value": "1"
                },
                {
                    "pos": 2,
                    "event_id", "c2",
                    "event_time_offset": "-9",
                    "eta": "1 day 12:16:00",
                    "channel": "z8.trigger",
                    "value": "1"
                }
            ]
        }
    ]
}
```


Command: Rename camera
----------------------

We use a short hand camera id to uniquely reference an attached camera to the
system that is used in camera sequence files.  When a new camera is first detected,
the description on the camera will be composed of the camera's make + model, for
example: `Nikon Corporation Z 7`.  Using this command, the UI can allow a user
to set a simplier camera id and use the camera id in their sequence files.

```
[sequenc id: int]
set_camera_id
[serial:str] [id:str]
```

For example:
```
1 set_camera_id 3006513 z7
```

CameraControl should respond with:
```
{"id":1,"success":true}
```

This command will fail if another camera is already mapped to this id, the response
will be:
```
{"id":1,"success":false,"message":"Failure, \"z7\" is already in use"}
```

Command: Set events
-------------------

When the event solver calculates a solution, it should notify CameraControl of the
events using this command message format.

```
[sequenc id: int]
set_events
[[event id:str] [timestamp: milliseconds since UNIX EPOCH (UTC)]\n]+
```

For example:
```
2 set_events c1 1750627393194 c2 1750627397194 mid 1750627397254 c3 1750627397314 c4 1750627401314
```

CameraControl should respond with:
```
{"id":2,"success":true}
```

Command: load camera sequence file
----------------------------------

To load a camera sequence file use:

```
[sequenc id: int]
load_sequence
[filename: str]
```

For example:
```
3 load_sequnce sequences/spain-2026.seq
```

CameraControl should respond with:
```
{"id":3,"success":true}
```

If the sequence file fails to validate, an error message thats one or more lines
is returned:
```
{"id":3,"success":false,"message":"Failed to load sequences/spain-2026.seq\nsequences/spain-2026.seq(16): bad event_id 'u1'"}
```

Command: Reset camera sequence
------------------------------

When we start or stop a simulation, or load a new camera sequence, we need to reset
the position in the camera sequence such that events in the past are disgarded. This
command should be sent to CameraControl to reset the camera sequence such that the
next event hasn't happend yet.

```
[sequence id: int] reset_sequence
```

For example:
```
4 reset_sequence
```

The successful response would be:
```
{"id":4,"success":true}
```

Command: Read camera choices
----------------------------

Loads the properity choices for a given camera property, so the UI can populate a
drop down list to choose from, for example, for setting the image quality.  The
command format:

```
[sequence id: int]
read_choices
[serial: str] [property:str]
```

For example:
```
5 read_choices 3006513 imagequality
```

The successful response would be:
```
{"id":5,"success":true,"data":["JPEG Fine","JPEG Basic", "NEF (Raw)"]}
```

If an error was encountered, the response would be:
```
{"id":5,"success":false,"message":"Unknown perperty 'imagequality'"}
```


Command: Set camera choice
--------------------------

Sets the property to the specified value for camera with the specified serial number.

```
[sequence id: int]
set shoice
[serial: str] [property:str] [value:str]
```

For example:
```
6 set_choice 3006513 iso 54
```

The successful response would be:
```
{"id":6,"success":true}
```

If an error was encountered, the response would be:
```
{"id":6,"success":false,"message":"Some kind of error message."}
```
