# THIS REPOSITORY IS GOING AWAY SOON. NEWER CODE IS HERE:
https://github.com/adafruit/Teensy3.1_Eyes


# Teensy-Iris
Moving iris for animatronic eye based on concept by David Boccabella: http://forums.stanwinstonschool.com/discussion/1470/animatronic-pupil-for-eye

This requires a PJRC Teensy 3.1 microcontroller. This WILL NOT WORK on normal Arduino or other boards -- the code is much too large. Teensy should be run at 72 MHz; don't use 96 MHz overclock mode right now.

As written, the code writes the same image to two different displays: an Adafruit 1.5" TFT LCD (product #2088) and 1.5" color OLED (product #1431), though a Freetronics OLED is also likely interchangeable for the latter. Each requires its own library installation and you'll need to tweak some includes and/or wiring configuration in this code.

'IrisTFT' directory contains Arduino sketch for Teensy 3.1.

'Python' directory contains script for generating image map and polar coordinate tables (redirect output to header file or copy-and-paste from terminal). Pass image filename as a command-line argument, e.g.:
python tablegen.py colormap404.png > eyeData.h
