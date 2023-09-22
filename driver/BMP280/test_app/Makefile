BUILDROOT_DIR=~/Desktop/buildroot
TOOLCHAIN_PATH=${BUILDROOT_DIR}/output/host/bin
CC=${TOOLCHAIN_PATH}/aarch64-buildroot-linux-gnu-gcc

CFLAGS := -Wall -Wextra

TARGET := bmp280_test

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) $(CFLAGS) $(TARGET).o -o $(TARGET)


clean:
	rm -rf *.o $(TARGET)