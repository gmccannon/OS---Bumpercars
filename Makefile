CC = gcc
CFLAGS = -Wall -Wextra -pthread

TARGET = bumpercars
SRC = bumpercars.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean

