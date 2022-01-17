all: compile upload serial

compile:
	arduino-cli compile --fqbn arduino:avr:uno

upload:
	sudo arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno

serial:
	sudo cat /dev/ttyACM0
