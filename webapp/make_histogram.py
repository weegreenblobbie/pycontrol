import os

import matplotlib
# Use a non-GUI backend, essential for server-side code
matplotlib.use('Agg')

import matplotlib.pyplot as plt
import numpy as np


def make_histogram(timelapse, output_filename, saturation_warning = 0.985):

    histogram = timelapse["histogram"]

    target_bin = timelapse["target_bin"]
    current_bin = timelapse["current_bin"]
    min_hist_mask = timelapse["min_hist_mask"]
    max_hist_mask = timelapse["max_hist_mask"]
    min_deadband = timelapse["min_deadband"]
    max_deadband = timelapse["max_deadband"]

    histogram = np.array(histogram)

    if min_hist_mask >= 0:
        histogram[0:min_hist_mask] = 0

    if max_hist_mask <= 255:
        histogram[max_hist_mask:] = 0

    bins = np.arange(len(histogram))

    hist_sum = sum(histogram)

    # Compute saturated and crushed percentages.

    saturation_index = int(saturation_warning * len(histogram) + 0.5)
    num_saturated = sum(histogram[saturation_index:])
    num_crushed = histogram[0]

    percent_saturated = 100.0 * num_saturated / hist_sum
    percent_crushed = 100.0 * num_crushed / hist_sum

    # Split the data for plotting.
    normal_values = histogram[1:saturation_index]
    saturated_values = histogram[saturation_index:]

    # Get the corresponding bins for each part.
    normal_bins = bins[1:saturation_index]
    saturated_bins = bins[saturation_index:]

    # Create the plot.
    fig = plt.figure(figsize=(15, 6))

    # Break the plot up into 4 regions, plot 3 vertical lines for the grid.

    num_bins = len(bins)

    x0 = int(0.25 * num_bins + 0.5)
    x1 = int(0.50 * num_bins + 0.5)
    x2 = int(0.75 * num_bins + 0.5)

    plt.axvline(x0, color='black', linestyle='-', linewidth=1, alpha=1, zorder=0)
    plt.axvline(x1, color='black', linestyle='-', linewidth=1, alpha=1, zorder=0)
    plt.axvline(x2, color='black', linestyle='-', linewidth=1, alpha=1, zorder=0)

    # Plot the "normal" histogram in gray.
    plt.bar(normal_bins, normal_values, width=1.0, color='gray', edgecolor='gray', zorder=3)

    # Plot the "saturated" histogram in red.
    plt.bar(saturated_bins, saturated_values, width=1.0, color='red', edgecolor='red', zorder=3)

    # Plot the "crushed" histogram in blue.
    plt.bar([0], histogram[0], width=1.0, color='blue', edgecolor='blue', zorder=3)

    # Normalize the plot to the peak of the normal values, effectivly auto
    # scaling the histogram while ignoriing curshed and saturated pixel counts.
    y_max = 1.07 * np.max(normal_values)

    # Remove tick labels.
    plt.xticks([])
    plt.yticks([])

    # Set x-axis limits to be tight.
    plt.xlim(0, len(histogram) - 1)
    plt.ylim(0, y_max)

    # Add crushed and saturated labels.

    plt.text(5, y_max * 1.05, f"{percent_crushed:.2f}% Crushed Blacks",
             ha='left', va='top', fontsize=18, color='blue', fontweight='bold')

    plt.text(len(histogram) - 6, y_max * 1.05, f"{percent_saturated:.2f}% Saturated Pixels",
             ha='right', va='top', fontsize=18, color='red', fontweight='bold')

    plt.axvline(target_bin + max_deadband, color="#FF0000", linestyle="--", linewidth=2, zorder=4)
    plt.axvline(target_bin - min_deadband, color="#FF0000", linestyle="--", linewidth=2, zorder=4)
    plt.axvline(current_bin, color="#FFA500", linestyle="-", linewidth=2, zorder=4)
    plt.axvline(target_bin, color="#00FF00", linestyle="-", linewidth=2, zorder=5)

    # Improve layout.
    plt.tight_layout()

    # Write out the plot to disk.

    # To make this file update atomically on the filesystem, we write the image
    # out to a temporary filename, then rename it.
    plt.savefig(output_filename + ".tmp.jpg")
    plt.close(fig)

    os.rename(output_filename + ".tmp.jpg", output_filename)
