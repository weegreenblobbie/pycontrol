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

            buffer_size = 16 * (3 * 128)

            while True:
                data, addr = sock.recvfrom(buffer_size)
                data = data.decode('utf-8')

                # Example message:
                #
                # CAM_INFO
                #    1
                #    7 3052284
                #   11 usb:001,004
                #   11 Nikon Z 6_2
                #

                # Parse the message.
                tokens = data.split("\n")
                idx = 0

                # Assert the message type.
                msg_type = tokens[idx].strip()
                idx += 1

                if msg_type != "CAM_INFO":
                    print(f"Unknown message type '{msg_type}'")
                    continue

                num_cameras = int(tokens[idx])
                idx += 1

                detected = []
                for i in range(num_cameras):

                    # Serial Number.
                    len_serial, serial = tokens[idx].split()
                    idx += 1

                    # Port.
                    len_port, port = int(tokens[idx][0:4]), tokens[idx][5:]
                    idx += 1

                    # Description.
                    len_desc, desc = int(tokens[idx][0:4]), tokens[idx][5:]
                    idx += 1
                    detected.append(dict(serial=serial, port=port, desc=desc))

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

