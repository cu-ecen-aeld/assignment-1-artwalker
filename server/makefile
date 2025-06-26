#CC = $(CROSS_COMPILE)gcc
CC ?= gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) aesdsocket.c

clean:
	rm -f $(TARGET)
