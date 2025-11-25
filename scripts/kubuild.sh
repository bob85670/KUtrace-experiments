#!/bin/bash

# Define paths
SOURCE_DIR="."
POSTPROC_DIR="../KUtrace/postproc"  # Relative path to postproc
OUTPUT_DIR="."    # Where to place the executable
BINARY="trace_example"              # Default binary name

# Check if argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <cpp_file>"
    echo "Example: $0 trace_example.cc"
    exit 1
fi

CPP_FILE="$1"
CPP_NAME=$(basename "$CPP_FILE" .cc)  # Extract name without .cc extension

# Compiler and flags
CC=g++
CFLAGS="-O2 -Wall"                    # Optimization and warnings
INCLUDE="-I${POSTPROC_DIR}"           # Include path for headers
SOURCES="${SOURCE_DIR}/${CPP_FILE} ${POSTPROC_DIR}/kutrace_lib.cc"  # Source files

# Create output directory if it doesn't exist
mkdir -p ${OUTPUT_DIR}

# Compile and link
${CC} ${CFLAGS} ${INCLUDE} -o ${OUTPUT_DIR}/${BINARY} ${SOURCES}

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo "Compilation failed. Check errors above."
    exit 1
fi

echo "Compilation successful. Executable: ${OUTPUT_DIR}/${BINARY}"

# Run the executable to produce .trace file in POSTPROC_DIR
cd ${POSTPROC_DIR} || { echo "Failed to change to ${POSTPROC_DIR}"; exit 1; }
echo "Running ${OUTPUT_DIR}/${BINARY} to generate trace in ${POSTPROC_DIR}..."
${OUTPUT_DIR}/${BINARY}
TRACE_FILE=$(ls ku_*.trace 2>/dev/null | sort -r | head -n 1)  # Get latest .trace file

if [ -z "$TRACE_FILE" ]; then
    echo "No .trace file generated in ${POSTPROC_DIR}. Check if tracing worked."
    cd - > /dev/null
    exit 1
fi

echo "Trace file generated: $TRACE_FILE"

# Run postproc3.sh with trace file and cpp file name
echo "Running postproc3.sh..."
./postproc3.sh "$TRACE_FILE" "$CPP_FILE"
