#!/bin/bash

# Define paths
SOURCE_DIR="."
POSTPROC_DIR="../KUtrace/postproc"  # Relative path to postproc
OUTPUT_DIR="."    # Output directory
BINARY="trace_example"

# Compiler and flags
CC=g++
CFLAGS="-O2 -Wall"                  # Optimization and warnings
INCLUDE="-I${POSTPROC_DIR}"         # Include path for headers
SOURCES="${SOURCE_DIR}/trace_example.cc ${POSTPROC_DIR}/kutrace_lib.cc"  # Source files

# Create output directory if it doesn't exist
mkdir -p ${OUTPUT_DIR}

# Compile and link
${CC} ${CFLAGS} ${INCLUDE} -o ${OUTPUT_DIR}/${BINARY} ${SOURCES}

# Check if compilation succeeded
if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable: ${OUTPUT_DIR}/${BINARY}"
    echo "Run ./KUtrace-experiments/trace_example to generate a .trace file."
else
    echo "Compilation failed. Check errors above."
    exit 1
fi
