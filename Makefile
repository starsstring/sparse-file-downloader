CC ?= gcc
CPPFLAGS ?=
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDFLAGS ?=
LDLIBS ?=
TARGET := sparse_downloader

.PHONY: all test clean
all: $(TARGET)

$(TARGET): sparse_downloader.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

test: $(TARGET)
	python3 tests/test_downloader.py ./$(TARGET)

clean:
	rm -f $(TARGET)
