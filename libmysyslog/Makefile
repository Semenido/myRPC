CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = libmysyslog.a
SRC = mysyslog.c
OBJ = mysyslog.o

all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
<<<<<<< HEAD
	rm -f $(TARGET) $(OBJ)
	
=======
	rm -f $(TARGET) $(OBJ)
>>>>>>> feature/logger
