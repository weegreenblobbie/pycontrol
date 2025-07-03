#! /usr/bin/env python3
"""
A client that reads camera_monitor_bin UDP packets.
"""
import copy
import ctypes
import datetime
import functools
import json
import os.path
import socket
import threading
import time
import udp_socket
import utils

now = datetime.datetime.now


def args_cache(maxsize):
    """
    A decorator that caches a function's results, rounding float arguments
    to a specified number of decimal places for cache key generation.

    Args:
        num_decimal_places (int): The number of decimal places to round floats to.
        maxsize (int): The maximum size of the cache.
                       Set to None for an unbounded cache.
    """
    def decorator(func):
        # Create a proxy function that rounds floats before calling the original
        # This proxy is what lru_cache will actually wrap
        @functools.lru_cache(maxsize=maxsize)
        def cached_func(*args, **kwargs):
            # The actual logic of the original function goes here
            return func(*args, **kwargs)

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # Pre-process arguments for hashing: round floats
            processed_args = []
            for arg in args:
                processed_args.append(arg)

            processed_kwargs = {}
            for k, v in kwargs.items():
                processed_kwargs[k] = v

            # Call the inner cached function with the processed arguments
            return cached_func(*processed_args, **processed_kwargs)

        # Expose cache control methods (optional but good practice)
        wrapper.cache_info = cached_func.cache_info
        wrapper.cache_clear = cached_func.cache_clear
        wrapper.cache_parameters = cached_func.cache_parameters
        return wrapper
    return decorator


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

        self._retry_count = 15
        self._retry_sleep = 0.250

    def _send_command(self, message):
        """
        Sends the command to CameraControl, automatically increments the command
        id by one and then repeatively sends the UDP command messages and waits
        for the response.

        UDP commanding and telemetry is not guarenteed to be delivered, which is
        why we repeatly send the command and check for the response.
        """
        with self._write_lock:
            telem = self.read()
            command_id = ctypes.c_uint32(telem["command_response"]["id"] + 1)
            cmd = f"{command_id.value} {message}"
            response = None
            for _ in range(self._retry_count):
                udp_socket.send_message(cmd, self._udp_ip, self._command_port)
                time.sleep(self._retry_sleep)
                telem = self.read()
                response_id = telem["command_response"]["id"]
                if response_id >= command_id.value:
                    response = telem["command_response"]
                    break
        return response

    def set_camera_id(self, serial, cam_id):
        """
        Commands CameraControl with a new serial -> camera id mapping.
        """
        self._serial_id_cam_id[serial] = cam_id
        utils.write_kv_config(self._cam_desc_config, self._serial_id_cam_id)

        cmd = f"set_camera_id {serial} {cam_id}"
        return self._send_command(cmd)

    def set_events(self, event_map):
        """
        Commands CameraControl with a new event_id to timestamp map.
        """
        cmd = "set_events "
        for event_id, timestamp in event_map.items():
            cmd += f"{event_id} {timestamp} "
        return self._send_command(cmd)

    def load_sequence(self, sequence_fn):
        """
        Commands CameraControl to load a camera sequenc file.
        """
        cmd = f"load_sequence {sequence_fn}"
        return self._send_command(cmd)

    def reset_sequence(self):
        """
        Commands CameraControl to reset the camera sequence.
        """
        return self._send_command("reset_sequence")

    @args_cache(maxsize=128)
    def read_choices(self, serial, property_):
        """
        Commands CameraControl to return a list of choices for the specified
        camera serial number and property.
        """
        cmd = f"read_choices {serial} {property_}"
        return self._send_command(cmd)

    def set_choice(self, serial, property_, value):
        """
        Commands CameraControl to set a camera property.
        """
        cmd = f"set_choice {serial} {property_} {value}"
        return self._send_command(cmd)

    def trigger(self, serial):
        """
        Triggers the camera with `serial`.
        """
        cmd = f"trigger {serial}"
        return self._send_command(cmd)

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
