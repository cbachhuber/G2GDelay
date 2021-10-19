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
    parser = argparse.ArgumentParser(description="This script obtains delay measurements from a Glass-to-Glass device connected via USB, saves results and statistics to a CSV file, and displays the results in a histogram plot. You can also use it to display results from saved measurements in a previous test, in which case the CSV file is read from rather than written to.")

    parser.add_argument('filename', nargs='?', default="results.csv", type=Path, help="The name of the CSV file where the data is saved. It is also the name for the saved plot. Default is 'results.csv' which will cause 'results.png' to be created as well. This is the filename used when using the '--readcsv' option as well.")

    parser.add_argument('num_measurements', nargs='?', default=100, type=int, help="An integer for the number of measurements to save and use when generating statistics. Default is 100.")

    parser.add_argument('--quiet', '-q', action='store_true', help="The script won't print the measurements to the terminal (but will still save them into the CSV file).")

    parser.add_argument('--readcsv', '-r', action='store_true', help="Reads a previously generated CSV and plots it. Be sure to provide the name of the CSV file if it's not the default name.")

    return parser.parse_args()

def find_arduino_on_serial_port():
    devs = serial.tools.list_ports.comports()
    found = 0
    for dev in devs:
        if dev.manufacturer is not None:
            if "Arduino" in dev.manufacturer:
                print(f"Found Arduino at {dev[0]}")
                ser = serial.Serial(dev[0], 115200,timeout=5)
                found = 1
    if found == 0:
        print("Did not find Arduino on any serial port. Is it connected?")
        sys.exit()

    return ser

def get_measurements_serial(num_measurements, quiet_mode):
    ser = find_arduino_on_serial_port()

    print(f"Collecting {num_measurements} measurements from the Arduino")
    if quiet_mode:
        print("Running in quiet mode, won't print the measurements to the terminal")

    # Read messages from Arduino
    timeout = time.time() + 0.01
    while True:
        a = ser.readline().decode()
        if time.time() > timeout:
            break

    measurements = []
    i = 0
    overall_rounds = 0
    init_message = 0

    while (i < num_measurements):
        overall_rounds += 1
        a = ser.readline().decode()
        if "." in a:
            init_message = 1
            i += 1
            a = a.replace("\n","")
            a = a.replace("\r","")
            measurements.append(float(a))
            if not quiet_mode:
                print(f"G2G Delay trial {i}/{num_measurements}: {a} ms")
            else:
                print(f"G2G Delay trial {i}/{num_measurements}", end="\r")
        else:
            if overall_rounds > 0 and init_message == 1:
                print("Did not receive msmt data from the Arduino for another 5 seconds. Is the phototransistor still sensing the LED?")
            else:
                print("Did not receive msmt data from the Arduino for 5 seconds. \n Is the phototransistor sensing the LED on the screen? \n Is the correct side of the PT pointing towards the screen (the flat side with the knob on it)? \n Is the screen brightness high enough (max recommended)?")
                init_message = 1

    return measurements

def write_measurements_csv(filename, measurements, stats):
    with open(filename, 'w') as f:
        writer = csv.writer(f)

        header = ['Samples', 'Min', 'Max', 'Mean', 'Median', 'stdDev']
        writer.writerow(header)

        stats_list = [stats.num_measurements, stats.min_delay, stats.max_delay, stats.mean_delay, stats.median_delay, stats.std_dev]
        writer.writerow(stats_list)

        writer.writerow(measurements)

    print(f"Saved results to {filename}")

def get_measurements_csv(filename):
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        # 2nd row has the stats, 3rd row has the datapoints
        for i, row in enumerate(reader):
            if i == 1:
                # convert every string in the array into a float
                stats_list = [float(i) for i in row]
                # grab each element of the list to create the Stats object
                stats = Stats(*stats_list)
            if i == 2:
                measurements = [float(i) for i in row]
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

    return measurements_np, stats

def plot_results(measurements, stats, fig_name):
    plt.hist(measurements, bins=20)
    plt.gcf().canvas.manager.set_window_title(fig_name)
    plt.title('Latency Histogram')
    plt.xlabel('Latency (ms)')
    plt.ylabel('Frequency')

    ax = plt.gca()
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    textstr1 = '\n'.join((
        r'$\mathrm{min}=%.2f$' % (stats.min_delay,),
        r'$\mathrm{max}=%.2f$' % (stats.max_delay,),
        r'$\mathrm{median}=%.2f$' % (stats.median_delay,)))
    # place it at position x=0.05, y=0.95, relative to the top and left of the box
    ax.text(0.05, 0.95, textstr1, transform=ax.transAxes, fontsize=14,
        verticalalignment='top', horizontalalignment='left', bbox=props)
    textstr2 = '\n'.join((
        r'$\mu=%.2f$' % (stats.mean_delay, ),
        r'$\sigma=%.2f$' % (stats.std_dev,)))
    # place it at position x=0.95, y=0.90, relative to the top and right of the box
    ax.text(0.95, 0.95, textstr2, transform=ax.transAxes, fontsize=14,
        verticalalignment='top', horizontalalignment='right', bbox=props)

    plt.savefig(fig_name)
    print(f"Saved histogram to {fig_name}")
    plt.show()

if __name__ == "__main__":
    # Set up signal handler to handle keyboard interrupts
    signal.signal(signal.SIGINT, sigint_handler)

    args = parse_arguments() # parse command line input arguments
    filename = args.filename.name # obtain the filename as a string, not a PosixPath
    file_extension = args.filename.suffix
    if file_extension != '.csv':
        print("Error: Provided filename is invalid or does not have .csv extension")
        sys.exit()

    # Either write the data to a CSV file or read from a preexisting one
    fig_name = args.filename.with_suffix('.png').name
    if not args.readcsv:
        g2g_delays = get_measurements_serial(args.num_measurements, args.quiet)
        # Post-processing
        time.sleep(.1)
        # this function returns g2g_delays as a numpy array
        g2g_delays, stats = generate_stats(g2g_delays)
        write_measurements_csv(filename, g2g_delays, stats)
    else:
        print(f"Reading data from {filename}")
        # this function returns g2g_delays as a list
        g2g_delays, stats = get_measurements_csv(filename)

    # save plot to a png file and display it. Data type doesn't seem to matter
    plot_results(g2g_delays, stats, fig_name)
