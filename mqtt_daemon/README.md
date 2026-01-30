# r4dcb08-mqtt

MQTT daemon for publishing temperatures from R4DCB08 temperature sensors.

## Quick Start

```bash
# Build
cd mqtt_daemon
make

# Run (reads 8 channels, publishes to localhost every 10s)
./r4dcb08-mqtt -p /dev/ttyUSB0 -H localhost -v

# Subscribe to see values
mosquitto_sub -h localhost -t "sensors/r4dcb08/#" -v
```

## Description

`r4dcb08-mqtt` reads temperatures from R4DCB08 sensors via Modbus RTU (RS485) and publishes them to an MQTT broker. Supports up to 8 channels, filtering, TLS, and runs as a systemd service or standalone daemon.

## Hardware Requirements

- **R4DCB08 sensor module** - 8-channel temperature sensor with RS485 interface
- **RS485-USB converter** - e.g. CH340-based or FTDI-based adapter
- Wiring: Connect A+/B- from sensor to A+/B- on converter (match polarity)

Typical USB device: `/dev/ttyUSB0` (Linux), `/dev/cuaU0` (FreeBSD)

## Features

- Read temperatures from 1-8 channels
- Publish to MQTT with QoS 0/1/2 and retain
- TLS/SSL encryption support
- Last Will and Testament (LWT) - broker publishes "offline" when daemon dies unexpectedly
- Auto reconnect with exponential backoff (1-60s)
- Median filter (3-point) for spike removal
- MAF filter (moving average, 3-15 samples)
- Config via CLI or INI file
- Optional systemd integration (notify, watchdog)

## Dependencies

```bash
# Debian/Ubuntu
sudo apt-get install libmosquitto-dev libsystemd-dev

# Fedora/RHEL
sudo dnf install mosquitto-devel systemd-devel

# Arch Linux
sudo pacman -S mosquitto libsystemd

# Alpine Linux (no systemd)
apk add mosquitto-dev

# FreeBSD
pkg install mosquitto
```

`libsystemd-dev` is only needed for default build. Use `make NO_SYSTEMD=1` to build without it.

## Build

```bash
cd mqtt_daemon
make                  # with systemd support (default)
make NO_SYSTEMD=1     # without systemd (Alpine, FreeBSD, etc.)
make DBG=-g OPT=-O0   # debug build
```

## Install

```bash
sudo make install
```

Installs binary to `/usr/local/bin/`, config to `/etc/`, systemd unit to `/etc/systemd/system/`.

## CLI Options

### Serial

| Option | Long | Description | Default |
|--------|------|-------------|---------|
| `-p` | `--port` | Serial port | `/dev/ttyUSB0` |
| `-a` | `--address` | Modbus address (1-254) | `1` |
| `-b` | `--baudrate` | Baud rate | `9600` |
| `-n` | `--channels` | Number of channels (1-8) | `8` |

### MQTT

| Option | Long | Description | Default |
|--------|------|-------------|---------|
| `-H` | `--mqtt-host` | Broker hostname | `localhost` |
| `-P` | `--mqtt-port` | Broker port | `1883` (TLS: `8883`) |
| `-u` | `--mqtt-user` | Username | - |
| `-W` | `--password-file` | File containing password | - |
| `-t` | `--topic` | Topic prefix | `sensors/r4dcb08` |
| `-i` | `--client-id` | Client ID | auto-generated |

Password can also be set via `MQTT_PASSWORD` environment variable.

### TLS

| Option | Long | Description |
|--------|------|-------------|
| `-S` | `--tls` | Enable TLS |
| | `--tls-ca` | CA certificate file |
| | `--tls-cert` | Client certificate |
| | `--tls-key` | Client private key |
| | `--tls-insecure` | Skip cert verification (testing only) |

### Daemon

| Option | Long | Description | Default |
|--------|------|-------------|---------|
| `-I` | `--interval` | Measurement interval [s] | `10` |
| `-c` | `--config` | Config file path | - |
| `-F` | `--pid-file` | PID file path | `/var/run/r4dcb08-mqtt.pid` |
| `-d` | `--daemon` | Run as background daemon | no |
| `-v` | `--verbose` | Verbose output | no |

### Filters

| Option | Long | Description | Default |
|--------|------|-------------|---------|
| `-m` | `--median-filter` | Enable 3-point median filter | off |
| `-M` | `--maf-filter` | Enable MAF with window size (odd, 3-15) | off |

### Diagnostics

| Option | Long | Description | Default |
|--------|------|-------------|---------|
| `-D` | `--diagnostics-interval` | Publish every N intervals (0=disable) | `6` |

## Config File

INI format, CLI options override config values.

```ini
[serial]
port = /dev/ttyUSB0
address = 1
baudrate = 9600
channels = 8

[mqtt]
host = localhost
mqtt_port = 1883
user =
password_file =
topic = sensors/r4dcb08
qos = 1
retain = true
keepalive = 60

[tls]
enabled = false
ca_file =
cert_file =
key_file =
insecure = false

[daemon]
interval = 10
verbose = false

[filters]
median_filter = false
maf_filter = false
maf_window = 5

[diagnostics]
diagnostics_interval = 6
```

## MQTT Topics

```
{prefix}/{address}/temperature/ch1    Channel 1 temperature [°C]
{prefix}/{address}/temperature/ch2    Channel 2 temperature [°C]
...
{prefix}/{address}/status             "online" / "offline"
{prefix}/{address}/timestamp          Measurement time
{prefix}/{address}/diagnostics        JSON metrics
```

