TARGET = toy_system

SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server
HAL = ./hal

objects = main.o system_server.o web_server.o input.o gui.o toy.o common.o dump_state.o hardware.o
# cxx_objects = camera_HAL.o ControlThread.o
CXXLIBS = -lrt -lpthread -ldl
CXX = g++
shared_libs = libcamera.oem.so libcamera.toy.so

$(TARGET): $(objects) $(cxx_objects) $(shared_libs)
	$(CXX) -o $(TARGET) $(cxx_objects) $(objects) $(CXXLIBS)

main.o:  main.c
	$(CC) -c main.c

system_server.o: $(SYSTEM)/system_server.h $(SYSTEM)/system_server.c
	$(CC) -c ./system/system_server.c

dump_state.o: $(SYSTEM)/dump_state.h $(SYSTEM)/dump_state.c
	$(CC) -c ./system/dump_state.c

gui.o: $(UI)/gui.h $(UI)/gui.c
	$(CC) -c $(UI)/gui.c

input.o: $(UI)/input.h $(UI)/input.c
	$(CC) -c $(UI)/input.c

toy.o: $(UI)/input/toy.h $(UI)/input/toy.c
	$(CC) -c $(UI)/input/toy.c

web_server.o: $(WEB_SERVER)/web_server.h $(WEB_SERVER)/web_server.c
	$(CC) -c $(WEB_SERVER)/web_server.c

common.o: ./common.h common.c
	$(CC) -c common.c

hardware.o: $(HAL)/hardware.c
	$(CC) -g $(INCLUDES) -c  $(HAL)/hardware.c

# camera_HAL.o: $(HAL)/camera_HAL.cpp
# 	$(CXX) -c  $(HAL)/camera_HAL.cpp

# ControlThread.o: $(HAL)/ControlThread.cpp
# 	$(CXX) -c  $(HAL)/ControlThread.cpp

# .PHONY: libcamera.oem.so
libcamera.oem.so:
	$(CXX) -shared -fPIC -o libcamera.oem.so $(HAL)/oem/camera_HAL_oem.cpp $(HAL)/oem/ControlThread.cpp

# .PHONY: libcamera.toy.so
libcamera.toy.so:
	$(CXX) -shared -fPIC -o libcamera.toy.so $(HAL)/toy/camera_HAL_toy.cpp $(HAL)/toy/ControlThread.cpp


clean:
	rm -rf *.o *.so
	rm -rf $(TARGET)