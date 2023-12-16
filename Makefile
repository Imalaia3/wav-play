CC = g++
CFLAGS = -lSDL2 -g -Wall
TARGET = wavplay


$(TARGET): *.cpp
	$(CC) $? $(CFLAGS) -o $(TARGET)

clean:
	rm $(TARGET)