import unittest
import astropy.time
import astropy.units as u
from webapp.solar_eclipse_contact_times import find_contact_times

class TestSolarEclipse(unittest.TestCase):
    def test_contact_times(self):
        # Data ported from webapp/solar_eclipse_contact_times.py run_test()
        test_cases = [
            # Iceland.
            dict(lat=64.92186, lon=-23.28779, alt=5.0,
                 C1="2026-08-12 16:45:16.6",
                 C2="2026-08-12 17:46:05.3",
                 MID="2026-08-12 17:47:01.4",
                 C3="2026-08-12 17:47:57.3",
                 C4="2026-08-12 18:46:08.3"),

            dict(lat=64.08045, lon=-22.69157, alt=5.0,
                 C1="2026-08-12 16:47:01.8",
                 C2="2026-08-12 17:47:53.5",
                 MID="2026-08-12 17:48:45.1",
                 C3="2026-08-12 17:49:36.5",
                 C4="2026-08-12 18:47:45.1"),

            # Spain.
            dict(lat=43.53666, lon=-6.49477, alt=97.0,
                 C1="2026-08-12 17:30:57.8",
                 C2="2026-08-12 18:26:53.9",
                 MID="2026-08-12 18:27:48.9",
                 C3="2026-08-12 18:28:43.7",
                 C4="2026-08-12 19:21:04.9"),

            dict(lat=43.25793, lon=-2.94985, alt=39.0,
                 C1="2026-08-12 17:31:49.9",
                 C2="2026-08-12 18:27:24.0",
                 MID="2026-08-12 18:27:39.8",
                 C3="2026-08-12 18:27:55.4",
                 C4="2026-08-12 19:20:05.4"),

            dict(lat=41.89892, lon=-5.68087, alt=716.0,
                 C1="2026-08-12 17:34:00.8",
                 C2="2026-08-12 18:29:45.9",
                 MID="2026-08-12 18:30:24.3",
                 C3="2026-08-12 18:31:02.4",
                 C4="2026-08-12 19:23:12.8"),

            dict(lat=40.38566, lon=0.13181, alt=321.0,
                 C1="2026-08-12 17:36:52.0",
                 C2="2026-08-12 18:30:37.6",
                 MID="2026-08-12 18:31:27.5",
                 C3="2026-08-12 18:32:17.3",
                 C4="2026-08-12 19:22:45.5"),

            dict(lat=40.918959, lon=-1.289364, alt=321.0,
                 C1="2026-08-12 17:35:58.6",
                 C2="2026-08-12 18:30:10.6",
                 MID="2026-08-12 18:31:01.5",
                 C3="2026-08-12 18:31:52.2",
                 C4="2026-08-12 19:22:42.7"),

            # 2024 American eclipse.
            dict(lat=23.198333, lon=-106.113, alt=107.0,
                C1="2024-04-08 16:51:44.6",
                C2="2024-04-08 18:07:49.3",
                MID="2024-04-08 18:10:02.7",
                C3="2024-04-08 18:12:16.4",
                C4="2024-04-08 19:32:39.3",
            ),
            dict(lat=28.326666, lon=-101.105, alt=491.0,
                C1="2024-04-08 17:08:38.6",
                C2="2024-04-08 18:25:49.3",
                MID="2024-04-08 18:28:02.8",
                C3="2024-04-08 18:30:16.5",
                C4="2024-04-08 19:49:49.7",
            ),
            dict(lat=34.53333, lon=-93.845, alt=326.0,
                C1="2024-04-08 17:30:49.0",
                C2="2024-04-08 18:47:53.8",
                MID="2024-04-08 18:50:02.8",
                C3="2024-04-08 18:52:11.7",
                C4="2024-04-08 20:09:04.1",
            ),
            dict(lat=40.09333, lon=-84.93833, alt=352.0,
                C1="2024-04-08 17:52:49.8",
                C2="2024-04-08 19:08:03.1",
                MID="2024-04-08 19:10:02.9",
                C3="2024-04-08 19:12:02.4",
                C4="2024-04-08 20:24:50.6",
            ),
            dict(lat=45.37333, lon=-71.32833, alt=462.0,
                C1="2024-04-08 18:17:21.9",
                C2="2024-04-08 19:28:18.2",
                MID="2024-04-08 19:30:02.9",
                C3="2024-04-08 19:31:47.2",
                C4="2024-04-08 20:38:43.1",
            ),
            dict(lat=48.41833, lon=-55.065, alt=237.0,
                C1="2024-04-08 18:37:04.5",
                C2="2024-04-08 19:42:33.7",
                MID="2024-04-08 19:44:02.5",
                C3="2024-04-08 19:45:30.9",
                C4="2024-04-08 20:46:38.7",
            ),
        ]

        for tc in test_cases:
            lat = tc["lat"]
            lon = tc["lon"]
            alt = tc["alt"]
            start = tc["MID"][:11] + "12:00:00.000"

            contact_times = find_contact_times(start=start, lat=lat, lon=lon, alt=alt)

            for label in ["C1", "C2", "MID", "C3", "C4"]:
                if label not in tc:
                    continue
                computed_time = contact_times[label]
                expected_time = astropy.time.Time(tc[label], format="iso")

                delta = abs((expected_time - computed_time).sec)
                
                # Apply different tolerances based on the contact type
                if label in ["C2", "MID", "C3"]:
                    tolerance = 1.1
                else:
                    tolerance = 10.0
                
                self.assertLess(delta, tolerance, f"Error for {label} at lat={lat}, lon={lon} is {delta}s")

    def test_invalid_input(self):
        # Test Task 2 requirements: validation for lat, lon, alt
        # The crash was a TypeError when passing "N/A"
        # Now it should return a dict with type="invalid-input" and None values
        
        result = find_contact_times("2026-08-12 12:00:00.000", "N/A", -23.28779, 5.0)
        self.assertEqual(result["type"], "invalid-input")
        self.assertIsNone(result["C1"])
        self.assertIsNone(result["C2"])
        self.assertIsNone(result["MID"])
        self.assertIsNone(result["C3"])
        self.assertIsNone(result["C4"])

        # Test None
        result = find_contact_times("2026-08-12 12:00:00.000", 64.92186, None, 5.0)
        self.assertEqual(result["type"], "invalid-input")

        # Test missing altitude (N/A as string)
        result = find_contact_times("2026-08-12 12:00:00.000", 64.92186, -23.28779, "N/A")
        self.assertEqual(result["type"], "invalid-input")

if __name__ == "__main__":
    unittest.main()
