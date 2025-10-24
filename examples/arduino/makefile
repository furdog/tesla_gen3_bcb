# Define the board package to install
BOARD_PACKAGE := esp32:esp32

# Files to copy
SOURCE_FILES := ../../*.h
DEST_DIR := ./src/

# Define the FQBN and port for upload
FQBN := esp32:esp32:esp32c6
PORT ?= /dev/ttyUSB0

all: compile upload monitor

copy-files:
	mkdir -p $(DEST_DIR)
	cp $(SOURCE_FILES) $(DEST_DIR)

compile: copy-files
	arduino-cli --config-file arduino-cli.yaml compile -b $(FQBN)

upload:
	arduino-cli --config-file arduino-cli.yaml upload -b $(FQBN) -p $(PORT)

monitor:
	arduino-cli --config-file arduino-cli.yaml monitor -p $(PORT)

board-install:
	arduino-cli core update-index
	arduino-cli core install $(BOARD_PACKAGE)

.PHONY: all compile board-install copy-files upload monitor
