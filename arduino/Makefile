PROJECT := SerialVectorGenerator
FQBN := arduino:avr:uno
#FQBN := arduino:megaavr:nona4809
#BOARD := /dev/ttyACM0
BOARD := $(shell arduino-cli board list | grep arduino:avr:uno | head -n 1 | sed -e 's/ .*//')
#BOARD := $(shell arduino-cli board list | grep arduino:megaavr:nona4809 | head -n 1 | sed -e 's/ .*//')

#find:
#	echo Board '$(BOARD)'

all:
	arduino-cli compile --fqbn $(FQBN) $(PROJECT)

upload: all
	arduino-cli upload -p $(BOARD) --fqbn $(FQBN) $(PROJECT)

show:
	arduino-cli compile --fqbn $(FQBN) --show-properties $(PROJECT)

connect:
	minicom -D $(BOARD) -b 115200

clean:
	arduino-cli compile --clean --fqbn $(FQBN) $(PROJECT)
