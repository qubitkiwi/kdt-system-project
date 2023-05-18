TARGET = toy_system

SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server
HAL = ./hal

objects = main.o system_server.o web_server.o input.o gui.o toy.o common.o
cxx_objects = camera_HAL.o ControlThread.o

CXX = g++

$(TARGET): $(objects) $(cxx_objects)
	$(CXX) -o $(TARGET) $(cxx_objects) $(objects) -lrt -lpthread

main.o:  main.c
	$(CC) -c main.c

system_server.o: $(SYSTEM)/system_server.h $(SYSTEM)/system_server.c
	$(CC) -c ./system/system_server.c

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

camera_HAL.o: $(HAL)/camera_HAL.cpp
	$(CXX) -c  $(HAL)/camera_HAL.cpp

ControlThread.o: $(HAL)/ControlThread.cpp
	$(CXX) -c  $(HAL)/ControlThread.cpp


clean:
	rm -rf *.o
	rm -rf $(TARGET)