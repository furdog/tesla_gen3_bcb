# Define the board package to install
BUILD_FLAGS   := --config-file arduino-cli.yaml
BOARD_PACKAGE := esp32:esp32

# Flag to enable continuous monitoring
AUTORECONNECT ?= true

# Files to copy
SOURCE_FILES := ../../*.h
DEST_DIR := ./src/

# Define the FQBN and port for upload
FQBN := esp32:esp32:esp32c6:CDCOnBoot=cdc
PORT ?= /dev/ttyUSB0
BAUD ?= 921600

all: compile upload monitor

copy-files:
	mkdir -p $(DEST_DIR)
	cp $(SOURCE_FILES) $(DEST_DIR)

compile: copy-files
	arduino-cli $(BUILD_FLAGS) compile -b $(FQBN)

upload:
ifeq ($(AUTORECONNECT),true)
	while true; do \
		arduino-cli $(BUILD_FLAGS) upload -b $(FQBN) -p $(PORT); \
		if [ $$? -eq 0 ]; then break; fi; \
		sleep 1; \
	done
else
	arduino-cli $(BUILD_FLAGS) upload -b $(FQBN) -p $(PORT);
endif

monitor:
ifeq ($(AUTORECONNECT),true)
	while true; do \
		arduino-cli $(BUILD_FLAGS) monitor -p $(PORT) \
			--config baudrate=$(BAUD); \
		sleep 1; \
	done
else
	arduino-cli $(BUILD_FLAGS) monitor -p $(PORT);
endif

board-install:
	arduino-cli core update-index
	arduino-cli core install $(BOARD_PACKAGE)

.PHONY: all compile board-install copy-files upload monitor
