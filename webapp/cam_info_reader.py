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

now = datetime.datetime.now


# TODO: Read settings from config file.
UDP_IP = "239.192.168.1"
CAM_INFO_PORT = 10_018
CAM_RENAME_PORT = 10_019
FILENAME = "../config/camera_descriptions.config"


def send_udp_message(message, target_port, target_ip):
    """
    Sends a UDP message (text or bytes) to a specified IP address and port.

    Args:
        message (str or bytes): The message to send. If a string, it will be
                                encoded to UTF-8 bytes before sending.
        target_port (int): The UDP port of the destination.
        target_ip (str): The IP address of the destination.
    """
    # Create a UDP socket
    # AF_INET specifies the address family (IPv4)
    # SOCK_DGRAM specifies the socket type (UDP)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        # Encode the message to bytes if it's a string
        if isinstance(message, str):
            encoded_message = message.encode('utf-8')
        elif isinstance(message, bytes):
            encoded_message = message
        else:
            raise TypeError("Message must be a string or bytes.")

        # Send the message
        # sendto() is used for UDP because it includes the destination address
        sock.sendto(encoded_message, (target_ip, target_port))
        print(f"UDP message sent to {target_ip}:{target_port}: '{message}'")

    except socket.gaierror as e:
        print(f"Error resolving host {target_ip}: {e}")
    except socket.error as e:
        print(f"Socket error: {e}")
    except TypeError as e:
        print(f"Type error: {e}")
    finally:
        # Close the socket
        sock.close()


class CameraInfoReader:

    def __init__(self, filename=None):
        self._session = None
        self._lock = threading.Lock()
        self._thread = None
        self._detected = []
        self._descriptions = dict()
        self._filename = filename if filename else FILENAME
        if os.path.isfile(self._filename):
            with open(self._filename, "r") as fin:
                data = fin.readlines()
            for line in data:
                if line.strip().startswith("#"):
                    continue
                tokens = line.split()
                if len(tokens) != 2:
                    raise ValueError(f"Parse error while reading {self._filename}\n    {line.strip()}\n")
                serial, description = tokens
                self._descriptions[serial] = description

    def update_description(self, serial, desciption):

        with open(self._filename, "w") as fout:
            for serial, description in self._descriptions.items():
                fout.write(f"{serial} {description}")

        # Send camera renaming messages so CameraControl.cc will update the
        # mapping.
        #
        # message_size serial_number ' ' short_description '\n'
        msg = f"{serial} {desciption}"
        for _ in range(3):
            send_udp_message(msg, CAM_RENAME_PORT, UDP_IP)
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
            sock.bind(('', CAM_INFO_PORT))

            # Join the multicast group
            mreq = socket.inet_aton(UDP_IP) + socket.inet_aton('0.0.0.0')
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

