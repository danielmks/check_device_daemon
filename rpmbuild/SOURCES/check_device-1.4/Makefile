CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lnetsnmp

SRCS = main.c daemon.c metrics.c alarms.c logging.c
OBJS = $(SRCS:.c=.o)
TARGET = check_device

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)
	install -d /etc/systemd/system
	install -m 0644 check_device.service /etc/systemd/system/check_device.service

.PHONY: all clean install
