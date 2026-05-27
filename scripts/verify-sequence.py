import csv
import csv
import argparse
import collections
from datetime import datetime
from tabulate import tabulate

def load_exif_data(exif_filepath):
    """Parses the new CSV exposure dump."""
    actual_shots = []
    with open(exif_filepath, 'r') as f:
        lines = f.readlines()

        # Skip the header row
        if lines and "Filename" in lines[0]:
            lines = lines[1:]

        for line in lines:
            clean_line = line.strip('\n')
            if not clean_line.strip():
                continue

            parts = clean_line.split(',')
            if len(parts) < 6:
                continue

            # Extract based on new column positions
            filename = parts[0].strip()
            timestamp_str = parts[1].strip()
            shutter = parts[3].strip()
            aperture = parts[4].strip()
            iso = parts[5].strip()

            try:
                # Updated to parse YYYY-MM-DD instead of YYYY:MM:DD
                dt = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S.%f")

                actual_shots.append({
                    'raw_line': clean_line,
                    'filename': filename,
                    'time': dt,
                    'iso': iso,
                    'aperture': aperture,
                    'shutter': shutter
                })
            except ValueError as e:
                print(f"Skipping row due to timestamp error: {clean_line} -> {e}")

    return actual_shots


def load_sequence_data(sequence_filepath):
    """Parses the eclipse trigger sequence and tracks camera state."""
    expected_shots = []
    current_state = {'iso': None, 'aperture': None, 'shutter': None}

    with open(sequence_filepath, 'r') as f:
        for line in f:
            clean_line = line.strip()
            if not clean_line or clean_line.startswith('#'):
                continue

            parts = clean_line.split()
            if len(parts) >= 4:
                phase = parts[0]
                rel_time = float(parts[1])
                command = parts[2]
                value = " ".join(parts[3:])

                if command == 'z8.iso':
                    current_state['iso'] = value
                elif command == 'z8.fstop':
                    current_state['aperture'] = value.replace('f/', '')
                elif command == 'z8.shutter_speed':
                    current_state['shutter'] = value
                elif command == 'z8.trigger':
                    expected_shots.append({
                        'raw_line': clean_line,
                        'phase': phase,
                        'time_sec': rel_time,
                        'iso': current_state['iso'],
                        'aperture': current_state['aperture'],
                        'shutter': current_state['shutter']
                    })

    return expected_shots


