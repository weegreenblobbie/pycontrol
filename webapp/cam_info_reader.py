#! /usr/bin/env python3
"""
A client that reads camera_monitor_bin UDP packets.
"""
import copy
import datetime
import os.path
import socket
import threading
import time

import utils

now = datetime.datetime.now


class CameraInfoReader:

    def __init__(self, camera_control_config):

        config = utils.read_kv_config(camera_control_config)
        self._udp_ip = config["udp_ip"]
        self._cam_info_port = int(config["cam_info_port"])
        self._cam_rename_port = int(config["cam_rename_port"])
        self._cam_desc_config = os.path.join("..", config["camera_aliases"])
        assert os.path.isfile(self._cam_desc_config)

        self._lock = threading.Lock()
        self._thread = None
        self._detected = []
        self._descriptions = dict()

        # Read in camera aliases.
        self._descriptions = utils.read_kv_config(self._cam_desc_config)

    def update_description(self, serial, desciption):
        """
        Updates the serial -> description mapping and notifies camera control.
        """
        self._descriptions[serial] = desciption
        utils.write_kv_config(self._cam_desc_config, self._descriptions)

        msg = f"{serial} {desciption}"
        for _ in range(3):
            udp_socket.send_message(msg, self._udp_ip, self._cam_rename_port)
            time.sleep(0.5)

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
            sock.bind(('', self._cam_info_port))

            # Join the multicast group
            mreq = socket.inet_aton(self._udp_ip) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            sock.settimeout(3.0)

            buffer_size = 16 * (3 * 128)

            while True:
                try:
                    data, addr = sock.recvfrom(buffer_size)
                except socket.timeout:
                    continue
                data = data.decode('utf-8')

                # Example message:
                #
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
                        value = " ".join(tokens[1:]).upper()
                        if value != "__ERROR__":
                            cam[key] = value

                    detected.append(cam)

                detected = sorted(detected, key = lambda x: x['serial'])

                with self._lock:
                    self._detected = detected

        print("thread exited")

