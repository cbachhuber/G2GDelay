# Glass to Glass Delay Measurement System

![Build](https://github.com/cbachhuber/G2GDelay/actions/workflows/build_arduino_code.yml/badge.svg)

This repository contains all the information and software you need to build your own Glass-to-Glass delay measurement system. You also need the hardware detailed in the [Construction Manual](https://github.com/cbachhuber/G2GDelay#construction-manual) below. There are three main components in this folder: the circuit layout (file [Circuit.pdf](https://github.com/cbachhuber/G2GDelay/blob/master/Circuit.pdf)) for the measurement device, the Arduino source code (in folder [Arduino_code](https://github.com/cbachhuber/G2GDelay/tree/master/Arduino_code)) and the optional Android Application (in folder [Android_app](https://github.com/cbachhuber/G2GDelay/tree/master/Android_App)). 

You have two options for retrieving G2G delay values from the Arduino:
- Quick setup, not so convenient usage: connect the measurement system to a computer via USB, and use [delayrecorder.py](https://github.com/cbachhuber/G2GDelay/blob/master/delayrecorder.py) to guide you through the measurement process. Even quicker: use the serial monitor of Arduino's IDE to retrieve G2G delay values.
- More complex setup, very convenient usage: instead of using a computer, connect the Arduino to an Android device using Bluetooth. The provided Android application will guide the user throught the measurement process.

The instructions for both options are given below. The setup steps for the measurement system itself are described in the following:

## Construction Manual

For building the measurement device, you need the following parts, depicted in [Circuit.pdf](https://github.com/cbachhuber/G2GDelay/blob/master/Circuit.pdf):

- Arduino Mega 2560: Does not have to be original Arduino, can also be e.g. a SunFounder Mega 2560
- LED: A Light-emitting diode, e.g. LED 5-4500 RT
- Phototransistor: For example the SDP 8406-003
- Resistor 11kOhm: Any 11kOhm resistor will do the job, e.g. the 1/4W 11K
- Cables
- Optional for Bluetooth usage: HC-05. For example the Aukru HC-05 Wireless-Bluetooth-Host Serial-Transceiver-Module
- Optional: 9V battery. You don't have to use the 9V battery as power supply, instead you can for example connect the Arduino to a USB port.
- Optional: A breadboard

Next, connect the elements as in [Circuit.pdf](https://github.com/cbachhuber/G2GDelay/blob/master/Circuit.pdf). In [Circuit.pdf](https://github.com/cbachhuber/G2GDelay/blob/master/Circuit.pdf), the principle of the circuit is shown, you can simplify it and waive for example the breadboard. Make sure that pin A0 is not connected to anything (or pick a new analog pin for it) - it needs to be floating for the randomness to work properly.

## Arduino source code

[Download the Arduino IDE](https://www.arduino.cc/en/Main/Software), open the Arduino_code project.
From the IDE, you can compile the code and then upload the binary to the Arduino board via USB.

## Measuring with Computer

The quickest way to get measurements is connecting the arduino to a computer using USB and observe the output on the serial console that comes with the Arduino IDE, or, more convenient, use the script delayrecorder.py to observe, process and store your measurements. Once the Arduino is connected to the USB port of a computer running the serial monitor of the arduino IDE, continue reading at [Starting Measurements](https://github.com/cbachhuber/G2GDelay#starting-measurements).

## Measuring with Android Device

### Android Application

Copy the file G2GDelay.apk from the folder Android_app to your Android device running Android 5.0 or higher. Make sure to enable the option 'Unknown Sources' in System Settings/Security before attempting to install the APK.

### Starting the Arduino and the Android Application

Connect the Arduino to its power source (from some users we heard that the USB port of a laptop/PC might not give enough power. In that case, the bluetooth connection will not be stable. Connect the Arduino to another power source, such as a smartphone charger or a 9V battery). The LED has to light up twice in the beginning, signalling that the Arduino started without an error. If it doesn't, try to flip the polarity of the LED contacts.

After that, start the Android application, allow it to control the bluetooth adaptor of your Android device, open the side menu and go to 'device management'. If the Arduino is not yet paired with your phone or tablet, search for it. Tap on 'HC-05' to connect to your measurement device (if this is the first time to connect, you have to enter the security code, which is usually 1234 or 0000). Now you can go to live measurement in the side menu and start the measurement using the top right dots.

## Starting Measurements

I strongly recommend to align the Phototransistor directly to the LED in the beginning. The phototransistor has a little knob on one side, this should point towards the LED and be very close to it, the know may even touch the LED. This way, you can check whether the Phototransistor is correctly connected to the circuit. You should see samples with a delay of 0 milliseconds on your Android device. This makes sense because there is nothing delaying the propagation of light between the LED and Phototransistor. 

Next, for testing the G2G latency of a video transmission system (e.g. your smartphone, with the camera application started), you can put the LED in the field of view of a camera and put the PT on the corresponding display where the LED is shown. Make sure to place the PT on the LED and let the knob on the PT face towards the screen. 

## Troubleshooting

If you do not see any samples coming in, first try to adjust the position of the Phototransistor relative to the LED, if that does not succeed, try to flip the polarity of the Phototransistor contacts. Additional hints, if the system is working when putting the LED directly to the PT, but not when putting the LED on the screen:

- The system is based on detecting a brightness increase at the PT. Therefore, maximize the screen brightness to minimize the influence of ambient light.
- Make sure that the environment of the turned off LED is depicted as dark as possible on the screen, and that the enabled LED is sufficiently bright.
- You might want to attach the PT with an adhesive film strip to the monitor, if you tend to lose the location of the LED while manually holding the PT to the screen.


Congratulations, you are now all set to do your very own Glass-to-Glass delay measurements! If you use the system in course of your research, please reference our corresponding paper ["A System for High Precision Glass-to-Glass Delay Measurements in Video Communication"](https://doi.org/10.1109/ICIP.2016.7532735). Thank you very much!

    @inproceedings{bachhuber2016system,
      title={A System for High Precision Glass-to-Glass Delay Measurements in Video Communication},
      author={Bachhuber, Christoph and Steinbach, Eckehard},
      booktitle={IEEE International Conference on Image Processing (ICIP)},
      pages={2132--2136},
      year={2016},
    }


Further details about the measurement system are provided on [arXiv](https://arxiv.org/abs/1510.01134v1). Feel free to contact me (christoph dot bachhuber at tum dot de) in case you run into any difficulties.
