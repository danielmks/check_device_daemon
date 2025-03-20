CC = gcc
CFLAGS = -Wall -O2
TARGET = check_device

.PHONY: all clean install

all: $(TARGET)

$(TARGET): check_device.c
	$(CC) $(CFLAGS) -o $(TARGET) check_device.c

clean:
	rm -f $(TARGET)

install: all
	# 바이너리를 /usr/local/bin 에 복사
	install -d /usr/local/bin
	install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)
	# systemd 서비스 파일 복사 (루트 권한 필요)
	install -d /etc/systemd/system
	install -m 0644 check_device.service /etc/systemd/system/check_device.service

