# Teensy-Iris
Moving iris for animatronic eye based on concept by David Boccabella: http://forums.stanwinstonschool.com/discussion/1470/animatronic-pupil-for-eye

This version uses an Adafruit 1.5" TFT LCD (product #2088) and a PJRC Teensy 3.1 microcontroller. This WILL NOT WORK on normal Arduino or other boards -- the code is much too large.

Should look amazing on an OLED (e.g. www.adafruit.com/product/1431) but I don't have one around at the moment. This code is written for the TFT above and will not work with the OLED unless a few changes are made.

'IrisTFT' directory contains Arduino sketch for Teensy 3.1.

'Python' directory contains script for generating image map and polar coordinate tables (redirect output to header file or copy-and-paste from terminal). Pass image filename as a command-line argument, e.g.:
python tablegen.py colormap404.png > eyeData.h
