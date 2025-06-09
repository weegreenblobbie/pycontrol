# Treat warnings as errors.
#import warnings
#warnings.filterwarnings("error")

import numpy as np
#~import scipy.optimize
import astropy.units as u
import astropy.time
import astropy.constants
import astropy.coordinates

import dateutil as du

astropy.coordinates.solar_system_ephemeris.set("de430")

RADIUS_SUN = astropy.constants.R_sun
RADIUS_MOON = 1736.0 * u.km


def distance_calc(
    location: astropy.coordinates.EarthLocation,
    time, #: np.array[astropy.time.Time],
    mode: str
):

    assert mode in {'overlap', 'totality'}

    coordinate_sun = astropy.coordinates.get_body("sun", time)
    coordinate_moon = astropy.coordinates.get_body("moon", time)

    frame_local = astropy.coordinates.AltAz(obstime=time, location=location)

    alt_az_sun = coordinate_sun.transform_to(frame_local)
    alt_az_moon = coordinate_moon.transform_to(frame_local)

    angular_size_sun = np.arctan2(RADIUS_SUN, alt_az_sun.distance).to(u.deg)
    angular_size_moon = np.arctan2(RADIUS_MOON, alt_az_moon.distance).to(u.deg)

    separation = alt_az_moon.separation(alt_az_sun).deg * u.deg

    if mode == "overlap":
        # C1, C4, any_overlap >= 0.0
        return angular_size_sun + angular_size_moon - separation

    elif mode == "totality":
        return (
            angular_size_moon - angular_size_sun - separation,
            angular_size_sun,
            angular_size_moon,
        )

    raise ValueError(f"Unkown mode '{mode}'")


def calculate_contact_times(
    location: astropy.coordinates.EarthLocation,
    time_search_start: astropy.time.Time,
):

    c1 = None
    c2 = None
    c3 = None
    c4 = None
    mid = None

    #--------------------------------------------------------------------------
    # Find any overlapping times.
    #--------------------------------------------------------------------------

    # Please note, this uses a very course 30m time step, so it's possible that
    # locations near the edge of any overlap could be missed.

    # Course search +/- 12 hours
    c1_c2_course_times = astropy.time.Time(
        np.arange(
            time_search_start - 12 * u.hour,
            time_search_start + 12 * u.hour,
            step = 15 * u.min
        )
    )

    # Find overlapping times.
    any_overlap = distance_calc(
        location=location,
        time=c1_c2_course_times,
        mode="overlap",
    )

    # Abort early if no overlap is detected.
    have_overlap = any_overlap >= 0.0
    if not np.any(have_overlap):
        return dict(C1=c1, C2=c2, MID=mid, C3=c3, C4=c4, type="no-overlap")

    # Fine tune C1.
    c1_index = np.argmax(have_overlap)
    c1_time = c1_c2_course_times[c1_index]
    # Fine grain search.
    c1_times = astropy.time.Time(
        np.arange(
            c1_time - 31 * u.min,
            c1_time + 31 * u.min,
            step = 5 * u.s,
        )
    )
    c1_contacts = distance_calc(location, c1_times, "overlap")
    c1_index = np.argmax(c1_contacts >= 0.0)
    c1 = c1_times[c1_index]

    # Fine tune C4.
    c4_index = len(any_overlap) - np.argmax(have_overlap[::-1]) - 1
    c4_time = c1_c2_course_times[c4_index]
    # Fin grain serch
    c4_times = astropy.time.Time(
        np.arange(
            c4_time - 31 * u.min,
            c4_time + 31 * u.min,
            step = 5 * u.s
        )[::-1] # reversed
    )
    c4_contacts = distance_calc(location, c4_times, "overlap")
    c4_index = np.argmax(c4_contacts >= 0.0)
    c4 = c4_times[c4_index]

    #--------------------------------------------------------------------------
    # Find any totality times.
    #--------------------------------------------------------------------------

    # Please note, this uses a very course 30s time step, so it's possbile that
    # locations neary the edge of totality could be missed.

    c2_c3_course_times = astropy.time.Time(np.arange(c1, c4, step = 5 * u.min))
    any_totality, _, _ = distance_calc(
        location=location,
        time=c2_c3_course_times,
        mode="totality",
    )

    # Fine tune.
    center_time = c2_c3_course_times[np.argmax(any_totality)]
    totality_times = astropy.time.Time(
        np.arange(
            center_time - 5 * u.min,
            center_time + 5 * u.min,
            step = 0.25 * u.s,
        )
    )

    any_totality, angular_size_sun, angular_size_moon = distance_calc(
        location=location,
        time=totality_times,
        mode="totality",
    )

    mid_index = np.argmax(any_totality)
    mid = totality_times[mid_index]

    as_moon = angular_size_moon[mid_index]
    as_sun = angular_size_sun[mid_index]

    # Detect the type of elipse at max.
    type_ = None
    if as_moon >= as_sun:
        type_ = "totality"
    elif np.any(any_totality >= 0):
        type_ = "annular"
    else:
        type_ = "partial"

    if type_ != "partial":
        c2_index = np.argmax(any_totality >= 0)
        c2 = totality_times[c2_index]

        c3_index = np.argmax(any_totality[::-1] >= 0)
        c3 = totality_times[-c3_index]

    return dict(C1=c1, C2=c2, MID=mid, C3=c3, C4=c4, type=type_)


def find_contact_times(start, lat, lon, alt):

    if isinstance(start, str):
        start = astropy.time.Time(start, format="iso")

    assert isinstance(start, astropy.time.Time)

    return calculate_contact_times(
        location=astropy.coordinates.EarthLocation(lat=lat * u.deg, lon=lon * u.deg, height=alt * u.m),
        time_search_start=start,
    )

if __name__ == "__main__":

    import datetime

    d0 = datetime.datetime.now()

    test_cases = [

        # Iceland + Spain 2026.
        # http://xjubier.free.fr/en/site_pages/solar_eclipses/xSE_GoogleMap3.php?Ecl=+20260812&Acc=2&Umb=1&Lmt=1&Mag=1&Max=1&Lat=64.08045&Lng=-22.69157&Elv=5.0&Zoom=19&LC=1

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

        # 2024 American eclipse.
        # https://eclipse.gsfc.nasa.gov/SEpath/SEpath2001/SE2024Apr08Tpath.html
        # http://xjubier.free.fr/en/site_pages/solar_eclipses/TSE_2024_GoogleMapFull.html

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

        contact_times = find_contact_times(
            start=start,
            lat=lat,
            lon=lon,
            alt=alt)

        print("Calculated contact times:")
        print(f"Latitude:  {lat:.6f}")
        print(f"Longitude: {lon:.6f}")
        print(f"Altitude:  {alt:.2f}")

        for label in ["C1", "C2", "MID", "C2", "C4"]:
            if label not in tc:
                continue
            computed_time = contact_times[label]
            expected_time = astropy.time.Time(tc[label], format="iso")

            delta = expected_time - computed_time

            print(f"{label:3s}: {computed_time}, error: {delta.sec:.1f}")


    d1 = datetime.datetime.now()
    delta = (d1 - d0).total_seconds()
    avg = delta / len(test_cases)
    print(f"Average call time: {avg:.1f}")
