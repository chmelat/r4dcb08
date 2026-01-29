#
# Makefile for r4dcb08-mqtt daemon
#

PROGRAM = r4dcb08-mqtt
VERSION = 1.0

# Source files
MQTT_SRC = mqtt_main.c mqtt_config.c mqtt_error.c mqtt_client.c mqtt_publish.c

# Reused modules from parent directory
SHARED_SRC = ../serial.c ../packet.c ../monada.c ../now.c \
             ../median_filter.c ../maf_filter.c ../error.c

# Object files
MQTT_OBJ = $(MQTT_SRC:.c=.o)
SHARED_OBJ = serial.o packet.o monada.o now.o median_filter.o maf_filter.o error.o

# All objects
OBJ = $(MQTT_OBJ) $(SHARED_OBJ)

# C compiler
CC = clang

# Compiler optimization
OPT = -O2

# Debug flags (use -g for debugging, remove OPT=-O2)
DBG =

# Compiler flags
CFLAGS = -Wall -Wextra -I..

# Libraries
LIBS = -lmosquitto

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSCONFDIR = /etc
SYSTEMDDIR = /etc/systemd/system

# Phony targets
.PHONY: all clean install uninstall

# Default target
all: $(PROGRAM)

# Link
$(PROGRAM): $(OBJ)
	$(CC) $(OBJ) $(DBG) $(LIBS) -o $(PROGRAM)

# Compile MQTT daemon sources
%.o: %.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

# Compile shared modules from parent directory
serial.o: ../serial.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

packet.o: ../packet.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

monada.o: ../monada.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

now.o: ../now.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

median_filter.o: ../median_filter.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

maf_filter.o: ../maf_filter.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

error.o: ../error.c
	$(CC) $(CFLAGS) $(OPT) $(DBG) -c $< -o $@

# Install
install: $(PROGRAM)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(PROGRAM) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(SYSCONFDIR)
	@if [ ! -f $(DESTDIR)$(SYSCONFDIR)/r4dcb08-mqtt.conf ]; then \
		install -m 644 ../r4dcb08-mqtt.conf.example $(DESTDIR)$(SYSCONFDIR)/r4dcb08-mqtt.conf; \
		echo "Installed default config to $(SYSCONFDIR)/r4dcb08-mqtt.conf"; \
	else \
		echo "Config file exists, not overwriting"; \
	fi
	install -d $(DESTDIR)$(SYSTEMDDIR)
	install -m 644 r4dcb08-mqtt.service $(DESTDIR)$(SYSTEMDDIR)/
	@echo ""
	@echo "Installation complete. To enable the service:"
	@echo "  sudo systemctl daemon-reload"
	@echo "  sudo systemctl enable r4dcb08-mqtt"
	@echo "  sudo systemctl start r4dcb08-mqtt"

# Uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROGRAM)
	rm -f $(DESTDIR)$(SYSTEMDDIR)/r4dcb08-mqtt.service
	@echo "Note: Config file $(SYSCONFDIR)/r4dcb08-mqtt.conf not removed"

# Clean
clean:
	rm -f *.o $(PROGRAM)

# Show help
help:
	@echo "r4dcb08-mqtt Makefile targets:"
	@echo "  all       - Build the daemon (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to system (requires root)"
	@echo "  uninstall - Remove from system (requires root)"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)"
	@echo "  DESTDIR=$(DESTDIR)"
