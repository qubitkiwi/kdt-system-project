CC := gcc
CFLAGS := -Wall #-Wextra

INCLUDE = -Iinclude
LDFLAGS = -lrt -lpthread -ldl -lseccomp

CXX := g++
CXXFLAGS := -Wall -Wextra

HALLIBS = -shared -fPIC
shared_libs = libcamera.oem.so libcamera.toy.so

SRC_DIR := src
BUILD_DIR := build
TARGET := toy_system

CC_SOURCES := $(wildcard $(SRC_DIR)/*.c)
CC_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CC_SOURCES))

all: $(TARGET)

$(TARGET): $(CC_OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(CC_OBJECTS) $(LDFLAGS)

# Rule to build C source files object files with gcc
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)  # Create the build directory if it doesn't exist
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

# Clean rule to remove all generated files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# -include $(DEPS)