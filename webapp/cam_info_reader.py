#! /usr/bin/env python3
"""
A client that reads camera_monitor_bin UDP packets.
"""
import copy
import datetime
import socket
import threading
import time

now = datetime.datetime.now

# TODO: Read settings from config file.
UDP_IP = "239.192.168.1"
UDP_PORT = 10_018


class CameraInfoReader:

    def __init__(self):
        self._session = None
        self._lock = threading.Lock()
        self._thread = None
        self._detected = []

    def start(self):
        self._thread = threading.Thread(target=self._run_in_thread)
        self._thread.daemon = True
        self._thread.start()

    def read(self):
        assert self._thread, "Please call start() first!"
        with self._lock:
            data = dict(
                num_cameras=len(self._detected),
                detected=self._detected,
            )
        return data

    def _run_in_thread(self):

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:

            # Set the socket to reuse the address (optional, but useful for
            # debugging)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            # Bind to the specified port on all interfaces.
            sock.bind(('', UDP_PORT))

            # Join the multicast group
            mreq = socket.inet_aton(UDP_IP) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            sock.settimeout(3.0)

            buffer_size = 16 * (3 * 128)

            while True:
                try:
                    data, addr = sock.recvfrom(buffer_size)
                except socket.timeout:
                    with self._lock:
                        for cam in self._detected:
                            cam["connected"] = 0
                    continue
                data = data.decode('utf-8')

                # Example message:
                #
                #     CAM_CONTROL
                #     state monitor
                #     num_cameras 2
                #     connected 1
                #     serial 3006513
                #     port usb:001,005
                #     desc Nikon Corporation Z 7
                #     mode M
                #     shutter 1/400
                #     fstop f/8
                #     iso 500
                #     quality NEF (Raw)
                #     batt 60%
                #     num_photos 845
                #     connected 1
                #     serial 3052284
                #     port usb:001,006
                #     desc Nikon Corporation Z 6_2
                #     mode M
                #     shutter 1/400
                #     fstop f/8
                #     iso 500
                #     quality NEF+Fine
                #     batt 100%
                #     num_photos 1231
                #

                # Parse the message.
                lines = data.split("\n")
                idx = 0

                # Assert the message type.
                msg_type = lines[idx].strip()
                idx += 1

                if msg_type != "CAM_CONTROL":
                    print(f"Unknown message type '{msg_type}'")
                    continue

                # State.
                state = lines[idx].split()[-1]
                idx += 1

                # num _cameras.
                num_cameras = int(lines[idx].split()[-1])
                idx += 1

                detected = []
                for i in range(num_cameras):

                    # 11 properties
                    cam = dict()

                    for j in range(11):
                        tokens = lines[idx].split()
                        idx += 1
                        key = tokens[0]
                        value = " ".join(tokens[1:])
                        if value != "__ERROR__":
                            cam[key] = value

                    detected.append(cam)

                detected = sorted(detected, key = lambda x: x['serial'])

                with self._lock:
                    self._detected = detected

        print("thread exited")


if __name__ == "__main__":
    cir = CameraInfoReader()
    cir.start()

    while True:
        print(cir.read())
        time.sleep(3)

