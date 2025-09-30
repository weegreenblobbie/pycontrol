from unittest.mock import patch, MagicMock
import logging
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
logger.setLevel(logging.DEBUG)


def di_log(self, msg):
    """
    Directly injected log function.
    """
    logger.info(f"{self.__class__.__name__}:{msg}")


class FakeGpsReader:

    log = di_log

    def __init__(self):
        self.mock_data = dict(
            connected=False,
            mode="NOT SEEN",
            mode_time="00:00:00",
            time=None,
            lat=0.0,
            long=0.0,
            altitude=0.0,
            time_err=None,
            device=None,
            path=None,
            sats_used=0,
            sats_seen=0,
        )

    def read(self):
        self.log("read()")
        return self.mock_data

    def start(self):
        pass

    def stop(self):
        pass

class FakeCameraControlIo:

    log = di_log

    def __init__(self):
        self._id = 0
        self._seq_file = ""
        self._events = dict()

    def test_only_set_events(self, events):
        self.log(f"test_set_events({events})")
        self._events = events

    def start(self):
        self.log("start()")

    def read(self):
        out = dict(
            state = "unknown",
            sequence = self._seq_file,
            sequence_state = [],
            events = self._events
        )
        self.log(f"read(): {out}")
        return out

    def load_sequence(self, filename=None):
        self._id += 1
        self._seq_file = filename
        self.log(f"load_sequence({filename})")
        return dict(id=self._id, success=True)


    def set_events(self, event_map):
        self.log(f"set_events({event_map})")
        self._id += 1
        return dict(id=self._id, success=True)


the_gps_reader = FakeGpsReader()
the_camera_control = FakeCameraControlIo()


@pytest.fixture(scope='session')
def app(pytestconfig):

    pytest_dir = pytestconfig.invocation_params.dir
    print(f"pytest_dir = {pytest_dir}")
    root_dir = os.path.join(pytest_dir, "tests", "default_dir")
    return webapp.create_app(root_dir, the_gps_reader, the_camera_control, add_test_apis=True)


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
        print(f"TEST FAILURE: see defaults.png", file=sys.stderr)
        print(f"TEST FAILURE: see defaults.console", file=sys.stderr)

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
        table = page.locator("#event_info_table")
        text = table.locator("tbody").all_inner_texts()
        assert text == [""]

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

        # TODO: when we use direct injection for event solver.
#~        new_events = dict(e1 = 1786693200, e2 = 1786623600, e3 = 1786630800)
#~        response = page.request.post(
#~            # The URL must be absolute or relative to the base URL
#~            f"{test_client()}/api/test_only/set_events",
#~            data={'events': new_events}
#~        )
#~        assert response.status == 200

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

