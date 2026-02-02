#
# Makefile for r4dcb08-mqtt daemon
#

PROGRAM = r4dcb08-mqtt

# Source files
MQTT_SRC = mqtt_main.c mqtt_config.c mqtt_error.c mqtt_client.c mqtt_publish.c mqtt_metrics.c

# Object files
MQTT_OBJ = $(MQTT_SRC:.c=.o)
SHARED_OBJ = serial.o packet.o monada.o now.o median_filter.o maf_filter.o error.o
OBJ = $(MQTT_OBJ) $(SHARED_OBJ)

# Compiler
CC ?= clang
CFLAGS = -Wall -Wextra -I..
OPT = -O2
LIBS = -lmosquitto

# Systemd support (use: make NO_SYSTEMD=1 to disable)
ifndef NO_SYSTEMD
CFLAGS += -DUSE_SYSTEMD
LIBS += -lsystemd
endif

# Installation paths
BINDIR = /usr/local/bin
CONFDIR = /etc
SYSTEMDDIR = /usr/local/lib/systemd/system

.PHONY: all clean install uninstall help

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) $(OBJ) $(LIBS) -o $(PROGRAM)

# MQTT daemon sources
mqtt_main.o: mqtt_main.c mqtt_revision.h
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

mqtt_config.o: mqtt_config.c mqtt_revision.h
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

mqtt_error.o: mqtt_error.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

mqtt_client.o: mqtt_client.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

mqtt_publish.o: mqtt_publish.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

mqtt_metrics.o: mqtt_metrics.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

# Shared modules from parent directory
serial.o: ../serial.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

packet.o: ../packet.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

monada.o: ../monada.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

now.o: ../now.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

median_filter.o: ../median_filter.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

maf_filter.o: ../maf_filter.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

error.o: ../error.c
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

# Install (requires root)
install: $(PROGRAM)
	install -d $(BINDIR)
	install -m 755 $(PROGRAM) $(BINDIR)/
	@if [ ! -f $(CONFDIR)/r4dcb08-mqtt.conf ]; then \
		install -m 640 r4dcb08-mqtt.conf.example $(CONFDIR)/r4dcb08-mqtt.conf; \
		echo "Installed config to $(CONFDIR)/r4dcb08-mqtt.conf"; \
	else \
		echo "Config exists, not overwriting"; \
	fi
	install -d $(SYSTEMDDIR)
	install -m 644 r4dcb08-mqtt.service $(SYSTEMDDIR)/
	systemctl daemon-reload
	@echo ""
	@echo "Done. To start the service:"
	@echo "  1. Edit config: sudo nano $(CONFDIR)/r4dcb08-mqtt.conf"
	@echo "  2. Enable service: sudo systemctl enable --now r4dcb08-mqtt"

# Uninstall (requires root)
uninstall:
	@if systemctl is-active --quiet r4dcb08-mqtt 2>/dev/null; then \
		systemctl stop r4dcb08-mqtt; \
	fi
	@if systemctl is-enabled --quiet r4dcb08-mqtt 2>/dev/null; then \
		systemctl disable r4dcb08-mqtt; \
	fi
	rm -f $(SYSTEMDDIR)/r4dcb08-mqtt.service
	rm -f $(BINDIR)/$(PROGRAM)
	systemctl daemon-reload
	@echo "Uninstalled. Config $(CONFDIR)/r4dcb08-mqtt.conf kept."

clean:
	rm -f *.o $(PROGRAM)

help:
	@echo "Targets:"
	@echo "  make          - Build daemon"
	@echo "  make install  - Install (requires root)"
	@echo "  make uninstall- Uninstall (requires root)"
	@echo "  make clean    - Remove build files"
