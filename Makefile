ARDUINO_HOME=/home/ingo/run/arduino-1.8.3
ARDUINO=${ARDUINO_HOME}/arduino
SOURCE=console.ino
OPTS=--board arduino:avr:nano \
     --verbose \

compile:
	${ARDUINO} --verify ${OPTS} ${SOURCE}

upload:
	${ARDUINO} --upload ${OPTS} ${SOURCE} --port /dev/ttyUSB0

.PHONY: compile upload

# EOF #
