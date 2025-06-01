import datetime

import dateutil

def now():
    """
    Returns the current system time in UTC.
    """
    return datetime.datetime.now(datetime.timezone.utc)


def make_datetime(string):
    """
    Parses the provided string into a UTC datetime object.
    """
    dt_obj = dateutil.parser.parse(string, ignoretz=False)
    return dt_obj.astimezone(datetime.timezone.utc)


def normalize(obj):
    """
    Normalizes a date string into a UTC date string. For example:
        2026-08-12T21:35:00.000Z
    """
    if isinstance(obj, str):
        obj = make_datetime(obj)
    return obj.isoformat(timespec='milliseconds').replace('+00:00', 'Z')


def eta(source, destination):
    """
    Returns days, hours, minutes, seconds from the origin.
    """
    seconds = (destination - source).total_seconds()

    num_days = int(seconds / 86400.0)
    seconds -= num_days * 86400.0
    num_hours = int(seconds / 3600.0)
    seconds -= num_hours * 3600.0
    num_minutes = int(seconds / 60.0)
    seconds -= num_minutes * 60.0
    seconds = int(seconds)

    out = ""
    if num_days != 0:
        out += f"{num_days} days "

    out += f"{num_hours:2d}:{num_minutes:02d}:{seconds:02d}"

    return out






