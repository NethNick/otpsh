# Makefile for OTPSH
CC = gcc
CFLAGS = -Wall -g
LIBS = -lssl -lcrypto
TARGET = otpsh
CONFIG_FILE = ~/.otpsh

# default
all: $(TARGET)

# Main target
$(TARGET): otpsh.c
	$(CC) $(CFLAGS) otpsh.c -o $(TARGET) $(LIBS)

# Config
$(CONFIG_FILE):
	echo "# Encode secret as Base32 (example: 'dead bytes here')" > $(CONFIG_FILE)
	echo "secret=MRSWCZBAMJ4XIZLTEBUGK4TF" >> $(CONFIG_FILE)
	echo "command=/bin/bash" >> $(CONFIG_FILE)
	chmod 600 $(CONFIG_FILE)

# Cleanup
clean:
	rm -f $(TARGET) $(CONFIG_FILE)

# Test run
run: $(TARGET) $(CONFIG_FILE)
	./$(TARGET)