def analyze_correspondence(actual_shots, expected_triggers):
    """Aligns the sequence globally using Dynamic Programming to minimize time deltas."""

    start_idx = 0
    for i, shot in enumerate(actual_shots):
        if shot['iso'] == expected_triggers[0]['iso'] and shot['shutter'] == expected_triggers[0]['shutter']:
            start_idx = i
            break

    sequence_actuals = actual_shots[start_idx:]

    if not sequence_actuals or not expected_triggers:
        print("Error: Missing data for alignment.")
        return

    print(f"--- Sequence Alignment ---")
    print(f"Total Exif Files:        {len(actual_shots)}")
    print(f"Ignored Pre-shots:       {start_idx}")
    print(f"Expected Sequence Shots: {len(expected_triggers)}")
    print(f"Actual Sequence Shots:   {len(sequence_actuals)}\n")

    if len(sequence_actuals) != len(expected_triggers):
        print("WARNING: Shot count mismatch! Dynamic alignment will isolate dropped frames.\n")

    # Use the first actual shot as our "zero" epoch to keep math clean
    epoch = sequence_actuals[0]['time']

    # 1. Anchor each phase using offset clustering
    for e in expected_triggers:
        e['abs_time'] = e['time_sec'] # Safe fallback

    for phase in set([e['phase'] for e in expected_triggers]):
        phase_exp = [e for e in expected_triggers if e['phase'] == phase]

        valid_offsets = []
        for e in phase_exp:
            for a in sequence_actuals:
                if e['shutter'] == a['shutter']:
                    # Calculate real-world offset from sequence time
                    offset = (a['time'] - epoch).total_seconds() - e['time_sec']
                    valid_offsets.append(offset)

        if not valid_offsets:
            continue

        # Find the mathematical center of the true offset cluster (rounded to 0.1s)
        buckets = [round(o * 10) / 10 for o in valid_offsets]
        most_common = collections.Counter(buckets).most_common(1)[0][0]
        cluster = [o for o in valid_offsets if abs(o - most_common) <= 0.15]
        cluster.sort()
        if cluster:
            true_offset = cluster[len(cluster)//2]

            # Apply the true real-world anchor to all expected shots in this phase
            for e in phase_exp:
                e['abs_time'] = e['time_sec'] + true_offset

    # 2. Dynamic Programming Alignment
    N = len(expected_triggers)
    M = len(sequence_actuals)

    dp = [[float('inf')] * (M + 1) for _ in range(N + 1)]
    dp[0][0] = 0
    parent = [[None] * (M + 1) for _ in range(N + 1)]

    for i in range(N + 1):
        for j in range(M + 1):
            if i < N and j < M:
                e = expected_triggers[i]
                a = sequence_actuals[j]

                a_abs_time = (a['time'] - epoch).total_seconds()
                time_err = abs(e['abs_time'] - a_abs_time)

                # Heavy penalty if camera settings don't match
                exp_penalty = 0 if e['shutter'] == a['shutter'] else 100.0

                cost = dp[i][j] + time_err + exp_penalty
                if cost < dp[i+1][j+1]:
                    dp[i+1][j+1] = cost
                    parent[i+1][j+1] = (i, j)

            # Increased penalty so the algorithm prefers a delayed shot
            # over marking it as missing/extra.
            skip_penalty = 10.0

            if i < N: # Skip expected (Camera missed the shot)
                skip_cost = dp[i][j] + skip_penalty
                if skip_cost < dp[i+1][j]:
                    dp[i+1][j] = skip_cost
                    parent[i+1][j] = (i, j)

            if j < M: # Skip actual (Unplanned extra shot)
                skip_cost = dp[i][j] + skip_penalty
                if skip_cost < dp[i][j+1]:
                    dp[i][j+1] = skip_cost
                    parent[i][j+1] = (i, j)

    # 3. Backtrack the DP matrix to find the optimal path
    i, j = N, M
    alignment = []
    while i > 0 or j > 0:
        p = parent[i][j]
        if p is None: break
        pi, pj = p
        if pi == i - 1 and pj == j - 1:
            alignment.append((pi, pj))
        elif pi == i - 1 and pj == j:
            alignment.append((pi, None))
        elif pi == i and pj == j - 1:
            alignment.append((None, pj))
        i, j = pi, pj

    alignment.reverse()

    # 4. Output the final correspondence using tabulate
    table_data = []
    for exp_i, act_j in alignment:
        if exp_i is not None and act_j is not None:
            e = expected_triggers[exp_i]
            a = sequence_actuals[act_j]

            a_abs_time = (a['time'] - epoch).total_seconds()
            delta = a_abs_time - e['abs_time']

            table_data.append([e['raw_line'], a['raw_line'], f"{delta:6.3f}"])
        elif exp_i is not None:
            e = expected_triggers[exp_i]
            table_data.append([e['raw_line'], "*** MISSING SHOT DETECTED ***", "---"])
        elif act_j is not None:
            a = sequence_actuals[act_j]
            table_data.append(["*** UNPLANNED EXTRA SHOT ***", a['raw_line'], "---"])

    output = tabulate(
        table_data,
        headers=["SEQUENCE TRIGGER LINE", "EXIF LOG LINE", "DELTA (s)"],
        colalign=("left", "left", "right"),
    )

    print(output)


def main():
    parser = argparse.ArgumentParser(description="Verify camera trigger sequence against EXIF timestamps.")
    parser.add_argument("sequence_log", help="Path to the expected sequence log file")
    parser.add_argument("exif_log", help="Path to the tab-separated EXIF log file")

    args = parser.parse_args()

    actual = load_exif_data(args.exif_log)
    expected = load_sequence_data(args.sequence_log)

    if not actual:
        print(f"Error: Could not load data from {args.exif_log}")
    elif not expected:
        print(f"Error: Could not load data from {args.sequence_log}")
    else:
        analyze_correspondence(actual, expected)

if __name__ == "__main__":
    main()