Example output:
```
sensors/r4dcb08/1/temperature/ch1     "23.5"
sensors/r4dcb08/1/status              "online"
sensors/r4dcb08/1/timestamp           "2026-01-29 17:54:32.45"
```

### Values

- Temperatures: one decimal place as string (`"23.5"`)
- Invalid readings: `"NaN"`
- All messages: `retain=true` by default, QoS 1

### Last Will and Testament (LWT)

When daemon disconnects unexpectedly (crash, network failure), the MQTT broker automatically publishes `"offline"` to the status topic. This lets subscribers detect sensor failures even when the daemon can't send a goodbye message.

## Filters

### Median filter (`-m`)

Removes random spikes by keeping last 3 values and returning the middle one. Good for noisy sensors.

### MAF filter (`-M <size>`)

Moving Average Filter smooths readings over a window of 3-15 samples. Edge samples have half weight to reduce lag. Larger window = smoother but slower response.

```bash
# Median only - removes spikes
./r4dcb08-mqtt -H localhost -m

# MAF with 7-sample window - smooth output
./r4dcb08-mqtt -H localhost -M 7

# Both - spike removal + smoothing
./r4dcb08-mqtt -H localhost -m -M 7
```

## Systemd

Service uses notify protocol with watchdog:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now r4dcb08-mqtt
sudo systemctl status r4dcb08-mqtt
sudo journalctl -u r4dcb08-mqtt -f
```

Security features: runs as `nobody`, read-only filesystem, access only to serial ports.

## Running Without Systemd

Build with `make NO_SYSTEMD=1`, then use one of these:

### OpenRC (Alpine, Gentoo)

```bash
cat > /etc/init.d/r4dcb08-mqtt << 'EOF'
#!/sbin/openrc-run
name="r4dcb08-mqtt"
command="/usr/local/bin/r4dcb08-mqtt"
command_args="-c /etc/r4dcb08-mqtt.conf"
command_background=true
pidfile="/run/${RC_SVCNAME}.pid"
command_user="nobody:dialout"
depend() { need net; after mosquitto; }
EOF
chmod +x /etc/init.d/r4dcb08-mqtt
rc-update add r4dcb08-mqtt default
```

### runit (Void, Devuan)

```bash
mkdir -p /etc/sv/r4dcb08-mqtt
cat > /etc/sv/r4dcb08-mqtt/run << 'EOF'
#!/bin/sh
exec chpst -u nobody:dialout /usr/local/bin/r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf 2>&1
EOF
chmod +x /etc/sv/r4dcb08-mqtt/run
ln -s /etc/sv/r4dcb08-mqtt /var/service/
```

### supervisord

```ini
[program:r4dcb08-mqtt]
command=/usr/local/bin/r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf
user=nobody
group=dialout
autorestart=true
stderr_logfile=/var/log/r4dcb08-mqtt.err.log
```

### Manual

```bash
# Foreground
./r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf -v

# Background
nohup ./r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf > /var/log/r4dcb08-mqtt.log 2>&1 &
```

Note: Without systemd watchdog, use a supervisor with health checks for auto-restart on hang.

## Troubleshooting

### "Failed to open serial port"

- Check device exists: `ls -la /dev/ttyUSB*`
- Check permissions: user must be in `dialout` group (Linux) or `dialer` (FreeBSD)
  ```bash
  sudo usermod -a -G dialout $USER   # then logout/login
  ```
- Check if device is in use: `fuser /dev/ttyUSB0`

### No temperature readings

1. Test sensor with main r4dcb08 tool first:
   ```bash
   ../r4dcb08 -p /dev/ttyUSB0 -a 1
   ```
2. Check Modbus address matches sensor DIP switches (default: 1)
3. Check wiring polarity (A+/B-)
4. Try lower baudrate: `-b 4800`

### MQTT connection fails

- Test broker connectivity: `mosquitto_pub -h localhost -t test -m hello`
- Check firewall: port 1883 (or 8883 for TLS)
- For TLS issues, try `--tls-insecure` first to isolate cert problems

### "NaN" values

- Sensor not connected to that channel
- Sensor cable too long or damaged
- Try enabling median filter (`-m`) to filter out occasional bad reads

### Daemon exits after 10 errors

Serial or MQTT problems. Check logs:
```bash
journalctl -u r4dcb08-mqtt -n 50   # systemd
tail -50 /var/log/r4dcb08-mqtt.log # manual
```

## Diagnostics

Publishes JSON to `{prefix}/{address}/diagnostics` every N intervals:

```json
{
  "uptime": 3600,
  "reads": {"total": 360, "success": 358, "failure": 2},
  "mqtt_reconnects": 1,
  "consecutive_errors": 0
}
```

## Signals

| Signal | Action |
|--------|--------|
| SIGTERM, SIGINT | Graceful shutdown (responds within 1s) |
| SIGHUP | Reserved for config reload (not implemented) |
| SIGPIPE | Ignored |

## Error Recovery

- MQTT disconnect: auto reconnect with backoff 1-60s
- Serial read error: reopens port
- 10 consecutive errors: daemon exits (let supervisor restart it)

## Limitations

- Max 8 temperature channels
- Modbus address 1-254
- Single sensor per daemon instance (run multiple daemons for multiple sensors)

## License

Same as main r4dcb08 project.
