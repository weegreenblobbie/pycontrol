import copy
import datetime
import logging
import multiprocessing
import os.path
import pytest
import sys
import time
import unittest

from playwright.sync_api import Page, expect

import webapp

from test_ui import Button


logging.getLogger().handlers = []
logger = logging.getLogger(__file__)
logger.handlers = []
logger.addHandler(logging.StreamHandler(sys.stdout))
logger.handlers[0].setFormatter(logging.Formatter(webapp.LOG_FORMAT))
logger.setLevel(logging.INFO)


def di_log(self, msg):
    """
    Directly injected log function.
    """
    logger.info(f"{self.__class__.__name__}:{msg}")


class FakeGpsReader:

    log = di_log

    def read(self):
        self.log("read()")
        return dict(
            connected=True,
            mode="3D FIX",
            mode_time="01:00:00",
            time=datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds').replace("+00:00", " UTC"),
            lat=10.0,
            long=20.0,
            altitude=30.0,
            time_err=None,
            device="/dev/ttyACM0",
            path=None,
            sats_used=4,
            sats_seen=5,
        )

    def start(self):
        pass

    def stop(self):
        pass


class FakeCameraControlIo:
    """
    See docs/camera_control_commands.md for camera_control_bin json.
    """

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
        self.log(f"set_events({events})")
        with self._lock:
            self._events.clear()
            self._events.update(events)

    def start(self):
        self.log("start()")

    def read(self):
        with self._lock:
            events = copy.deepcopy(dict(self._events))
            seq_file = str(self._seq_file)
            state = str(self._state["state"])
            seq_state = copy.deepcopy(list(self._seq_state))

        out = dict(
            state = state,
            sequence = seq_file,
            sequence_state = seq_state,
            events = events
        )
        self.log(f"read(): {out}")
        return out

    def set_seq_position(self, new_pos):
        with self._lock:
            self._pos = new_pos

    def _move_sequence_state_view(self, pos_offsets):
        seq_state = copy.deepcopy(list(self._read_seq_state))
        assert len(pos_offsets) == len(seq_state)

        # Limit to 10 as camera_control_bin does.
        for i, ss in enumerate(seq_state):
            pos_offset = pos_offsets[i]
            ss["events"] = ss["events"][i:i+10]
        return seq_state

    def load_sequence(self, filename=None):
        self.log(f"load_sequence({filename})")
        self._read_sequence(filename)
        seq_state = self._move_sequence_state_view([0] * len(self._read_seq_state))

        with self._lock:
            self._id += 1
            self._seq_file = filename
            self._seq_state[:] = seq_state
            out = dict(id=self._id, success=True)
        return out

    def set_events(self, event_map):
        self.log(f"set_events({event_map})")
        with self._lock:
            self._id += 1
            out = dict(id=self._id, success=True)
        return out

    def _read_sequence(self, filename):
        """
        Input: a camera sequnce text file:
            c2      4.000    z7.trigger    1
            c2      5.000    z7.trigger    1
            c2      6.000    z7.trigger    1

        Output: a sequence_state list of dicts:
            [
                {
                    "num_events": 3,
                    "id": "z7",
                    "events": [
                        {
                            "pos": 1,
                            "evenet_id": "c2",
                            "event_time_offset": "4.000",
                            "eta": "fake",
                            "channel": "z7.trigger",
                            "value": "1"
                        },
                        {...},
                        {...},
                    ]
                }
            ]
        """
        with open(filename, "r") as fin:
            all_lines = fin.readlines()
        all_lines = [line.strip() for line in all_lines if not line.strip().startswith("#")]
        data = dict()
        for line in all_lines:
            tokens = line.split()
            if not tokens:
                continue
            event_id = tokens.pop(0)
            offset = tokens.pop(0)
            channel = tokens.pop(0)
            value = " ".join(tokens)
            cam_id = channel.split(".")[0]
            if cam_id not in data:
                data[cam_id] = []
            data[cam_id].append([event_id, offset, channel, value])

        out = []
        for cam_id in sorted(data.keys()):
            all_events = data[cam_id]
            num_events = len(all_events)
            events = []
            pos = 1
            for event_id, offset, channel, value in all_events:
                events.append(dict(
                   pos = pos,
                   event_id = event_id,
                   event_time_offset = offset,
                   eta = "fake",
                   channel = channel,
                   value = value,
                ))
                pos += 1

            out.append(dict(
                num_events = num_events,
                id = cam_id,
                events = events,
            ))

        self._read_seq_state = out



