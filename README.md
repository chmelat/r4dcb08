# R4DCB08 Temperature Sensor Utility

A command-line utility for communicating with R4DCB08 temperature sensor modules via serial port.

## Overview

This utility allows you to:
- Read temperature data from up to 8 channels in real-time
- Configure device settings (address, baudrate)
- Read and set temperature correction values
- Monitor temperature with configurable intervals

## Installation

### Prerequisites

- GCC compiler
- Make utility
- Serial port support (Linux/Unix-based systems)

### Compilation

```bash
make
```

## Usage

The utility provides several operation modes based on the command-line options used.

### Basic Examples

1. Read temperature from the default channel:
```bash
./r4dcb08
```

2. Read temperature from multiple channels with 2 second intervals:
```bash
./r4dcb08 -n 4 -t 2
```

3. Read temperature correction values:
```bash
./r4dcb08 -c
```

4. Set temperature correction for channel 3 to 1.5°C:
```bash
./r4dcb08 -s 3,1.5
```

5. Change device address from 1 to 5:
```bash
./r4dcb08 -a 1 -w 5
```

6. Set device baudrate to 19200:
```bash
./r4dcb08 -a 1 -x 4
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p [name]` | Select port | `/dev/ttyUSB0` |
| `-a [address]` | Select device address | `01H` |
| `-b [n]` | Set baudrate on serial port {1200, 2400, 4800, 9600, 19200} | 9600 |
| `-t [time]` | Time step [s] | 1s |
| `-n [num]` | Number of channels to read (1-8) | 1 |
| `-c` | Read correction temperature [°C] | - |
| `-w [address]` | Write new device address (1-254) | - |
| `-x [n]` | Set baudrate on R4DCB08 device {0:1200, 1:2400, 2:4800, 3:9600, 4:19200} | - |
| `-s [ch,Tc]` | Set temperature correction Tc for channel ch | - |
| `-h` or `-?` | Display help | - |

## Notes

- Temperature readings are in degrees Celsius
- Temperature range: -55.0°C to 125.0°C (values outside this range will display as NaN)
- When reading temperatures continuously, press Ctrl+C to stop
- Device address changes and baudrate settings take effect immediately or after reboot (depending on device model)

## Technical Details

### Protocol

The utility communicates with R4DCB08 modules using Modbus RTU protocol over serial port:
- Function code 0x03 for reading data
- Function code 0x06 for writing registers

### Register Map

| Register | Description |
|----------|-------------|
| 0x0000-0x0007 | Temperature readings (channels 1-8) |
| 0x0008-0x000F | Temperature correction values (channels 1-8) |
| 0x00FE | Device address |
| 0x00FF | Baudrate setting |

## Exit Codes

- 0: Successful operation
- 1: Error (with description printed to stderr)

## Version

V1.4 (2025-04-14)
