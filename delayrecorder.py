#!/usr/bin/env python3

import sys, argparse, signal
import time, serial, serial.tools.list_ports
import csv
import numpy as np
import matplotlib.pyplot as plt

class Stats(object):
    def __init__(self, num_measurements, min, max, mean, median, std):
        self.num_measurements = num_measurements
        self.min = min
        self.max = max
        self.mean = mean
        self.median = median
        self.std = std

def sigint_handler(signal, frame):
    print("Caught KeyboardInterrupt, exiting.")
    sys.exit(0)

def get_measurements_serial(num_measurements, quietMode):
    # Search for Arduino
    devs = serial.tools.list_ports.comports()
    found = 0

    for dev in devs:
        if dev.manufacturer is not None:
            if "Arduino" in dev.manufacturer:
                print("Found Arduino at " + dev[0])
                ser = serial.Serial(dev[0], 115200,timeout=5)
                found = 1

    if found == 0:
        print("Did not find Arduino on any serial port. Is it connected?")
        sys.exit()

    print("Collecting " + str(num_measurements) + " measurements from the Arduino")
    if quietMode:
        print("Running in quiet mode, won't print the measurements to the terminal")

    # Read messages from Arduino
    timeout = time.time() + 0.01
    while True:
        a = ser.readline().decode()
        if time.time() > timeout:
            break

    measurements = []
    i = 0
    overallrounds = 0
    initmessage = 0

    while (i < num_measurements):
        overallrounds = overallrounds + 1
        a = ser.readline().decode()
        if "." in a:
            initmessage = 1
            i = i + 1
            a = a.replace("\n","")
            a = a.replace("\r","")
            measurements.append(float(a))
            if not quietMode:
                print("G2G Delay trial " + str(i) + "/" + str(num_measurements) + ": " + a + " ms")
            else:
                print("G2G Delay trial " + str(i) + "/" + str(num_measurements), end='\r')
        else:
            if overallrounds > 0 and initmessage == 1:
                print("Did not receive msmt data from the Arduino for another 5 seconds. Is the phototransistor still sensing the LED?")
            else:
                print("Did not receive msmt data from the Arduino for 5 seconds. \n Is the phototransistor sensing the LED on the screen? \n Is the correct side of the PT pointing towards the screen (the flat side with the knob on it)? \n Is the screen brightness high enough (max recommended)?")
                initmessage = 1

    return measurements

def write_measurements_csv(filename, measurements, stats):
    with open(filename, 'w') as f:
        writer = csv.writer(f)
        writer.writerow(measurements)
        header = ['Samples', 'Min', 'Max', 'Mean', 'Median', 'Std']
        writer.writerow(header)
        stats_list = [stats.num_measurements, stats.min, stats.max, stats.mean, stats.median, stats.std]
        writer.writerow(stats_list)
    print("Saved results to " + args.filename)

def get_measurements_csv(filename):
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        # first row has the datapoints, 3rd row has the stats
        for i, row in enumerate(reader):
            if i == 0:
                measurements = [float(i) for i in row]
            if i == 2:
                # convert every string in the array into a float
                stats_list = [float(i) for i in row]
                # grab each element of the list to create the Stats object
                stats = Stats(*stats_list)
    print("Obtained values from " + filename)

    return measurements, stats

def generate_stats(measurements):
    # Obtain general stats
    measurements_np = np.array(measurements)
    min = np.min(measurements_np)
    max = np.max(measurements_np)
    mean = np.mean(measurements_np)
    median = np.median(measurements_np)
    std = np.std(measurements_np)
    print("Min/Max G2G Delay: " + '%.2f' % (min) + "/" + '%.2f' % (max) + " ms")
    print("Mean/Median/Std G2G Delay: " + '%.2f' % (mean) + "/" + '%.2f' % (median) + "/" + '%.2f' % (std) + " ms")

    return measurements_np, Stats(len(measurements_np), min, max, mean, median, std)

def plot_results(measurements, stats, figName):
    plt.hist(measurements, bins=20)
    plt.gcf().canvas.manager.set_window_title(figName)
    plt.title('Latency Histogram')
    plt.xlabel('Latency (ms)')
    plt.ylabel('Frequency')

    ax = plt.gca()
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    textstr1 = '\n'.join((
        r'$\mathrm{min}=%.2f$' % (stats.min, ),
        r'$\mathrm{max}=%.2f$' % (stats.max, )))
    # place it at position x=0.05, y=0.95, relative to the top and left of the box
    ax.text(0.05, 0.95, textstr1, transform=ax.transAxes, fontsize=14,
        verticalalignment='top', horizontalalignment='left', bbox=props)
    textstr2 = '\n'.join((
        r'$\mu=%.2f$' % (stats.mean, ),
        r'$\mathrm{median}=%.2f$' % (stats.median, ),
        r'$\sigma=%.2f$' % (stats.std, )))
    # place it at position x=0.95, y=0.90, relative to the top and right of the box
    ax.text(0.95, 0.95, textstr2, transform=ax.transAxes, fontsize=14,
        verticalalignment='top', horizontalalignment='right', bbox=props)

    plt.savefig(figName)
    print("Saved histogram to " + figName)
    plt.show()

if __name__ == "__main__":
    # Set up signal handler to handle keyboard interrupts
    signal.signal(signal.SIGINT, sigint_handler)

    # Parse input arguments
    parser = argparse.ArgumentParser(description="This script obtains delay measurements from a Glass-to-Glass device connected via USB, saves results and statistics to a CSV file, and displays the results in a histogram plot. You can also use it to display results from saved measurements in a previous test, in which case the CSV file is read from rather than written to.")

    parser.add_argument('filename', nargs='?', default="results.csv", type=str, help="The name of the CSV file where the data is saved. It is also the name for the saved plot. Default is 'results.csv' which will cause 'results.png' to be created as well. This is the filename used when using the '--readcsv' option as well.")
    parser.add_argument('num_measurements', nargs='?', default=100, type=int, help="An integer for the number of measurements to save and use when generating statistics. Default is 100.")
    parser.add_argument('--quiet', '-q', action='store_true', default=0, help="The script won't print the measurements to the terminal (but will still save them into the CSV file).")
    parser.add_argument('--readcsv', '-r', action='store_true', default=0, help="Reads a previously generated CSV and plots it. Be sure to provide the name of the CSV file if it's not the default name.")
    args = parser.parse_args()

    file_extension = args.filename.split('.')
    if len(file_extension) != 2 or file_extension[1] != 'csv':
        print("Error: Provided filename is invalid or does not have .csv extension")
        sys.exit()

    # Either write the data to a CSV file or read from a preexisting one
    figName = '.'.join(args.filename.split('.')[:-1]) + '.png'
    if not args.readcsv:
        G2Gdelays = get_measurements_serial(args.num_measurements, args.quiet)
        # Post-processing
        time.sleep(.1)
        # this function returns G2Gdelays as a numpy array
        G2Gdelays, stats = generate_stats(G2Gdelays)
        write_measurements_csv(args.filename, G2Gdelays, stats)
    else:
        print('Reading data from ' + args.filename)
        # this function returns G2Gdelays as a list
        G2Gdelays, stats = get_measurements_csv(args.filename)

    # save plot to a png file and display it. Data type doesn't seem to matter
    plot_results(G2Gdelays, stats, figName)