fake_gps_reader = FakeGpsReader()
fake_camera_io = FakeCameraControlIo()

fake_camera_io.load_sequence("tests/default_dir/sequences/s1.seq")


@pytest.fixture(scope='session')
def app(pytestconfig):

    pytest_dir = pytestconfig.invocation_params.dir
    print(f"pytest_dir = {pytest_dir}")
    root_dir = os.path.join(pytest_dir, "tests", "default_dir")
    return webapp.create_app(root_dir, fake_gps_reader, fake_camera_io)


@pytest.fixture(scope='session', autouse=True)
def start_app_threads(app):
    app.pycontrol_app.start()
    yield


@pytest.fixture
def test_client(live_server):
    """
    Provides a base URL for the running Flask app.
    """
    return live_server.url


@pytest.fixture(scope='session', autouse=True)
def setup_configs(pytestconfig):
    """
    The flask app will "remember" the last event and sequence file it loaded
    and saves them to ROOT_DIR/config/gui.config.

    This unit test assumes gui.config is empty, so this function ensure
    gui.config is empty before the flask app is launched.
    """
    prefix = pytestconfig.invocation_params.dir
    gui_config = os.path.join(prefix, "tests", "default_dir", "config", "gui.config")
    with open(gui_config, "w") as fout:
        fout.write("")


_console_messages = []

def on_console_messages(msg):
    """Callback function to process console messages."""
    global _console_messages
    if isinstance(msg, str):
        _console_messages.append(f"testcase:  {msg}\n")
    else:
        _console_messages.append(f"webapp.js: {msg.type.upper()}: {msg.text}\n")


