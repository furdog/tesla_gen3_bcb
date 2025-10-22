.PHONY: all docs misra test clean

# Variables
MISRA_REPO := https://github.com/furdog/MISRA.git
MISRA_DIR := MISRA
MISRA_SCRIPT := $(MISRA_DIR)/misra.sh
HEADER_FILES := *.h
SOURCE_FILES := *.test.c
TEST_OUTPUT := test_out
DOXYFILE := docs/Doxyfile

# Default target
all: misra test docs

# Target for running MISRA checks and setup
misra: $(MISRA_SCRIPT)
	@echo "--- Setting up and running MISRA checks ---"
	# Setup MISRA
	@$(MISRA_SCRIPT) setup
	# Run MISRA check
	@$(MISRA_SCRIPT) check $(HEADER_FILES)

# Prerequisite for 'misra': ensure the MISRA repository is cloned
$(MISRA_SCRIPT):
	@echo "--- Getting/Updating MISRA repository ---"
	if cd $(MISRA_DIR); then \
		echo "MISRA directory exists, pulling updates..."; \
		git pull; \
		cd ..; \
	else \
		echo "Cloning MISRA repository..."; \
		git clone $(MISRA_REPO); \
	fi
	# Check if misra.sh exists after cloning
	@if [ ! -f $(MISRA_SCRIPT) ]; then \
		echo "Error: $(MISRA_SCRIPT) not found after clone/update."; \
		exit 1; \
	fi

# Target for compiling and running tests
test: $(SOURCE_FILES)
	@echo "--- Compiling and running tests ---"
	# Compile the test source file(s)
	gcc $(SOURCE_FILES) -std=c89 -pedantic -Wall -Wextra -g \
	  -fsanitize=undefined -fsanitize-undefined-trap-on-error \
	  -o $(TEST_OUTPUT)
	# Run the compiled test executable
	./$(TEST_OUTPUT)
	# Clean up the test executable
	@rm -f $(TEST_OUTPUT)

# Target for generating documentation
docs: $(DOXYFILE)
	@echo "--- Generating documentation using Doxygen ---"
	# Generate documentation (the original script only ran this if a condition was *not* met,
	# but in a Makefile, the 'docs' target is explicitly triggered)
	doxygen $(DOXYFILE)

# Clean up generated files (optional but good practice)
clean:
	@echo "--- Cleaning up generated files ---"
	@rm -rf $(MISRA_DIR) # Remove the whole MISRA repo to reset
	@rm -f $(TEST_OUTPUT)
	@rm -rf docs/html docs/latex # Add other Doxygen output directories as needed
