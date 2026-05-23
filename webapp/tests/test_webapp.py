import copy
import datetime
import logging
import multiprocessing
import os.path
import pytest
import random
import re
import sys
import time
import unittest

from playwright.sync_api import Page, expect

# Local webapp dir modules.
from webapp import date_utils as du
from webapp import webapp

from webapp.tests.test_ui import Button


logging.getLogger().handlers = []
logger = logging.getLogger(__file__)
logger.handlers = []
logger.addHandler(logging.StreamHandler(sys.stdout))
logger.handlers[0].setFormatter(logging.Formatter(webapp.LOG_FORMAT))
logger.setLevel(logging.INFO)


@pytest.fixture
def test_client(live_server):
    """
    Provides a base URL for the running Flask app.
    """
    return live_server.url


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

    def _setup(self, page, test_client):
        """
        Captures the playwright page object and registers the javascript
        console log callback.
        """
        if not self._page:
            self._page = page
            self._page.on("console", on_console_messages)

        self._page.goto(test_client())

        timeout = 5.0
        self._timeout = int(timeout * 1000)
        self._page = page
        self._event_button = Button(self._page, "#load_event_file_button")
        self._seq_button = Button(self._page, "#load_camera_sequence_button")
        self._sim_button = Button(self._page, "#run_sim_button")

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

    def _wait_for_event_table_ready(self, timeout=30.0):
        """
        Wait until the event table no longer contains "Computing ...".
        """
        self._page.wait_for_function(
            """() => {
              const cells = Array.from(document.querySelectorAll('#event_table tbody td'));
              if (cells.length === 0) return false;
              // Check if any cell in the second column (index 1, 4, 7...) contains "Computing ..."
              for (let i = 1; i < cells.length; i += 3) {
                  if (cells[i].innerText.includes('Computing ...')) return false;
              }
              return true;
            }""",
            timeout=timeout * 1000
        )

    def _wait_for_sim_state(self, state, timeout=10.0):
        """
        Wait until the simulation button has the expected state class.
        """
        self._page.wait_for_function(
            f"selector => document.querySelector(selector).classList.contains('{state}')",
            arg="#run_sim_button",
            timeout=timeout * 1000
        )

    def _wait_for_event_table_computing(self, timeout=2.0):
        """
        Wait until the event table contains "Computing ...".
        This is useful to ensure we are not reading stale data from before a re-calculation.
        """
        try:
            self._page.wait_for_function(
                """() => {
                  const cells = Array.from(document.querySelectorAll('#event_table tbody td'));
                  for (let i = 1; i < cells.length; i += 3) {
                      if (cells[i].innerText.includes('Computing ...')) return true;
                  }
                  return false;
                }""",
                timeout=timeout * 1000
            )
        except:
            # It's possible the calculation is so fast that we miss the "Computing ..." state in the UI.
            pass

    def test_defaults(self, page, test_client, fake_camera_io):
        """
        Test hook for exercising the default webapp behavior buttons.
        """
        self._setup(page, test_client)
        try:
            self._main(fake_camera_io)
        except Exception as e:
            self._snapshot()
            raise e

    def _main(self, fake_camera_io):
        """
        Main test suite.  Since there is a single flask app instance, we must
        execute tests in a known order as all interactions changes the flask app
        state.

        So execute the tests, in some kind of logcial order.

        TODO:
            1) assert gps states for connected and disconnected
            2) assert camera stats for connected and disconnected
        """
        self._assert_default_button_color()
        self._assert_default_event_info_table()
        self._load_events()
        self._load_sequences(fake_camera_io)
        self._load_solor_eclipse_event()
        self._run_simulation()

    def _assert_default_button_color(self):
        """
        Assert default button classes, thereby asserting their color.

        Grabbing the button colors directly does not work as the browser
        rendering can result in dramatic color value differences for green, as
        maby vectors of (red, green, blue) map to the same percieved green
        color.
        """
        assert "loaded" not in self._event_button.classes
        assert "loaded" not in self._seq_button.classes
        assert "stopped" in self._sim_button.classes

    def _assert_default_event_info_table(self):
        """
        Check the event info table.
        """
        table = self._page.locator("#event_info_table tbody td")
        all_cell_texts = table.all_inner_texts()
        expected = [
            "Please load an Event file.",
            "N/A",
            "Please load a Sequence file."
        ]
        assert all_cell_texts == expected

        # Check that the events in the event table are empty.
        table = self._page.locator("#event_table")
        table.wait_for()
        all_cell_texts = table.locator("tbody td").all_inner_texts()
        assert len(all_cell_texts) == 0

    def _load_events(self):
        """
        Click on the load event button.
        """
        on_console_messages("Load Event button click")
        self._event_button.click()

        # Wait for the popup container to become visible.
        self._page.wait_for_selector("#file_list", state="visible", timeout=self._timeout)
        file_list = self._page.locator("#file_list").inner_text().split("\n")
        assert file_list == ["custom1.event", "custom2.event", "spain-2026.event"]

        # Click on the custom1.event.
        on_console_messages("custom1.event click")
        li = self._page.locator('#file_list li:has-text("custom1.event")')
        li.click()

        # Wait for the file_list to disappear.
        self._page.wait_for_selector("#file_list", state="hidden", timeout=self._timeout)

        # Verify the event info table updates.
        table = self._page.locator("#event_info_table")
        self._page.wait_for_selector("#event_info_table tbody td", timeout=self._timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom1.event", "custom", "Please load a Sequence file."]

        # Wait for the events to show up.
        self._page.wait_for_selector("#event_row_e3", timeout=self._timeout)
        self._wait_for_event_table_ready()

        table = self._page.locator("#event_table")
        text = table.locator("tbody td").all_inner_texts()

        expected = [
            # event_id   Timestamp                    ETA
            "e1",        "2026-08-13 05:00:00.000 Z", "  317 days 02:38:59",
            "e2",        "2026-08-13 07:00:00.000 Z", "  317 days 04:38:59",
            "e3",        "2026-08-13 09:00:00.000 Z", "  317 days 06:38:59",
        ]
        assert len(text) == len(expected)

        for i in range(0, len(expected), 3):
            event_id, timestamp = text[i : i + 2]
            ex_event_id, ex_timestamp = expected[i : i + 2]

            assert event_id == ex_event_id
            assert timestamp == ex_timestamp
            # Skip ETA for now, as it's always changing.

        # Assert that the load event button turns green.
        assert "loaded" in self._event_button.classes
        assert "loaded" not in self._seq_button.classes
        assert "stopped" in self._sim_button.classes

        #---------------------------------------------------------------------
        # Now load custom2.event.
        on_console_messages("load event button click")
        self._event_button.click()

        # Wait for the popup container to become visible.
        modal = "#file_list"
        self._page.wait_for_selector(modal, state="visible", timeout=self._timeout)
        file_list = self._page.locator(modal).inner_text().split("\n")
        assert file_list == ["custom1.event", "custom2.event", "spain-2026.event"]

        # Click on the custom2.event.
        on_console_messages("custom2.event click")
        li = self._page.locator('#file_list li:has-text("custom2.event")')
        li.click()

        # Wait for the file_list to disappear.
        self._page.wait_for_selector("#file_list", state="hidden", timeout=self._timeout)

        # Verify the event info table updates.
        table = self._page.locator("#event_info_table")
        self._page.wait_for_selector("#event_info_table tbody td", timeout=self._timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "Please load a Sequence file."]

        # Wait for the events to show up.
        self._page.wait_for_selector("#event_row_e5", timeout=self._timeout)
        self._wait_for_event_table_ready()

        table = self._page.locator("#event_table")
        text = table.locator("tbody td").all_inner_texts()

        expected = [
            # event_id   Timestamp                    ETA
            "e4",        "2026-08-14 09:00:00.000 Z", "  317 days 06:38:59",
            "e5",        "2026-08-14 10:00:00.000 Z", "  317 days 07:38:59",
        ]
        assert len(text) == len(expected)

        for i in range(0, len(expected), 3):
            event_id, timestamp = text[i : i + 2]
            ex_event_id, ex_timestamp = expected[i : i + 2]

            assert event_id == ex_event_id
            assert timestamp == ex_timestamp
            # Skip ETA for now, as it's always changing.

        # Assert that the load event button turns green.
        assert "loaded" in self._event_button.classes, f"expected 'loaded' in {self._event_button.classes}"
        assert "loaded" not in self._seq_button.classes, f"expected 'loaded' not in {self._seq_button.classes}"
        assert "stopped" in self._sim_button.classes, f"expected 'stopped' in {self._sim_button.classes}"


    def _load_sequences(self, fake_camera_io):
        """
        Now load a camera sequence files.
        """
        on_console_messages("load camera sequence: s1.seq")
        self._seq_button.click()

        # Wait for the popup container to become visible.
        modal = "#camera_sequence_file_list"
        self._page.wait_for_selector(modal, state="visible", timeout=self._timeout)
        self._page.wait_for_selector("#camera_sequence_file_list li", state="visible", timeout=self._timeout)
        file_list = self._page.locator(modal + " li").all_inner_texts()
        assert file_list == ["s1.seq", "s2.seq"]

        # Click on s1.seq.
        on_console_messages("s1.seq click")
        li = self._page.locator(f'{modal} li:has-text("s1.seq")')
        li.click()

        fake_camera_io.load_sequence("webapp/tests/default_dir/sequences/s1.seq")
        fake_camera_io.set_state("execute_ready")

        seq_cell = self._page.locator("#event_info_table tbody tr:nth-child(1) td:nth-child(3)")
        expect(seq_cell).to_have_text("s1.seq", timeout=self._timeout)

        # Assert all event info table is correct.
        table = self._page.locator("#event_info_table")
        self._page.wait_for_selector("#event_info_table tbody td", timeout=self._timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "s1.seq"]

        # Assert that the load sequence button turns green.
        assert "loaded" in self._event_button.classes, f"expected 'loaded' in {self._event_button.classes}"
        assert "loaded" in self._seq_button.classes, f"expected 'loaded' in {self._seq_button.classes}"
        assert "stopped" in self._sim_button.classes, f"expected 'stopped' in {self._sim_button.classes}"

        self._page.wait_for_selector("#control_table_z7 thead th", timeout=self._timeout)
        text = self._page.locator("#control_table_z7 thead th").all_inner_texts()
        assert text == ['z7 (34)', 'ETA', 'Event ID', 'Offset (HH:MM:SS.sss)', 'Channel', 'Value']

        text = self._page.locator("#control_table_z7 tbody td").all_inner_texts()

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
        self._seq_button.click()

        # Wait for the popup container to become visible.
        modal = "#camera_sequence_file_list"
        self._page.wait_for_selector(modal, state="visible", timeout=self._timeout)
        self._page.wait_for_selector("#camera_sequence_file_list li", state="visible", timeout=self._timeout)
        file_list = self._page.locator(modal + " li").all_inner_texts()
        assert file_list == ["s1.seq", "s2.seq"]

        # Click on s2.seq.
        on_console_messages("s2.seq click")
        li = self._page.locator(f'{modal} li:has-text("s2.seq")')
        li.click()

        fake_camera_io.load_sequence("webapp/tests/default_dir/sequences/s2.seq")
        fake_camera_io.set_state("execute_ready")

        seq_cell = self._page.locator("#event_info_table tbody tr:nth-child(1) td:nth-child(3)")
        expect(seq_cell).to_have_text("s2.seq", timeout=self._timeout)

        # Assert all event info table is correct.
        table = self._page.locator("#event_info_table")
        self._page.wait_for_selector("#event_info_table tbody td", timeout=self._timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["custom2.event", "custom", "s2.seq"]

        # Assert that the load sequence button turns green.
        assert "loaded" in self._event_button.classes
        assert "loaded" in self._seq_button.classes
        assert "stopped" in self._sim_button.classes

        self._page.wait_for_selector("#control_table_z7 thead th", timeout=self._timeout)
        text = self._page.locator("#control_table_z7 thead th").all_inner_texts()
        assert text == ['z7 (64)', 'ETA', 'Event ID', 'Offset (HH:MM:SS.sss)', 'Channel', 'Value']
        text = self._page.locator("#control_table_z7 tbody td").all_inner_texts()
        expected = [
            '1',  'fake', 'c2', '-10.500', 'z7.quality',       'NEF (Raw)',
            '2',  'fake', 'c2', '-10.500', 'z7.iso',           '100',
            '3',  'fake', 'c2', '-10.500', 'z7.shutter_speed', '1/500',
            '4',  'fake', 'c2', '-10.500', 'z7.fstop',         'f/8',
            '5',  'fake', 'c2',  '-9.500', 'z7.trigger',       '1',
            '6',  'fake', 'c2',  '-9.000', 'z7.trigger',       '1',
            '7',  'fake', 'c2',  '-8.500', 'z7.trigger',       '1',
            '8',  'fake', 'c2',  '-8.000', 'z7.trigger',       '1',
            '9',  'fake', 'c2',  '-7.500', 'z7.trigger',       '1',
            '10', 'fake', 'c2',  '-7.000', 'z7.trigger',       '1',
        ]
        assert text == expected

        self._page.wait_for_selector("#control_table_z8 thead th", timeout=self._timeout)
        text = self._page.locator("#control_table_z8 thead th").all_inner_texts()
        assert text == ['z8 (64)', 'ETA', 'Event ID', 'Offset (HH:MM:SS.sss)', 'Channel', 'Value']
        text = self._page.locator("#control_table_z8 tbody td").all_inner_texts()
        expected = [
            '1',  'fake', 'c2', '-10.500', 'z8.quality',       'NEF (Raw)',
            '2',  'fake', 'c2', '-10.500', 'z8.iso',           '200',
            '3',  'fake', 'c2', '-10.500', 'z8.shutter_speed', '1/250',
            '4',  'fake', 'c2', '-10.500', 'z8.fstop',         'f/5.6',
            '5',  'fake', 'c2',  '-9.500', 'z8.trigger',       '1',
            '6',  'fake', 'c2',  '-9.000', 'z8.trigger',       '1',
            '7',  'fake', 'c2',  '-8.500', 'z8.trigger',       '1',
            '8',  'fake', 'c2',  '-8.000', 'z8.trigger',       '1',
            '9',  'fake', 'c2',  '-7.500', 'z8.trigger',       '1',
            '10', 'fake', 'c2',  '-7.000', 'z8.trigger',       '1',
        ]
        assert text == expected

    def _load_solor_eclipse_event(self):
        """
        Click on the load event button and load the solor eclipse.
        """
        on_console_messages("Load Event button click")
        self._event_button.click()

        # Wait for the popup container to become visible.
        self._page.wait_for_selector("#file_list", state="visible", timeout=self._timeout)
        self._page.wait_for_selector("#file_list li", state="visible", timeout=self._timeout)
        file_list = self._page.locator("#file_list").inner_text().split("\n")
        assert file_list == ["custom1.event", "custom2.event", "spain-2026.event"]

        # Click on the custom1.event.
        on_console_messages("spain-2026.event click")
        li = self._page.locator('#file_list li:has-text("spain-2026.event")')
        li.click()

        # Wait for the file_list to disappear.
        self._page.wait_for_selector("#file_list", state="hidden", timeout=self._timeout)

        # Verify the event info table updates.
        table = self._page.locator("#event_info_table")
        self._page.wait_for_selector("#event_info_table tbody td", timeout=self._timeout)
        text = table.locator("tbody td").all_inner_texts()
        assert text == ["spain-2026.event", "solar", "s2.seq"]

        # Wait for the events to show up.
        self._page.wait_for_selector("#event_row_c1", timeout=self._timeout)

        table = self._page.locator("#event_table")
        text = table.locator("tbody td").all_inner_texts()

        # If the solver is very fast, it might have already computed the times.
        if "Computing ..." in text:
            expected = [
                # event_id   Timestamp           ETA
                "c1",        "Computing ...",    "",
                "c2",        "Computing ...",    "",
                "mid",       "Computing ...",    "",
                "c3",        "Computing ...",    "",
                "c4",        "Computing ...",    "",
            ]
            assert text == expected

        # Assert that the load event button turns green.
        assert "loaded" in self._event_button.classes, f"expected 'loaded' in {self._event_button.classes}"
        assert "loaded" in self._seq_button.classes, f"expected 'loaded' in {self._seq_button.classes}"
        assert "stopped" in self._sim_button.classes, f"expected 'stopped' in {self._sim_button.classes}"

        # Wait for the solver to solve and replace the time stamps.

        # http://xjubier.free.fr/en/site_pages/solar_eclipses/xSE_GoogleMap3.php?Ecl=+20260812&Acc=2&Umb=1&Lmt=1&Mag=1&Max=1&Lat=42.977592&Lng=-4.997906&Elv=1258.0&Zoom=19&LC=1
        #    lat=42.977592,
        #    long=-4.997906,
        #    altitude=1258.0,  # pvlib.location.lookup_altitude(42.977592, -4.997906)
        #
        #    Start of partial eclipse (C1):   2026/08/12	17:32:09.4	+19.6	272.2	299	03.6
        #    Start of total eclipse (C2):     2026/08/12	18:27:37.8	+09.6	281.4	108	10.0
        #    Maximum eclipse (MAX):           2026/08/12	18:28:30.9	+09.4	281.5	208	06.7
        #    End of total eclipse (C3):       2026/08/12	18:29:23.7	+09.2	281.7	308	03.3
        #    End of partial eclipse (C4):     2026/08/12	19:21:21.0	+00.1	290.3	117	09.6
        #
        self._wait_for_event_table_ready()

        text = self._page.locator("#event_table tbody td").all_inner_texts()

        # print("=" * 60)
        # for i in range(0, len(text), 3):
        #     print(f"{text[i : i + 3]}")
        # print("=" * 60)

        expected = [
            "c1",  "2026-08-12 17:32:15.000 Z", "  303 days 19:17:37",
            "c2",  "2026-08-12 18:27:38.000 Z", "  303 days 20:13:00",
            "mid", "2026-08-12 18:28:30.750 Z", "  303 days 20:13:53",
            "c3",  "2026-08-12 18:29:23.500 Z", "  303 days 20:14:46",
            "c4",  "2026-08-12 19:21:15.000 Z", "  303 days 21:06:37",
        ]
        assert len(text) == len(expected)

        for i in range(0, len(expected), 3):
            event_id, timestamp = text[i : i + 2]
            ex_event_id, ex_timestamp = expected[i : i + 2]

            assert event_id == ex_event_id
            assert timestamp == ex_timestamp
            # Skip ETA for now, as it's always changing.

    def _run_simulation(self):
        """
        Click on the load event button and load the solor eclipse.
        """
        on_console_messages("Run Simulation button click")
        self._sim_button.click()

         # Wait for the popup container to become visible.
        self._page.wait_for_selector("#run_sim_modal", state="visible", timeout=self._timeout)

        all_cases = [
            # Locator   value from webapp/tests/default_dir/config/run_sim.config
            ("sim_latitude_input", "1.0"),
            ("sim_longitude_input", "2.0"),
            ("sim_altitude_input", "3.0"),
            ("sim_event_id_input", "c2"),
            ("sim_time_offset_input", "4.0"),
        ]

        for id_, expected in all_cases:
            actual = self._page.locator(f"#{id_}").input_value()
            assert actual == expected

        # Update the form with these values:
        new_values = {
            "sim_latitude_input": "42.977592",
            "sim_longitude_input": "-4.997906",
            "sim_altitude_input": "1258.0",
            "sim_event_id_input": "c2",
            "sim_time_offset_input": "-25.0",
        }

        for key, value in new_values.items():
            self._page.fill(f"#{key}", value)

        on_console_messages("Clicking on the run sim okay button")
        self._page.click("#sim_okay_button")

        self._wait_for_sim_state("running")

        # Assert the run sim button is now red.
        assert "loaded" in self._event_button.classes
        assert "loaded" in self._seq_button.classes
        assert "running" in self._sim_button.classes

        self._wait_for_event_table_computing()
        self._wait_for_event_table_ready()

        text = self._page.locator("#event_table tbody td").all_inner_texts()
        right_now = du.now()

        # print("=" * 60)
        # for i in range(0, len(text), 3):
        #     print(f"{text[i : i + 3]}")
        # print("=" * 60)

        expected = [
            # event id   timestammp                  ETA
            "c1",        "fake timestamp", "- 00:55:02",
            "c2",        "fake timestamp", "  00:00:20",
            "mid",       "fake timestamp", "  00:01:13",
            "c3",        "fake timestamp", "  00:02:06",
            "c4",        "fake timestamp", "  00:53:57",
        ]
        assert len(text) == len(expected)

        hms_patern = re.compile(r"(-| ) ([0-9]{2}):([0-9]{2}):([0-9]{2})")

        def eta_to_seconds(eta_string):
            match = hms_patern.match(eta_string)
            if not match:
                return 0.0
            sign, hours, minutes, seconds = match.groups()
            sign = -1.0 if sign == "-" else 1.0
            
            # Strip leading zeros for float conversion if needed, but float() handles "09" fine.
            hours = float(hours)
            minutes = float(minutes)
            seconds = float(seconds)
            return sign * (hours * 3600 + minutes * 60 + seconds)

        for row_num, expected_event_id in enumerate(["c1", "c2", "mid", "c3", "c4"]):
            idx = row_num * 3
            id_, timestamp, eta = text[idx:idx + 3]

            id_ == expected_event_id

            timestamp = du.make_datetime(timestamp)

            actual_eta = eta_to_seconds(eta)
            expected_eta = eta_to_seconds(expected[idx + 2])

            assert abs(expected_eta - actual_eta) <= 5.0

            # Timestamp + eta ~= right_now
            assert abs((timestamp - right_now).total_seconds() - actual_eta) <= 5.0

        # Now clicking on the sim button should stop the simulation.
        self._sim_button.click()
        self._wait_for_sim_state("stopped")

        # Assert the run sim button is now green.
        assert "loaded" in self._event_button.classes
        assert "loaded" in self._seq_button.classes
        assert "stopped" in self._sim_button.classes

        #---------------------------------------------------------------------
        # Running a simulation and erasing the event id or time, so that the
        # real-world eclipse contact times are computed.

        on_console_messages("Run Simulation button click")
        self._sim_button.click()

         # Wait for the popup container to become visible.
        self._page.wait_for_selector("#run_sim_modal", state="visible", timeout=self._timeout)

        # Update the form with these values:
        new_values = {
            "sim_latitude_input": "42.977592",
            "sim_longitude_input": "-4.997906",
            "sim_altitude_input": "1258.0",
            "sim_event_id_input": "c2",
            "sim_time_offset_input": "-25.0",
        }

        # Erasing one of these will use the simulated gps position but not
        # a simulated time offset, so the real world contact times are computed
        # for the given position.
        choice = random.choice(["sim_event_id_input", "sim_time_offset_input"])
        new_values[choice] = ""

        for key, value in new_values.items():
            self._page.fill(f"#{key}", value)

        on_console_messages("Clicking on the run sim okay button again")
        self._page.click("#sim_okay_button")
        self._wait_for_sim_state("running")

        # Assert the run sim button is now red.
        assert "loaded" in self._event_button.classes
        assert "loaded" in self._seq_button.classes
        assert "running" in self._sim_button.classes

        self._wait_for_event_table_computing()
        self._wait_for_event_table_ready()

        text = self._page.locator("#event_table tbody td").all_inner_texts()
        right_now = du.now()

        # print("=" * 60)
        # for i in range(0, len(text), 3):
        #     print(f"{text[i : i + 3]}")
        # print("=" * 60)

        # http://xjubier.free.fr/en/site_pages/solar_eclipses/xSE_GoogleMap3.php?Ecl=+20260812&Acc=2&Umb=1&Lmt=1&Mag=1&Max=1&Lat=42.977592&Lng=-4.997906&Elv=1258.0&Zoom=19&LC=1
        #    lat=42.977592,
        #    long=-4.997906,
        #    altitude=1258.0,  # pvlib.location.lookup_altitude(42.977592, -4.997906)
        #
        #    Start of partial eclipse (C1):   2026/08/12	17:32:09.4	+19.6
        #    Start of total eclipse (C2):     2026/08/12	18:27:37.8	+09.6
        #    Maximum eclipse (MAX):           2026/08/12	18:28:30.9	+09.4
        #    End of total eclipse (C3):       2026/08/12	18:29:23.7	+09.2
        #    End of partial eclipse (C4):     2026/08/12	19:21:21.0	+00.1
        #
        expected = [
            # event id   timestammp                   ETA
            "c1",        "2026-08-12 17:32:15.000 Z", "  302 days 15:00:04",
            "c2",        "2026-08-12 18:27:38.000 Z", "  302 days 15:55:27",
            "mid",       "2026-08-12 18:28:30.750 Z", "  302 days 15:56:20",
            "c3",        "2026-08-12 18:29:23.500 Z", "  302 days 15:57:13",
            "c4",        "2026-08-12 19:21:15.000 Z", "  302 days 16:49:04",
        ]
        assert len(text) == len(expected)

        for i in range(0, len(expected), 3):
            event_id, timestamp = text[i : i + 2]
            ex_event_id, ex_timestamp = expected[i : i + 2]

            assert event_id == ex_event_id
            assert timestamp == ex_timestamp

        # Now clicking on the sim button should stop the simulation.
        self._sim_button.click()
        self._wait_for_sim_state("stopped")

        # Assert the run sim button is now green.
        assert "loaded" in self._event_button.classes
        assert "loaded" in self._seq_button.classes
        assert "stopped" in self._sim_button.classes
