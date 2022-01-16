#!/bin/zsh

arduino-cli compile --fqbn arduino:avr:uno
sudo arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno
