# Makefile for KUtrace-experiments

# Compiler and flags
CXX := g++
CXXFLAGS := -O2 -Wall -I./include
LDFLAGS := -pthread

# Directories
SRC_DIR := src
BIN_DIR := bin
INCLUDE_DIR := include

# Find all C++ source files that contain a main function.
# We assume that each file with a main function is a separate program.
MAIN_SRCS := $(shell grep -l "main(" $(SRC_DIR)/*.cc $(SRC_DIR)/*.cpp)

# Create a list of executable names from the source files
# e.g. src/client4.cc -> bin/client4
TARGETS := $(patsubst $(SRC_DIR)/%.cc,$(BIN_DIR)/%,$(filter %.cc,$(MAIN_SRCS)))
TARGETS += $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%,$(filter %.cpp,$(MAIN_SRCS)))

# Common object files to be linked with every executable
# For now we will link all other .cc files as if they are common objects
# A better approach would be to specify dependencies for each target
OBJS := $(patsubst $(SRC_DIR)/%.cc,$(SRC_DIR)/%.o,$(filter-out $(MAIN_SRCS), $(wildcard $(SRC_DIR)/*.cc)))
OBJS += $(patsubst $(SRC_DIR)/%.cpp,$(SRC_DIR)/%.o,$(filter-out $(MAIN_SRCS), $(wildcard $(SRC_DIR)/*.cpp)))


.PHONY: all clean

# The 'all' target is the default target.
all: $(TARGETS)

# Rule to compile object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link each main source file into an executable
$(BIN_DIR)/%: $(SRC_DIR)/%.cc $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

$(BIN_DIR)/%: $(SRC_DIR)/%.cpp $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

# The 'clean' target removes all generated files.
clean:
	rm -rf $(BIN_DIR) $(SRC_DIR)/*.o