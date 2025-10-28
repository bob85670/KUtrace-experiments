# Makefile
CXX = g++
STD = -std=c++20
OPT = -O3 -march=native -mtune=native -flto
WARN = -Wall -Wextra -Wpedantic          # <<< removed -Werror
INC = -I.
LIBS = -pthread -lrt

CFLAGS = $(STD) $(OPT) $(WARN) $(INC) \
         -fno-exceptions -fno-rtti \
         -DNDEBUG \
         -mavx2 -mfma

TARGET = hft_timestamp_engine
SOURCES = main.cpp
HEADERS = simd_unwrap.hpp ring_spsc.hpp dpdk_receiver.hpp

.PHONY: all clean run bench

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CFLAGS) $(SOURCES) -o $@ $(LIBS)

run: $(TARGET)
	sudo taskset -c 0,1 ./$(TARGET)

clean:
	rm -f $(TARGET)

bench: $(TARGET)
	sudo taskset -c 0,1 stdbuf -o0 ./$(TARGET) > /dev/null

restart: clean all run