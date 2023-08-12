.PHONY: all clean

BUILD_DIR = build
TMP_DIR = tmp log

C_SOURCES = \
src/writer.c \
src/reader.c

C_INCLUDES = -Iinc

CC = gcc
CFLAGS = -Wall -std=gnu99

all: $(BUILD_DIR)/writer.out $(BUILD_DIR)/reader.out

$(BUILD_DIR)/writer.out: $(BUILD_DIR) src/writer.c
	$(CC) $(CFLAGS) $(C_INCLUDES) src/writer.c -o $@

$(BUILD_DIR)/reader.out: $(BUILD_DIR) src/reader.c
	$(CC) $(CFLAGS) $(C_INCLUDES) src/reader.c -o $@

$(BUILD_DIR):
	mkdir $@

writer: $(BUILD_DIR)/writer.out
	chmod +x $<
	$<

reader: $(BUILD_DIR)/reader.out
	chmod +x $<
	$<

clean:
	rm -rf $(BUILD_DIR) $(TMP_DIR)
