TARGET = toy_system

SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server

objects = main.o system_server.o web_server.o input.o gui.o

$(TARGET): $(objects)
	$(CC) -o $(TARGET) $(objects) -lrt

main.o:  main.c
	$(CC) -c main.c

system_server.o: $(SYSTEM)/system_server.h $(SYSTEM)/system_server.c
	$(CC) -c ./system/system_server.c

gui.o: $(UI)/gui.h $(UI)/gui.c
	$(CC) -c $(UI)/gui.c

input.o: $(UI)/input.h $(UI)/input.c
	$(CC) -c $(UI)/input.c

web_server.o: $(WEB_SERVER)/web_server.h $(WEB_SERVER)/web_server.c
	$(CC) -c $(WEB_SERVER)/web_server.c

clean:
	rm -rf *.o
	rm -rf $(TARGET)