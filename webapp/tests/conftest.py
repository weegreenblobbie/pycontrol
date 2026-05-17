import copy
import datetime
import logging
import multiprocessing
import os.path
import pytest
import sys

from webapp import date_utils as du
from webapp import webapp

def di_log(self, msg):
    """Directly injected log function."""
    #logger.info(f"{self.__class__.__name__}:{msg}")

class FakeGpsReader:
    log = di_log
    def read(self):
        return dict(
            connected=True,
            mode="3D FIX",
            mode_time="01:00:00",
            time=du.normalize(du.now()),
            lat=42.977592,
            long=-4.997906,
            altitude=1258.0,
            delta_t=0.5,
            device="/dev/ttyACM0",
            path=None,
            sats_used=4,
            sats_seen=5,
        )
    def start(self): pass
    def stop(self): pass

class FakeCameraControlIo:
    log = di_log
    def __init__(self):
        self._id = 0
        self._seq_file = ""
        self._seq_pos = 0
        self._manager = multiprocessing.Manager()
        self._lock = self._manager.Lock()
        self._state = self._manager.dict(state="unknown")
        self._events = self._manager.dict()
        self._seq_state = self._manager.list()
        self._read_seq_state = None

    def set_state(self, new_state):
        with self._lock:
            self._state.clear()
            self._state.update(state=new_state)

    def set_events(self, events):
        with self._lock:
            self._events.clear()
            self._events.update(events)
        return {"success": True, "last_accepted_id": self._id}

    def start(self): pass

    def read(self):
        with self._lock:
            events = copy.deepcopy(dict(self._events))
            seq_file = str(self._seq_file)
            state = str(self._state["state"])
            seq_state = copy.deepcopy(list(self._seq_state))
        return dict(state=state, sequence=seq_file, sequence_state=seq_state, events=events)

    def load_sequence(self, filename=None):
        self._read_sequence(filename)
        seq_state = self._move_sequence_state_view([0] * len(self._read_seq_state))
        with self._lock:
            self._id += 1
            self._seq_file = os.path.basename(filename)
            self._seq_state[:] = seq_state
            out = dict(last_accepted_id=self._id, success=True)
        return out

    def reset_sequence(self):
        seq_state = self._move_sequence_state_view([0] * len(self._read_seq_state))
        with self._lock:
            self._seq_state[:] = seq_state
        return {"success": True}

    def reset(self):
        with self._lock:
            self._id = 0
            self._seq_file = ""
            self._state.clear()
            self._state.update(state="unknown")
            self._events.clear()
            self._seq_state[:] = []

    def _move_sequence_state_view(self, pos_offsets):
        seq_state = copy.deepcopy(list(self._read_seq_state))
        for i, ss in enumerate(seq_state):
            pos = pos_offsets[i]
            ss["events"] = ss["events"][pos:pos + 10]
        return seq_state

    def _read_sequence(self, filename):
        with open(filename, "r") as fin:
            all_lines = fin.readlines()
        all_lines = [line.strip() for line in all_lines if not line.strip().startswith("#")]
        data = dict()
        for line in all_lines:
            tokens = line.split()
            if not tokens: continue
            event_id, offset, channel = tokens[0], tokens[1], tokens[2]
            value = " ".join(tokens[3:])
            cam_id = channel.split(".")[0]
            if cam_id not in data: data[cam_id] = []
            data[cam_id].append([event_id, offset, channel, value])
        out = []
        for cam_id in sorted(data.keys()):
            all_events = data[cam_id]
            events = [dict(pos=i+1, event_id=e[0], event_time_offset=e[1], eta="fake", channel=e[2], value=e[3]) for i, e in enumerate(all_events)]
            out.append(dict(num_events=len(all_events), id=cam_id, events=events))
        self._read_seq_state = out

    def set_camera_id(self, serial, cam_id): return {"success": True}
    def read_choices(self, serial, prop): return {"success": True, "data": ["choice1", "choice2"]}
    def set_choice(self, serial, prop, value): return {"success": True}
    def trigger(self, serial): return {"success": True}
    def timelapse_enable(self): return {"success": True}
    def timelapse_disable(self): return {"success": True}
    def timelapse_update(self, **kwargs): return {"success": True}
    def timelapse_start(self, **kwargs): return {"success": True}
    def timelapse_stop(self, **kwargs): return {"success": True}

@pytest.fixture(scope='session')
def fake_gps_reader(): return FakeGpsReader()

@pytest.fixture(scope='session')
def fake_camera_io():
    fcio = FakeCameraControlIo()
    return fcio

@pytest.fixture(scope='session')
def app(pytestconfig, fake_gps_reader, fake_camera_io, setup_configs):
    pytest_dir = pytestconfig.invocation_params.dir
    root_dir = os.path.join(pytest_dir, "webapp", "tests", "default_dir")
    return webapp.create_app(root_dir, fake_gps_reader, fake_camera_io)

@pytest.fixture(scope='session', autouse=True)
def start_app_threads(app):
    app.pycontrol_app.start()
    yield

@pytest.fixture(autouse=True)
def reset_app_state(app, fake_camera_io):
    app.pycontrol_app._event_file = None
    app.pycontrol_app._seq_file = None
    app.pycontrol_app._defaults_loaded = False
    app.pycontrol_app._event_solver.reset()
    fake_camera_io.reset()
    # Clear gui.config
    gui_config = app.pycontrol_app._gui_config_filename
    with open(gui_config, "w") as fout:
        fout.write("")

@pytest.fixture(scope='session', autouse=True)
def setup_configs(pytestconfig):
    prefix = pytestconfig.invocation_params.dir
    gui_config = os.path.join(prefix, "webapp", "tests", "default_dir", "config", "gui.config")
    with open(gui_config, "w") as fout: fout.write("")
