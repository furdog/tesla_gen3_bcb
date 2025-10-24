# Define the board package to install
BOARD_PACKAGE := esp32:esp32

# Files to copy
SOURCE_FILES := ../../*.h
DEST_DIR := ./src/

all: compile

copy-files:
	@echo "--- Copying required files before compile ---"
	mkdir -p $(DEST_DIR) # Create destination directory if it doesn't exist
	cp $(SOURCE_FILES) $(DEST_DIR)
	@echo "--- File copying complete ---"

compile: copy-files
	arduino-cli --config-file arduino-cli.yaml compile -b esp32:esp32:esp32c6

board-install:
	# Ensure the index is updated before installing
	arduino-cli core update-index
	# Install the specified board package
	arduino-cli core install $(BOARD_PACKAGE)

.PHONY: all compile board-install copy-files
