BUILDROOT_DIR=~/Desktop/buildroot/
TOOLCHAIN_PATH=${BUILDROOT_DIR}/output/host/bin

# CC := gcc
CC=${TOOLCHAIN_PATH}/aarch64-buildroot-linux-gnu-gcc
CFLAGS := -Wall -Wextra
INCLUDE = -Iinclude
LDFLAGS = -lrt -lpthread -ldl -lseccomp

# CXX := g++
CXX=${TOOLCHAIN_PATH}/aarch64-buildroot-linux-gnu-g++
CXXFLAGS := -std=c++14 -Wall -Wextra

SRC_DIR := src
BUILD_DIR := build

HAL_SRC_DIR := ./hal
LIB_DIR := ./lib
HAL_TOY_DIR := $(HAL_SRC_DIR)/toy
HAL_OEM_DIR := $(HAL_SRC_DIR)/oem

TARGET := system

CC_SOURCES := $(wildcard $(SRC_DIR)/*.c)
CC_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CC_SOURCES))

HAL_TOY_LIB := $(LIB_DIR)/libcamera.toy.so
HAL_OEM_LIB := $(LIB_DIR)/libcamera.oem.so

HAL_TOY_SRCS := $(wildcard $(HAL_TOY_DIR)/*.cpp)
HAL_TOY_OBJS := $(patsubst $(HAL_TOY_DIR)/%.cpp,$(BUILD_DIR)/toy/%.o,$(HAL_TOY_SRCS))

HAL_OEM_SRCS := $(wildcard $(HAL_OEM_DIR)/*.cpp)
HAL_OEM_OBJS := $(patsubst $(HAL_OEM_DIR)/%.cpp,$(BUILD_DIR)/oem/%.o,$(HAL_OEM_SRCS))


all: $(TARGET) $(HAL_TOY_LIB) $(HAL_OEM_LIB)

$(TARGET): $(CC_OBJECTS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(CC_OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

# HAL
$(HAL_TOY_LIB): $(HAL_TOY_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -shared $^ -o $@

$(HAL_OEM_LIB): $(HAL_OEM_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -shared $^ -o $@

$(BUILD_DIR)/toy/%.o: $(HAL_TOY_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -fPIC -c $< -o $@

$(BUILD_DIR)/oem/%.o: $(HAL_OEM_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -fPIC -c $< -o $@



clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(LIB_DIR)