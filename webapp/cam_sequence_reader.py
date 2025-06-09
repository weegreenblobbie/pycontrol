#! /usr/bin/env python3
"""
A client that reads camera event sequence state UDP packets.
"""
import copy
import datetime
import os.path
import socket
import threading
import time

import date_utils as du
import utils


MAX_TIME = (1 << 63) - 1


class CameraSequenceReader:

    def __init__(self, camera_control_config):

        config = utils.read_kv_config(camera_control_config)
        self._udp_ip = config["udp_ip"]
        self._seq_port = int(config["seq_state_port"])
        self._lock = threading.Lock()
        self._thread = None
        self._data = dict()

    def start(self):
        self._thread = threading.Thread(target=self._run_in_thread)
        self._thread.daemon = True
        self._thread.start()

    def read(self):
        assert self._thread, "Please call start() first!"
        with self._lock:
            data = self._data
        return data

    def _run_in_thread(self):

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:

            # Set the socket to reuse the address (optional, but useful for
            # debugging)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            # Bind to the specified port on all interfaces.
            sock.bind(('', self._seq_port))

            # Join the multicast group
            mreq = socket.inet_aton(self._udp_ip) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            sock.settimeout(3.0)

            buffer_size = 1024

            while True:
                try:
                    data, addr = sock.recvfrom(buffer_size)
                except socket.timeout:
                    continue
                data = data.decode('utf-8')

                # Example message:
                #
                #     state executing
                #     num_cameras 2
                #     connected 1
                #     name z7
                #     num_events 20
                #     position 1
                #     event_id C1
                #     eta 6000
                #     channel trigger
                #     value 1
                #     connected 1
                #     name z8
                #     num_events 20
                #     position 1
                #     event_id C1
                #     eta 6000
                #     channel trigger
                #     value 1
                #

                # Parse the message.
                lines = data.split("\n")
                idx = 0

                # State
                state = lines[idx].split()[-1]
                idx += 1

                # num _cameras.
                num_cameras = int(lines[idx].split()[-1])
                idx += 1

                cameras = []
                for _ in range(num_cameras):
                    connected = bool(lines[idx].split()[-1])
                    idx += 1
                    name = lines[idx].split()[-1]
                    idx += 1
                    num_events = int(lines[idx].split()[-1])
                    idx += 1
                    position = int(lines[idx].split()[-1])
                    idx += 1
                    event_id = lines[idx].split()[-1]
                    idx += 1
                    if event_id == "none":
                        idx += 4
                        continue

                    event_time_offset_s = lines[idx].split()[-1]
                    idx += 1

                    eta = int(lines[idx].split()[-1])
                    idx += 1

                    channel = lines[idx].split()[-1]
                    idx += 1

                    value = lines[idx].split()[-1]
                    idx += 1

                    # If the event isn't defined (caused by the event not being
                    # visible), the eta will be MAX_TIME, so use N/A.
                    if eta == MAX_TIME:
                        eta = "N/A"
                    else:
                        eta = du.eta(seconds = eta / 1000.0)

                    cameras.append(dict(
                        connected = connected,
                        name = name,
                        num_events = num_events,
                        position = position,
                        event_id = event_id,
                        event_time_offset_s = event_time_offset_s,
                        eta = eta,
                        channel = channel,
                        value = value,
                    ))

                with self._lock:
                    self._data = dict(
                        state = state,
                        num_cameras = len(cameras),
                        cameras = cameras,
                    )

        print("thread exited")

