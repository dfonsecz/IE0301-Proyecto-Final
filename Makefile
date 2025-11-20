CXX := g++
CXXFLAGS := -std=c++17 -O2 `pkg-config --cflags gstreamer-1.0`
LDFLAGS := `pkg-config --libs gstreamer-1.0`

MAIN := src/main.cpp
TARGET := $(basename $(notdir $(MAIN)))

SRC := $(wildcard src/**/*.cpp) $(MAIN)

TEST_DIR := test

# Compile all

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Execute test
# ...

clean:
	rm -rf $(TARGET)
	rm -rf *.o