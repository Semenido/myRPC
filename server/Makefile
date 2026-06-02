CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = myRPC-server
SRC = myRPC-server.c ../libmysyslog/mysyslog.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o