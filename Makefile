OBJS   = sv2020.o main.o
TARGET = sv2020
CFLAGS = -pedantic -Wall -Wextra -std=gnu99

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
