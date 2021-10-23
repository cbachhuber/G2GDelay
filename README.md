# Glass to Glass Delay Measurement System

This repository contains all the information and software you need to build your own Glass-to-Glass delay measurement system. You also need the hardware detailed in the [Construction Manual](#construction-manual) below. There are three main components in this folder: the circuit layout (file [Circuit.pdf](Circuit.pdf)) for the measurement device, the Arduino source code (in folder [Arduino_code](Arduino_code)) and the optional Android Application (in folder [Android_App](Android_App)).

You have three options for retrieving G2G delay values from the Arduino:
- Quickest setup: Connect the measurement system to a computer via USB, and use the serial monitor from Arduino's IDE to retrieve G2G delay values.
- Relatively simple setup, convenient usage: Connect the measurement system to a computer via USB, and use [delayrecorder.py](delayrecorder.py) to generate a CSV file and histogram plot (and PNG) of the results with statistics.
- More complex setup, very convenient usage: instead of using a computer, connect the Arduino to an Android device using Bluetooth. The provided Android application will guide the user throught the measurement process.

The instructions for each option are given below. The setup steps for building the measurement system itself are described in the following:

## Construction Manual

For building the measurement device, you need the following parts, depicted in [Circuit.pdf](Circuit.pdf):

- Arduino Mega 2560: Does not have to be original Arduino, can also be e.g. a SunFounder Mega 2560
- LED: A Light-emitting diode, e.g. LED 5-4500 RT
- Phototransistor: For example the SDP 8406-003
- Resistor 11kOhm: Any 11kOhm resistor will do the job, e.g. the 1/4W 11K
- Cables
- Optional for Bluetooth usage: HC-05. For example the Aukru HC-05 Wireless-Bluetooth-Host Serial-Transceiver-Module
- Optional: 9V battery. You don't have to use the 9V battery as power supply, instead you can for example connect the Arduino to a USB port.
- Optional: A breadboard

Next, connect the elements as in [Circuit.pdf](Circuit.pdf). In `Circuit.pdf`, the principle of the circuit is shown, you can simplify it and waive for example the breadboard. Make sure that pin A0 is not connected to anything (or pick a new analog pin for it) - it needs to be floating for the randomness to work properly.

## Software Setup

### Arduino Source Code
To program the device, download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software) and install the TimerOne library provided in the [Arduino_code](Arduino_code) folder. For compatibility, be sure to use the one which was provided rather than installing via the libary manager or by finding it online. Now you can compile and upload the code to the Arduino board via USB.

### Python Script
To view the measurements using the provided script, install `python3` and ensure you have python modules `pyserial`, `matplotlib`, and `numpy`.

### Android Application
To view the measurements using the provided Anroid app, first copy the file [G2GDelay.apk](Android_App/G2GDelay.apk) to your Android device running Android 5.0 or higher. Make sure to enable the option 'Unknown Sources' in `System Settings/Security` before attempting to install the APK.

## Hardware Setup

### Powering the Device
When taking measurements with the Arduino IDE or the python script, connect the Arduino to the USB port of a computer. For use with the Android app, from some users we heard that the USB port of a laptop/PC might not give enough power for bluetooth. In that case, the bluetooth connection will not be stable. Connect the Arduino to another power source, such as a smartphone charger or a 9V battery.

### Phototransistor Placement
I strongly recommend to align the Phototransistor directly to the LED in the beginning. The phototransistor has a little knob on one side, this should point towards the LED and be very close to it, the know may even touch the LED. This way, you can check whether the Phototransistor is correctly connected to the circuit. You should see samples with a delay of 0 milliseconds. This makes sense because there is nothing delaying the propagation of light between the LED and Phototransistor.

### LED Placement
Next, for testing the G2G latency of a video transmission system (e.g. your smartphone, with the camera application started), you can put the LED in the field of view of a camera and put the PT on the corresponding display where the LED is shown. Make sure to place the PT on the LED and let the knob on the PT face towards the screen.

## Taking Measurements
Once a program establishes communication with the Arduino, the LED should light up twice in the beginning, signalling that the Arduino started without an error. If it doesn't, try to flip the polarity of the LED contacts.

### Arduino IDE
Run the Arduino IDE and open the serial monitor, you will be able to directly view the delay measurements.

### Python Script
The python3 script [delayrecorder.py](delayrecorder.py) will by default take 100 measurements, write to a CSV file called `results.csv`, pop up a histogram with some statistics (mean, median, standard deviation, minimum value, maximum value), and save it to `results.png`. This script also allows you to view previously saved measurements.

You can call `delayrecorder.py` with the target number of measurements and with flags to make it less verbose or to read a previously written CSV file. Call `delayrecorder.py -h` for more usage details.

### Android Device
Start the Android application, allow it to control the bluetooth adaptor of your Android device, open the side menu and go to 'device management'. If the Arduino is not yet paired with your phone or tablet, search for it. Tap on 'HC-05' to connect to your measurement device (if this is the first time to connect, you have to enter the security code, which is usually 1234 or 0000). Now you can go to live measurement in the side menu and start the measurement using the top right dots.

## Troubleshooting

If you do not see any samples coming in, first try to bring the phototransistor (PT) closer to the LED, and align it more carefully. If that does not succeed, try to flip the polarity of the PT contacts.

If the system is working when putting the LED directly up to the PT, but not when putting the LED on the screen, here are some additional strategies:
- The system is based on detecting a brightness increase at the PT. Therefore, maximize the screen brightness to minimize the influence of ambient light.
- Make sure that the environment of the turned off LED is depicted as dark as possible on the screen, and that the enabled LED is sufficiently bright.
- You might want to attach the PT with an adhesive film strip to the monitor, if you tend to lose the location of the LED while manually holding the PT to the screen.

## Conclusion

Congratulations, you are now all set to do your very own Glass-to-Glass delay measurements! If you use the system in course of your research, please reference our corresponding paper ["A System for High Precision Glass-to-Glass Delay Measurements in Video Communication"](https://doi.org/10.1109/ICIP.2016.7532735). Thank you very much!

    @inproceedings{bachhuber2016system,
      title={A System for High Precision Glass-to-Glass Delay Measurements in Video Communication},
      author={Bachhuber, Christoph and Steinbach, Eckehard},
      booktitle={IEEE International Conference on Image Processing (ICIP)},
      pages={2132--2136},
      year={2016},
    }

Further details about the measurement system are provided on [arXiv](https://arxiv.org/abs/1510.01134v1). Feel free to contact me (christoph dot bachhuber at tum dot de) in case you run into any difficulties.
