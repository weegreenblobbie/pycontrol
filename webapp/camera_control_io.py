#! /usr/bin/env python3
"""
A client that reads camera_monitor_bin UDP packets.
"""
import copy
import ctypes
import datetime
import json
import os.path
import socket
import threading
import time
import udp_socket
import utils

now = datetime.datetime.now


class CameraControlIo:
    """
    See docs/camera_control_commands.md for details on the command and telemetry
    format.
    """

    def __init__(self, camera_control_config):

        config = utils.read_kv_config(camera_control_config)
        self._udp_ip = config["udp_ip"]
        self._command_port = int(config["command_port"])
        self._telem_port = int(config["telem_port"])

        self._cam_desc_config = os.path.join("..", config["camera_aliases"])
        assert os.path.isfile(self._cam_desc_config)
        self._serial_id_cam_id = utils.read_kv_config(self._cam_desc_config)

        self._write_lock = threading.Lock()
        self._read_lock = threading.Lock()
        self._read_thread = None
        self._read_sock = None
        self._serial_id_cam_id = dict()
        self._telem = dict(command_response=dict(id=0))


    def set_camera_id(self, serial, cam_id):
        """
        Commands CameraControl with a new serial -> camera id mapping.
        """
        self._serial_id_cam_id[serial] = cam_id
        utils.write_kv_config(self._cam_desc_config, self._serial_id_cam_id)

        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = (
                f"{command_id.value} "
                "set_camera_id "
                f"{serial} {cam_id}"
            )
            response = None
            for _ in range(10):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(0.300)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                if response_id == command_id.value:
                    response = telem["command_response"]
                    break

        return response

    def set_events(self, event_map):
        """
        Commands CameraControl with a new event_id to timestamp map.
        """
        events = ""
        for event_id, timestamp in event_map.items():
            events += f"{event_id} {timestamp} "

        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = (
                f"{command_id.value} "
                "set_events "
                f"{events}"
            )
            response = None
            for _ in range(10):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(0.500)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                print(f'    {telem["command_response"]}')
                if response_id == command_id.value:
                    response = telem["command_response"]
                    break

        return response

    def load_sequence(self, sequence_fn):
        """
        Commands CameraControl to load a camera sequenc file.
        """
        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = (
                f"{command_id.value} "
                "load_sequence "
                f"{sequence_fn}"
            )
            print(f"load_sequence: {cmd}")
            response = None
            for _ in range(10):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(0.300)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                if response_id == command_id.value:
                    response = telem["command_response"]
                    print(f"load sequence response: {response}")
                    print(f"    sequence: {telem['sequence']}")
                    break
        return response

    def reset_sequence(self):
        """
        Commands CameraControl to reset the camera sequence.
        """
        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = (
                f"{command_id.value} "
                "reset_sequence"
            )
            response = None
            for _ in range(10):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(0.300)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                if response_id == command_id.value:
                    response = telem["command_response"]
                    break
        return response

    def read_choices(self, serial, property_):
        """
        Commands CameraControl to return a list of choices for the specified
        camera serial number and property.
        """
        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = (
                f"{command_id} "
                "read_choices "
                f"{serial} {property_}"
            )
            response = None
            for _ in range(10):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(0.300)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                if response_id == command_id.value:
                    response = telem["command_response"]
                    break
        return response

    def start(self):
        assert self._read_thread is None, "Read thread already started!"
        self._read_thread = threading.Thread(target=self._read_in_thread)
        self._read_thread.daemon = True
        self._read_thread.start()

    def read(self):
        assert self._read_thread, "Please call start() first!"
        with self._read_lock:
            telem = copy.deepcopy(self._telem)
        return telem

    def _read_in_thread(self):

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Set the socket to reuse the address.
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Bind to the specified port on all interfaces.
        sock.bind(('', self._telem_port))

        # Join the multicast group and set a read timeout.
        mreq = socket.inet_aton(self._udp_ip) + socket.inet_aton('0.0.0.0')
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        sock.settimeout(2.0)

        buffer_size = 4096

        while True:
            try:
                data, addr = sock.recvfrom(buffer_size)
            except socket.timeout:
                continue
            try:
                telem = json.loads(data.decode('utf-8'))
            except:
                print("failed to parse telem:\n" + repr(data.decode('utf-8')) + "\n")
                with open('telem.json', 'w') as fout:
                    fout.write(data.decode('utf-8') + "\n")
                raise

            with self._read_lock:
                self._telem = telem
