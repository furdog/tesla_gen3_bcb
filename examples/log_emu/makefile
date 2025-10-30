.PHONY: all build

# Variables
INCLUDE_PATHS := -I../../ -Icanary_log_reader/
HEADER_FILES := *.h
SOURCE_FILES := *.c
OUTPUT_FILE := main_out

# Default target
all: test

# Target for compiling and running tests
test: $(SOURCE_FILES)
	# Compile the test source file(s)
	gcc $(INCLUDE_PATHS) $(SOURCE_FILES) -std=c89 -pedantic -Wall -Wextra \
	  -g -fsanitize=undefined -fsanitize-undefined-trap-on-error \
	  -o $(OUTPUT_FILE)
	# Run the compiled test executable
	./$(OUTPUT_FILE)
	# Clean up the test executable
	@rm -f $(OUTPUT_FILE)

clean:
	@rm -f $(OUTPUT_FILE)
