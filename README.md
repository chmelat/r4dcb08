# R4DCB08 Temperature Sensor Utility

**V1.9 (2026-01-21)**

A command-line utility for communicating with R4DCB08 temperature sensor modules via serial port.

## Overview

This utility allows you to:
- Read temperature data from up to 8 channels in real-time
- Configure device settings (address, baudrate)
- Read and set temperature correction values
- Monitor temperature with configurable intervals

## Installation

### Prerequisites

- C compiler (GCC/Clang)
- Make utility
- Serial port support (Linux/Unix-based systems)

### Compilation

```bash
make
```

## Quick Start

1. Connect the R4DCB08 module to your computer via USB-RS485 adapter
2. Run the utility:

```bash
./r4dcb08
```

Example output (reading 4 channels every 2 seconds):
```
$ ./r4dcb08 -n 4 -t 2
# Date                  Ch1  Ch2  Ch3  Ch4
2026-01-21 14:30:15.42  22.5 23.1 21.8 22.0
2026-01-21 14:30:17.42  22.6 23.0 21.9 22.1
2026-01-21 14:30:19.42  22.5 23.1 21.8 22.0
^C
Received SIGINT (Ctrl+C), measurement stopped
```

Output format: `timestamp Ch1 Ch2 Ch3 ...` - channels are printed in order from 1 to n, separated by spaces.

Press **Ctrl+C** to stop continuous measurements.

## Usage

### Basic Examples

1. **Read temperature from channel 1** (default):
```bash
./r4dcb08
```

2. **Read 4 channels every 2 seconds:**
```bash
./r4dcb08 -n 4 -t 2
```

3. **Single measurement without timestamp** (useful for scripts):
```bash
./r4dcb08 -n 4 -f
# Output: 22.5 23.1 21.8 22.0
```

4. **Use different serial port:**
```bash
./r4dcb08 -p /dev/ttyUSB1
```

5. **Enable median filter** (smooths out noise/spikes):
```bash
./r4dcb08 -m
```

### Device Configuration

6. **Read current temperature correction values:**
```bash
./r4dcb08 -c
```

7. **Set temperature correction for channel 3 to +1.5°C:**
```bash
./r4dcb08 -s 3,1.5
```
Temperature correction is useful when sensor readings need calibration (e.g., sensor shows 21.0°C but actual temperature is 22.5°C → set correction to +1.5).

8. **Change device address from 1 to 5:**
```bash
./r4dcb08 -a 1 -w 5
```
Note: `-a 1` specifies current address, `-w 5` sets new address.

9. **Change device baudrate to 19200:**
```bash
./r4dcb08 -x 4
```
Baudrate codes: 0=1200, 1=2400, 2=4800, 3=9600, 4=19200. Change takes effect after device power cycle.

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p [port]` | Serial port device | `/dev/ttyUSB0` |
| `-a [1-254]` | Device address (for multi-device setups) | 1 |
| `-b [baud]` | Serial port baudrate {1200, 2400, 4800, 9600, 19200} | 9600 |
| `-t [seconds]` | Interval between measurements | 1 |
| `-n [1-8]` | Number of channels to read | 1 |
| `-c` | Read temperature correction values | - |
| `-w [1-254]` | Write new device address | - |
| `-x [0-4]` | Write device baudrate (0=1200, 1=2400, 2=4800, 3=9600, 4=19200) | - |
| `-s [ch,value]` | Set temperature correction for channel (e.g., `-s 3,1.5`) | - |
| `-m` | Enable three-point median filter (reduces noise) | Off |
| `-f` | One-shot measurement without timestamp | Off |
| `-h` or `-?` | Display help | - |

### Understanding `-b` vs `-x`

- **`-b`** sets baudrate for **this session** (how fast your computer talks to the device)
- **`-x`** changes baudrate **stored in the device** (permanent, survives power cycle)

Normally you only need `-b` if default 9600 doesn't work. Use `-x` only when you want to permanently change device speed.

## Notes

- Temperature readings are in degrees Celsius
- Valid range: -55.0°C to 125.0°C (values outside show as `NaN`)
- Press Ctrl+C to stop continuous measurements
- Multiple devices can share one RS485 bus using different addresses

## Changelog

### V1.9 (2026-01-21)
- Replaced deprecated signal() with POSIX sigaction()
- Removed unsafe printf() from signal handler
- Added NULL check after now() call
- Replaced usleep() with POSIX nanosleep()
- Fixed type punning using portable bitwise operations

### V1.8 (2025-08-29)
- Added exclusive file locking (flock) for serial port
- Improved reliability in multi-process environments

### V1.7 (2025-06-06)
- Unified error handling system
- Centralized constants
- Fixed resource leak in port initialization

### V1.6 (2025-06-06)
- Fixed baud rate table bug (1200 baud)
- Code cleanup and optimization

### V1.5 (2025-04-17)
- Implemented AppStatus error codes
- Modularized configuration
- Improved signal handling

### V1.4 (2025-04-14/16)
- Added median filter
- Enhanced input validation

---

## Technical Reference

This section is for developers and advanced users.

### Protocol

Modbus RTU over serial port:
- Function 0x03: Read holding registers
- Function 0x06: Write single register

### Register Map

| Register | Description |
|----------|-------------|
| 0x0000-0x0007 | Temperature readings (channels 1-8) |
| 0x0008-0x000F | Temperature correction values |
| 0x00FE | Device address |
| 0x00FF | Baudrate setting |

### Error Codes

- 0: Success
- -10 to -19: Argument errors
- -20 to -29: Communication errors
- -30 to -39: Operation errors
