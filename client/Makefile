CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = myRPC-client
SRC = myRPC-client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o