class TestWebapp:

    _page = None

    def _setup(self, page):
        """
        Captures the playwright page object and registers the javascript
        console log callback.
        """
        if not self._page:
            self._page = page
            self._page.on("console", on_console_messages)

    def _snapshot(self):
        """
        On a test case failure, takes a full page screenshot and dumps all the
        recorded console messages to a log file.
        """
        # Take screenshot.
        self._page.screenshot(path = "defaults.png", full_page = True)

        # Write out console messages.
        global _console_messages
        with open("defaults.console", "w") as fout:
            for msg in _console_messages:
                fout.write(msg)
        with open("defaults.html", "w") as fout:
            fout.write(self._page.content())
        print(f"TEST FAILURE: see defaults.png", file=sys.stderr)
        print(f"TEST FAILURE: see defaults.console", file=sys.stderr)
        print(f"TEST FAILURE: see defaults.html", file=sys.stderr)

    def test_defaults(self, page, test_client):
        """
        Test hook for exercising the default webapp behavior buttons.
        """
        self._setup(page)

        try:
            page.goto(test_client())
            self._run(page, test_client)
        except Exception as e:
            self._snapshot()
            raise e

    def _run(self, page, test_client):
        """
        1) Tests the button color by assert DOM classes attached to the buttons.
        Clicks on the load event button and ensures the events get populated in
        the event table.

        2) Loads the another event file, ensures the old events in the table
        are erased and replaced by the new events.

        3) Loads a camera sequence file.
        """
        event_button = Button(page, "#load_event_file_button")
        seq_button = Button(page, "#load_camera_sequence_button")
        sim_button = Button(page, "#run_sim_button")

        assert "loaded" not in event_button.classes
        assert "loaded" not in seq_button.classes
        assert "stopped" in sim_button.classes

        #----------------------------------------------------------------------
        # Check the event info table.
        table = page.locator("#event_info_table tbody td")
        all_cell_texts = table.all_inner_texts()
        expected = [
            "Please load an Event file.",
            "N/A",
            "Please load a Sequence file."
        ]
        assert all_cell_texts == expected

        # Check that the events in the event table are empty.
        table = page.locator("#event_table")
        table.wait_for()

        all_cell_texts = table.locator("tbody td").all_inner_texts()

        assert len(all_cell_texts) == 0

        #----------------------------------------------------------------------
        # Click on the load event button.
        on_console_messages("Load Event button click")
        event_button.click()

        timeout = 5.0
        timeout = int(timeout * 1000)

        # Wait for the popup container to become visible.
        modal = "#file_list"
        page.wait_for_selector(modal, state="visible", timeout=timeout)
        file_list = page.locator(modal).inner_text().split("\n")
        assert file_list == ["custom1.event", "custom2.event"]

        # Click on the custom1.event.
        on_console_messages("custom1.event click")
        li = page.locator('#file_list li:has-text("custom1.event")')
        li.click()

        # Wait for the file_list to disappear.
        page.wait_for_selector("#file_list", state="hidden", timeout=timeout)

        # Verify the event info table updates.
        table = page.locator("#event_info_table")
        page.wait_for_selector("#event_info_table tbody td", timeout=timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom1.event", "custom", "N/A"]

        # Wait for the events to show up.
        page.wait_for_selector("#event_row_e3", timeout=timeout)

        table = page.locator("#event_table")
        all_cell_texts = table.locator("tbody td").all_inner_texts()

        expected = [
            # event_id   Timestamp                   ETA
            "e1",        "2026-08-13T05:00:00.000Z", "  317 days 02:38:59",
            "e2",        "2026-08-13T07:00:00.000Z", "  317 days 04:38:59",
            "e3",        "2026-08-13T09:00:00.000Z", "  317 days 06:38:59",
        ]

        assert len(all_cell_texts) == len(expected)

        for i in range(len(expected)):
            if (i + 1) % 3 == 0:
                # Skip ETA for now, as it's always chaning.
                assert all_cell_texts[i] != ""
            else:
                assert all_cell_texts[i] == expected[i]

        # Assert that the load event button turns green.
        assert "loaded" in event_button.classes
        assert "loaded" not in seq_button.classes
        assert "stopped" in sim_button.classes

        #---------------------------------------------------------------------
        # Now load custom2.event.
        on_console_messages("load event button click")
        event_button.click()

        # Wait for the popup container to become visible.
        modal = "#file_list"
        page.wait_for_selector(modal, state="visible", timeout=timeout)
        file_list = page.locator(modal).inner_text().split("\n")
        assert file_list == ["custom1.event", "custom2.event"]

        # Click on the custom2.event.
        on_console_messages("custom2.event click")
        li = page.locator('#file_list li:has-text("custom2.event")')
        li.click()

        # Wait for the file_list to disappear.
        page.wait_for_selector("#file_list", state="hidden", timeout=timeout)

        # Verify the event info table updates.
        table = page.locator("#event_info_table")
        page.wait_for_selector("#event_info_table tbody td", timeout=timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "N/A"]

        # Wait for the events to show up.
        page.wait_for_selector("#event_row_e5", timeout=timeout)

        table = page.locator("#event_table")
        all_cell_texts = table.locator("tbody td").all_inner_texts()

        expected = [
            # event_id   Timestamp                   ETA
            "e4",        "2026-08-14T09:00:00.000Z", "  317 days 06:38:59",
            "e5",        "2026-08-14T10:00:00.000Z", "  317 days 07:38:59",
        ]

        assert len(all_cell_texts) == len(expected)

        for i in range(len(expected)):
            if (i + 1) % 3 == 0:
                # Skip ETA for now, as it's always chaning.
                assert all_cell_texts[i] != ""
            else:
                assert all_cell_texts[i] == expected[i]

        # Assert that the load event button turns green.
        assert "loaded" in event_button.classes
        assert "loaded" not in seq_button.classes
        assert "stopped" in sim_button.classes

        #---------------------------------------------------------------------
        # Now load a camera sequence file: s1.seq

        on_console_messages("load camera sequence: s1.seq")
        seq_button.click()

        # Wait for the popup container to become visible.
        modal = "#camera_sequence_file_list"
        page.wait_for_selector(modal, state="visible", timeout=timeout)
        file_list = page.locator(modal + " li").all_inner_texts()
        assert file_list == ["s1.seq", "s2.seq"]

        # Click on s1.seq.
        on_console_messages("s1.seq click")
        li = page.locator(f'{modal} li:has-text("s1.seq")')
        li.click()

        fake_camera_io.load_sequence("tests/default_dir/sequences/s1.seq")
        fake_camera_io.set_state("execute_ready")

        seq_cell = page.locator("#event_info_table tbody tr:nth-child(1) td:nth-child(3)")
        expect(seq_cell).to_have_text("s1.seq", timeout=timeout)

        # Assert all event info table is correct.
        table = page.locator("#event_info_table")
        page.wait_for_selector("#event_info_table tbody td", timeout=timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "s1.seq"]

        # Assert that the load sequence button turns green.
        assert "loaded" in event_button.classes
        assert "loaded" in seq_button.classes
        assert "stopped" in sim_button.classes

        page.wait_for_selector("#control_table thead th", timeout=timeout)
        text = page.locator("#control_table thead th").all_inner_texts()
        assert text == ['z7 (# of 34)', 'ETA', 'Event ID', 'Offset (HH:MM:SS.sss)', 'Channel', 'Value']

        text = page.locator("#control_table tbody td").all_inner_texts()

        expected = [
            '1', 'fake', 'c2', '-10.000', 'z7.quality',       'NEF (Raw)',
            '2', 'fake', 'c2', '-10.000', 'z7.iso',           '100',
            '3', 'fake', 'c2', '-10.000', 'z7.shutter_speed', '1/500',
            '4', 'fake', 'c2', '-10.000', 'z7.fstop',         'f/8',
            '5', 'fake', 'c2',  '-9.000', 'z7.trigger',       '1',
            '6', 'fake', 'c2',  '-8.000', 'z7.trigger',       '1',
            '7', 'fake', 'c2',  '-7.000', 'z7.trigger',       '1',
            '8', 'fake', 'c2',  '-6.000', 'z7.trigger',       '1',
            '9', 'fake', 'c2',  '-5.000', 'z7.trigger',       '1',
            '10', 'fake', 'c2', '-4.000', 'z7.trigger',       '1'
        ]
        assert text == expected

        #---------------------------------------------------------------------
        # Now load a camera sequence file: s2.seq

        on_console_messages("load camera sequence: s2.seq")
        seq_button.click()

        # Wait for the popup container to become visible.
        modal = "#camera_sequence_file_list"
        page.wait_for_selector(modal, state="visible", timeout=timeout)
        file_list = page.locator(modal + " li").all_inner_texts()
        assert file_list == ["s1.seq", "s2.seq"]

        # Click on s2.seq.
        on_console_messages("s2.seq click")
        li = page.locator(f'{modal} li:has-text("s2.seq")')
        li.click()

        seq_cell = page.locator("#event_info_table tbody tr:nth-child(1) td:nth-child(3)")
        expect(seq_cell).to_have_text("s2.seq", timeout=timeout)

        # Assert all event info table is correct.
        table = page.locator("#event_info_table")
        page.wait_for_selector("#event_info_table tbody td", timeout=timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "s2.seq"]

        # Assert that the load sequence button turns green.
        assert "loaded" in event_button.classes
        assert "loaded" in seq_button.classes
        assert "stopped" in sim_button.classes

