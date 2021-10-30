#!/usr/bin/env python3

import sys
import argparse
import signal
import time
import serial, serial.tools.list_ports
import csv
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from dataclasses import dataclass
from typing import Optional


@dataclass
class Stats:
    num_measurements: int
    min_delay: float
    max_delay: float
    mean_delay: float
    median_delay: float
    std_dev: float


def sigint_handler(signal, frame):
    print("Caught KeyboardInterrupt, exiting.")
    sys.exit(0)


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="This script obtains delay measurements from a Glass-to-Glass device connected via USB, "
        "saves results and statistics to a CSV file, and displays the results in a histogram plot. "
        "You can also use it to display results from saved measurements in a previous test, in which case "
        "the CSV file is read from rather than written to."
    )
    parser.add_argument(
        "filename",
        nargs="?",
        default="results.csv",
        type=Path,
        help="The name of the CSV file where the data is saved. It is also the name for the saved plot. "
        "Default is 'results.csv' which will cause 'results.png' to be created as well. "
        "This is the filename used when using the '--readcsv' option as well.",
    )
    parser.add_argument(
        "num_measurements",
        nargs="?",
        default=100,
        type=int,
        help="An integer for the number of measurements to save and use when generating statistics. Default is 100.",
    )
    parser.add_argument(
        "--quiet",
        "-q",
        action="store_true",
        help="The script won't print the measurements to the terminal (but will still save them into the CSV file).",
    )
    parser.add_argument(
        "--readcsv",
        "-r",
        action="store_true",
        help="Reads a previously generated CSV and plots it. "
        "Be sure to provide the name of the CSV file if it's not the default name.",
    )

    args = parser.parse_args()
    if args.filename.suffix != ".csv":
        print("Error: Provided filename is invalid or does not have .csv extension")
        sys.exit(-1)
    return args


def find_arduino_on_serial_port() -> serial.Serial:
    devices = serial.tools.list_ports.comports()
    for device in devices:
        if device.manufacturer is not None:
            if "Arduino" in device.manufacturer:
                print(f"Found Arduino at {device[0]}")
                return serial.Serial(device[0], 115200, timeout=5)

    raise ConnectionRefusedError("Did not find Arduino on any serial port. Is it connected?")


def read_measurements_from_arduino(num_measurements, quiet_mode):
    serial = find_arduino_on_serial_port()

    print(f"Collecting {num_measurements} measurements from the Arduino")
    if quiet_mode:
        print("Running in quiet mode, won't print the measurements to the terminal")

    # Read messages from Arduino
    timeout = time.time() + 0.01
    while True:
        a = serial.readline().decode()
        if time.time() > timeout:
            break

    measurements = []
    i = 0
    overall_rounds = 0
    init_message = 0

    while i < num_measurements:
        overall_rounds += 1
        a = serial.readline().decode()
        if "." in a:
            init_message = 1
            i += 1
            a = a.replace("\n", "")
            a = a.replace("\r", "")
            measurements.append(float(a))
            if not quiet_mode:
                print(f"G2G Delay trial {i}/{num_measurements}: {a} ms")
            else:
                print(f"G2G Delay trial {i}/{num_measurements}", end="\r")
        else:
            if overall_rounds > 0 and init_message == 1:
                print(
                    "Did not receive msmt data from the Arduino for another 5 seconds. "
                    "Is the phototransistor still sensing the LED?"
                )
            else:
                print(
                    """Did not receive msmt data from the Arduino for 5 seconds.
Is the phototransistor sensing the LED on the screen?
Is the correct side of the PT pointing towards the screen (the flat side with the knob on it)?
Is the screen brightness high enough (max recommended)?"""
                )
                init_message = 1

    return measurements


def write_measurements_to_csv(filename, measurements, stats):
    with open(filename, "w") as f:
        writer = csv.writer(f)
        writer.writerow(["Samples", "Min", "Max", "Mean", "Median", "stdDev"])
        writer.writerow(
            [
                stats.num_measurements,
                stats.min_delay,
                stats.max_delay,
                stats.mean_delay,
                stats.median_delay,
                stats.std_dev,
            ]
        )
        writer.writerow(measurements)

    print(f"Saved results to {filename}")


def read_measurements_from_csv(filename):
    with open(filename, "r") as f:
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            if i == 0:  # header row, do nothing
                pass
            elif i == 1:  # stats values
                stats = Stats(*(float(i) for i in row))
            elif i == 2:  # measurement samples
                measurements = [float(i) for i in row]
                break  # ignore further rows
    print(f"Obtained values from {filename}")

    return measurements, stats


def generate_stats(measurements):
    measurements_np = np.array(measurements)

    min_delay = np.min(measurements_np)
    max_delay = np.max(measurements_np)
    mean_delay = np.mean(measurements_np)
    median_delay = np.median(measurements_np)
    std_dev = np.std(measurements_np)

    stats = Stats(len(measurements_np), min_delay, max_delay, mean_delay, median_delay, std_dev)

    print(f"\nmin: {min_delay:.2f} ms | max: {max_delay:.2f} ms | median: {median_delay:.2f} ms")
    print(f"mean: {mean_delay:.2f} ms | std_dev: {std_dev:.2f} ms\n")

    return stats


def plot_results(measurements, stats, fig_name):
    plt.hist(measurements, bins=20)
    plt.gcf().canvas.manager.set_window_title(fig_name)
    plt.title("Latency Histogram")
    plt.xlabel("Latency (ms)")
    plt.ylabel("Frequency")

    ax = plt.gca()
    props = dict(boxstyle="round", facecolor="wheat", alpha=0.5)
    textstr1 = "\n".join(
        (
            r"$\mathrm{min}=%.2f$" % (stats.min_delay,),
            r"$\mathrm{max}=%.2f$" % (stats.max_delay,),
            r"$\mathrm{median}=%.2f$" % (stats.median_delay,),
        )
    )
    # place it at position x=0.05, y=0.95, relative to the top and left of the box
    ax.text(
        0.05,
        0.95,
        textstr1,
        transform=ax.transAxes,
        fontsize=14,
        verticalalignment="top",
        horizontalalignment="left",
        bbox=props,
    )
    textstr2 = "\n".join((r"$\mu=%.2f$" % (stats.mean_delay,), r"$\sigma=%.2f$" % (stats.std_dev,)))
    # place it at position x=0.95, y=0.90, relative to the top and right of the box
    ax.text(
        0.95,
        0.95,
        textstr2,
        transform=ax.transAxes,
        fontsize=14,
        verticalalignment="top",
        horizontalalignment="right",
        bbox=props,
    )

    plt.savefig(fig_name)
    print(f"Saved histogram to {fig_name}")
    plt.show()


def main():
    # Set up signal handler to handle keyboard interrupts
    signal.signal(signal.SIGINT, sigint_handler)

    args = parse_arguments()

    if args.readcsv:
        g2g_delays, stats = read_measurements_from_csv(args.filename)
    else:
        g2g_delays = read_measurements_from_arduino(args.num_measurements, args.quiet)
        stats = generate_stats(g2g_delays)
        write_measurements_to_csv(args.filename, g2g_delays, stats)

    plot_results(g2g_delays, stats, args.filename.with_suffix(".png"))


if __name__ == "__main__":
    main()
