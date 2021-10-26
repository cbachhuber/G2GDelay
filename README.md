# Glass to Glass Delay Measurement System

![Build](https://github.com/cbachhuber/G2GDelay/actions/workflows/build_arduino_code.yml/badge.svg)

This repository contains all the information and software you need to build your own Glass-to-Glass delay measurement system. You will need the hardware detailed in the [Construction Manual](#construction-manual) below. After that, follow the [software setup](#software-setup) and [hardware setup](#hardware-setup) and you'll be good to go to [take some measurements](#taking-measurements)!

There are three main components in this repository: the circuit layout (file [circuit.svg](circuit.svg)) for the measurement device, the Arduino source code (in folder [Arduino_code](Arduino_code)) and a Python script for taking measurements and plotting them with statistics (file [delayrecorder.py](delayrecorder.py)). The quickest way to view measurements is to read the values directly from the Arduino IDE's Serial Monitor, and the most convenient is to run the script.

## Construction Manual

For building the measurement device, you need the following parts:
- Arduino Mega 2560: Does not have to be original Arduino, can also be e.g. a SunFounder Mega 2560
- LED: A Light-emitting diode, e.g. LED 5-4500 RT
- Phototransistor: For example the SDP 8406-003
- Resistor 11kOhm: Any 11kOhm resistor will do the job, e.g. the 1/4W 11K
- Wires
- Optional: A breadboard

Next, connect the elements as such:

<p align="center">
  <img width="960" src="./circuit.svg">
</p>

You can simplify the above circuit and waive for example the breadboard.
Make sure that pin A0 is not connected to anything (or pick a new analog pin for it) - it needs to be floating for the randomness to work properly.

## Software Setup

### Arduino Source Code
To program the device, download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software). From the IDE, you can compile the code and then upload the binary to the Arduino board via USB.

### Python Script
To be able to view the measurements using the provided script, install `python3` and ensure you have python modules `pyserial`, `matplotlib`, and `numpy`.

## Hardware Setup

### Powering the Device
Connect the Arduino to the USB port of a computer.

### Phototransistor Placement
I strongly recommend to align the Phototransistor (PT) directly to the LED in the beginning. The PT has a little bump on one side, this should point towards the LED and be very close to it and even touch the LED. This way, you can check whether the PT is correctly connected to the circuit. You should see samples with a delay of 0.00 milliseconds. This makes sense because there is nothing delaying the propagation of light between the LED and PT

### LED Placement
Next, for testing the G2G latency of a video transmission system (e.g. your smartphone, with the camera application started), you can put the LED in the field of view of a camera and put the PT on the corresponding display where the LED is shown. Make sure to place the PT on the LED and let the bump on the PT face towards the screen.

## Taking Measurements

Once a program establishes communication with the Arduino, the LED should light up twice in the beginning, signalling that the Arduino started without an error. If it doesn't, try to flip the polarity of the LED contacts.

### Arduino IDE
This is the quickest way to get measurements. Run the Arduino IDE and open the Serial Monitor, you will be able to directly view the delay measurements.

### Python Script
This is a convenient way to observe and interpret measurements. [delayrecorder.py](delayrecorder.py) will take the desired filename and number of measurements, and it will generate a histogram with statistics and save the results. You can call it with `./delayrecorder.py -h` for more usage details and other options.

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
