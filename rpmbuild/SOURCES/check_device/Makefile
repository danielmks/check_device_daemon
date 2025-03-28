CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lnetsnmp -laxio -L/usr/lib64 -Wl,-rpath,'$$ORIGIN/../lib64'

SRCS = main.c daemon.c metrics.c alarms.c logging.c config.c fanmonitor.c
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
