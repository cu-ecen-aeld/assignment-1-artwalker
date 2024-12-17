CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -Werror
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = writer

.PHONY: all clean

all: $(TARGET)

$(TARGET): writer.o
	$(CC) $(CFLAGS) -o $(TARGET) writer.o

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c

clean:
	rm -f $(TARGET) *.o