import json
import socket
import threading
import time
from unittest.mock import MagicMock, patch, mock_open

import pytest
from webapp.camera_control_io import CameraControlIo


@pytest.fixture
def mock_config():
    return {
        "udp_ip": "127.0.0.1",
        "command_port": "1234",
        "telem_port": "5678",
        "camera_aliases": "camera_aliases.config"
    }


@pytest.fixture
def camera_control_io(mock_config):
    with patch("webapp.utils.read_kv_config", return_value=mock_config):
        with patch("os.path.isfile", return_value=True):
            cio = CameraControlIo("fake_config.config")
            return cio


def test_init(mock_config):
    with patch("webapp.utils.read_kv_config", return_value=mock_config) as mock_read:
        with patch("os.path.isfile", return_value=True) as mock_isfile:
            cio = CameraControlIo("fake_config.config")
            assert cio._udp_ip == "127.0.0.1"
            assert cio._command_port == 1234
            assert cio._telem_port == 5678
            assert cio._cam_desc_config == "camera_aliases.config"
            mock_read.assert_any_call("fake_config.config")
            mock_read.assert_any_call("camera_aliases.config")


def test_init_search_config(mock_config):
    # Test the search logic in __init__
    # We want it to fail once then succeed
    call_count = 0
    def side_effect(path):
        nonlocal call_count
        call_count += 1
        if call_count > 1:
            return True
        return False

    with patch("webapp.utils.read_kv_config", return_value=mock_config):
        with patch("os.path.isfile", side_effect=side_effect):
            cio = CameraControlIo("fake_config.config")
            assert call_count == 3


def test_send_command_success(camera_control_io):
    camera_control_io._read_thread = True  # Mock that start() was called

    # First call to read() gets current IDs (0, 0) -> command_id = 1
    # Second call to read() (in loop) gets successful response (1, 0)
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]

    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                response = camera_control_io._send_command("test_cmd")
                assert response["success"] is True
                assert response["last_accepted_id"] == 1
                mock_send.assert_called()


def test_send_command_rejected(camera_control_io):
    camera_control_io._read_thread = True

    # First call to read() gets current IDs (0, 0) -> command_id = 1
    # Second call to read() (in loop) gets rejected response (0, 1)
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 1}}
    ]

    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message"):
            with patch("time.sleep"):
                response = camera_control_io._send_command("test_cmd")
                assert response["success"] is False
                assert response["last_rejected_id"] == 1


def test_send_command_no_response(camera_control_io):
    camera_control_io._read_thread = True

    # First call to read() gets current IDs (0, 0) -> command_id = 1
    # Subsequent calls in loop return same IDs (0, 0)
    mock_telem = {
        "command_response": {
            "last_accepted_id": 0,
            "last_rejected_id": 0
        }
    }

    with patch.object(camera_control_io, "read", return_value=mock_telem):
        with patch("webapp.udp_socket.send_message"):
            with patch("time.sleep"):
                response = camera_control_io._send_command("test_cmd")
                assert response["success"] is False
                assert response["message"] == "no response from camera_control"


def test_set_camera_id(camera_control_io):
    camera_control_io._read_thread = True
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                with patch("webapp.utils.write_kv_config") as mock_write:
                    camera_control_io.set_camera_id("serial123", "cam1")
                    mock_write.assert_called_with(camera_control_io._cam_desc_config, {"serial123": "cam1"})
                    mock_send.assert_called_with("1 set_camera_id serial123 cam1", camera_control_io._udp_ip, camera_control_io._command_port)


def test_set_events(camera_control_io):
    camera_control_io._read_thread = True
    # Two calls to set_events, each needs 2 read() calls
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 2, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                # Test with empty map
                camera_control_io.set_events()
                mock_send.assert_called_with("1 set_events ", camera_control_io._udp_ip, camera_control_io._command_port)

                # Test with events
                camera_control_io.set_events({"event1": 1000, "event2": 2000})
                mock_send.assert_called_with("2 set_events event1 1000 event2 2000 ", camera_control_io._udp_ip, camera_control_io._command_port)


def test_load_sequence(camera_control_io):
    camera_control_io._read_thread = True
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.load_sequence("seq.seq")
                mock_send.assert_called_with("1 load_sequence seq.seq", camera_control_io._udp_ip, camera_control_io._command_port)


def test_reset_sequence(camera_control_io):
    camera_control_io._read_thread = True
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.reset_sequence()
                mock_send.assert_called_with("1 reset_sequence", camera_control_io._udp_ip, camera_control_io._command_port)


def test_read_choices(camera_control_io):
    camera_control_io._read_thread = True
    # 3 unique calls to _send_command (1 cached)
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 2, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.read_choices("serial123", "shutter")
                mock_send.assert_called_with("1 read_choices serial123 shutter", camera_control_io._udp_ip, camera_control_io._command_port)

                # Test cache
                camera_control_io.read_choices("serial123", "shutter")
                assert mock_send.call_count == 1

                # Test kwargs
                camera_control_io.read_choices("serial123", property_="iso")
                mock_send.assert_called_with("2 read_choices serial123 iso", camera_control_io._udp_ip, camera_control_io._command_port)


def test_set_choice(camera_control_io):
    camera_control_io._read_thread = True
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.set_choice("serial123", "shutter", "1/100")
                mock_send.assert_called_with("1 set_choice serial123 shutter 1/100", camera_control_io._udp_ip, camera_control_io._command_port)


def test_trigger(camera_control_io):
    camera_control_io._read_thread = True
    side_effects = [
        {"command_response": {"last_accepted_id": 0, "last_rejected_id": 0}},
        {"command_response": {"last_accepted_id": 1, "last_rejected_id": 0}}
    ]
    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.trigger("serial123")
                mock_send.assert_called_with("1 trigger serial123", camera_control_io._udp_ip, camera_control_io._command_port)


def test_timelapse_commands(camera_control_io):
    camera_control_io._read_thread = True
    # 5 calls to _send_command
    side_effects = []
    for i in range(5):
        side_effects.append({"command_response": {"last_accepted_id": i, "last_rejected_id": 0}})
        side_effects.append({"command_response": {"last_accepted_id": i+1, "last_rejected_id": 0}})

    with patch.object(camera_control_io, "read", side_effect=side_effects):
        with patch("webapp.udp_socket.send_message") as mock_send:
            with patch("time.sleep"):
                camera_control_io.timelapse_enable()
                mock_send.assert_called_with("1 timelapse_enable", camera_control_io._udp_ip, camera_control_io._command_port)

                kwargs = {
                    "serial": "s1",
                    "interval": 2.0,
                    "min_shutter": "1/1000",
                    "max_shutter": "1",
                    "min_iso": "100",
                    "max_iso": "3200",
                    "min_hist_mask": 0,
                    "max_hist_mask": 255,
                    "min_deadband": 5,
                    "max_deadband": 10,
                    "target_offset": 0,
                    "target_percent": 0.5
                }
                camera_control_io.timelapse_update(**kwargs)
                expected_cmd = (
                    "2 timelapse_update s1 2.0 1/1000 1 100 3200 0 255 5 10 0 0.5"
                )
                mock_send.assert_called_with(expected_cmd, camera_control_io._udp_ip, camera_control_io._command_port)

                camera_control_io.timelapse_start(serial="s1")
                mock_send.assert_called_with("3 timelapse_start s1", camera_control_io._udp_ip, camera_control_io._command_port)

                camera_control_io.timelapse_stop(serial="s1")
                mock_send.assert_called_with("4 timelapse_stop s1", camera_control_io._udp_ip, camera_control_io._command_port)

                camera_control_io.timelapse_disable()
                mock_send.assert_called_with("5 timelapse_disable", camera_control_io._udp_ip, camera_control_io._command_port)


def test_start_read(camera_control_io):
    with patch("threading.Thread") as mock_thread:
        camera_control_io.start()
        mock_thread.assert_called_once()
        assert camera_control_io._read_thread is not None

        # Test read
        with patch("copy.deepcopy", return_value=camera_control_io._telem):
            telem = camera_control_io.read()
            assert telem == camera_control_io._telem

    # Test start again raises error
    with pytest.raises(AssertionError, match="Read thread already started!"):
        camera_control_io.start()


def test_read_in_thread_success(camera_control_io):
    mock_sock = MagicMock()
    # Mock recvfrom to return valid json, then raise timeout to continue, then raise error to exit
    mock_sock.recvfrom.side_effect = [
        (b'{"key": "value"}', ("127.0.0.1", 1234)),
        socket.timeout(),
        Exception("exit loop")
    ]

    with patch("socket.socket", return_value=mock_sock):
        with patch("socket.inet_aton"):
            try:
                camera_control_io._read_in_thread()
            except Exception as e:
                assert str(e) == "exit loop"

    assert camera_control_io._telem == {"key": "value"}


def test_read_in_thread_parse_error(camera_control_io):
    mock_sock = MagicMock()
    mock_sock.recvfrom.return_value = (b'invalid json', ("127.0.0.1", 1234))

    with patch("socket.socket", return_value=mock_sock):
        with patch("socket.inet_aton"):
            with patch("builtins.open", mock_open()) as mock_file:
                with pytest.raises(Exception): # The internal raise
                    camera_control_io._read_in_thread()
                mock_file.assert_called_with('telem.json', 'w')
