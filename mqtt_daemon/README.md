# r4dcb08-mqtt

MQTT daemon pro publikování teplot z R4DCB08 teplotních senzorů.

## Popis

`r4dcb08-mqtt` je daemon, který periodicky čte teploty z R4DCB08 teplotních senzorů přes Modbus RTU (RS485) a publikuje je na MQTT broker. Podporuje až 8 teplotních kanálů, filtrování hodnot a běh jako systemd služba.

## Funkce

- Čtení teplot z 1-8 kanálů R4DCB08 senzoru
- Publikování na MQTT broker s QoS 0/1/2 a retain
- Last Will and Testament (LWT) pro detekci offline stavu
- Automatické znovupřipojení s exponenciálním backoff
- Median filtr pro odstranění špiček
- MAF (Moving Average Filter) s lichoběžníkovými váhami
- Konfigurace přes CLI nebo INI soubor
- Běh jako daemon nebo v popředí
- Logování do syslog nebo stderr
- Systemd integrace

## Závislosti

```bash
# Debian/Ubuntu
sudo apt-get install libmosquitto-dev

# Arch Linux
sudo pacman -S mosquitto

# Fedora/RHEL
sudo dnf install mosquitto-devel
```

## Kompilace

```bash
cd mqtt_daemon
make
```

Pro debug verzi:
```bash
make clean
make DBG=-g OPT=-O0
```

## Instalace

```bash
sudo make install
```

Toto nainstaluje:
- `/usr/local/bin/r4dcb08-mqtt` - binární soubor
- `/etc/r4dcb08-mqtt.conf` - konfigurační soubor (pokud neexistuje)
- `/etc/systemd/system/r4dcb08-mqtt.service` - systemd unit

## Odinstalace

```bash
sudo make uninstall
```

## Použití

### Příkazová řádka

```bash
# Základní použití
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost

# S verbose výstupem
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -v

# Jako daemon
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -d

# S konfiguračním souborem
./r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf

# S filtry
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -m -M 5
```

### CLI parametry

| Parametr | Dlouhý tvar | Popis | Výchozí |
|----------|-------------|-------|---------|
| `-p` | `--port` | Sériový port | `/dev/ttyUSB0` |
| `-a` | `--address` | Modbus adresa (1-254) | `1` |
| `-b` | `--baudrate` | Přenosová rychlost | `9600` |
| `-n` | `--channels` | Počet kanálů (1-8) | `8` |
| `-H` | `--mqtt-host` | MQTT broker hostname | `localhost` |
| `-P` | `--mqtt-port` | MQTT broker port | `1883` |
| `-u` | `--mqtt-user` | MQTT uživatel | - |
| `-w` | `--mqtt-pass` | MQTT heslo | - |
| `-t` | `--topic` | Prefix MQTT topic | `sensors/r4dcb08` |
| `-i` | `--client-id` | MQTT client ID | auto |
| `-I` | `--interval` | Interval měření [s] | `10` |
| `-c` | `--config` | Konfigurační soubor | - |
| `-d` | `--daemon` | Běžet jako daemon | ne |
| `-v` | `--verbose` | Podrobný výstup | ne |
| `-m` | `--median-filter` | Povolit median filtr | ne |
| `-M` | `--maf-filter` | MAF filtr (velikost okna) | ne |
| `-h` | `--help` | Zobrazit nápovědu | - |
| `-V` | `--version` | Zobrazit verzi | - |

### Konfigurační soubor

Konfigurační soubor používá INI formát. Příklad (`/etc/r4dcb08-mqtt.conf`):

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
password =
topic = sensors/r4dcb08
qos = 1
retain = true
keepalive = 60

[daemon]
interval = 10
verbose = false

[filters]
median_filter = false
maf_window = 0
```

CLI parametry mají přednost před konfiguračním souborem.

## MQTT Topics

Daemon publikuje na následující topics:

```
{prefix}/{address}/temperature/ch1    Teplota kanálu 1 [°C]
{prefix}/{address}/temperature/ch2    Teplota kanálu 2 [°C]
...
{prefix}/{address}/temperature/ch8    Teplota kanálu 8 [°C]
{prefix}/{address}/timestamp          Čas měření (ISO 8601)
{prefix}/{address}/status             Stav: "online"/"offline"/"error"
```

Příklad s výchozím nastavením (adresa 1):
```
sensors/r4dcb08/1/temperature/ch1     "23.5"
sensors/r4dcb08/1/temperature/ch2     "24.1"
sensors/r4dcb08/1/timestamp           "2026-01-29 17:54:32.45"
sensors/r4dcb08/1/status              "online"
```

### Last Will and Testament (LWT)

Při neočekávaném odpojení daemon automaticky publikuje:
```
sensors/r4dcb08/1/status              "offline"
```

### Hodnoty

- Teploty jsou publikovány jako text s jedním desetinným místem (např. `"23.5"`)
- Neplatné hodnoty jsou publikovány jako `"NaN"`
- Všechny zprávy jsou s `retain=true` (lze změnit v konfiguraci)
- Výchozí QoS je 1

## Systemd

### Spuštění služby

```bash
sudo systemctl daemon-reload
sudo systemctl enable r4dcb08-mqtt
sudo systemctl start r4dcb08-mqtt
```

### Stav služby

```bash
sudo systemctl status r4dcb08-mqtt
```

### Logy

```bash
sudo journalctl -u r4dcb08-mqtt -f
```

### Restart

```bash
sudo systemctl restart r4dcb08-mqtt
```

### Zastavení

```bash
sudo systemctl stop r4dcb08-mqtt
```

## Filtry

### Median filtr (`-m`)

Tříbodový median filtr pro odstranění náhodných špiček. Filtr udržuje poslední 3 hodnoty a vrací prostřední hodnotu.

```bash
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -m
```

### MAF filtr (`-M <size>`)

Moving Average Filter s lichoběžníkovými váhami. Velikost okna musí být liché číslo 3-15.

```bash
# MAF s oknem 5
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -M 5

# Kombinace obou filtrů
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -m -M 7
```

Váhy: `[0.5, 1, 1, ..., 1, 0.5]`

## Testování

### Manuální test

Terminal 1 - MQTT subscriber:
```bash
mosquitto_sub -h localhost -t "sensors/r4dcb08/#" -v
```

Terminal 2 - Daemon:
```bash
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -I 5 -v
```

### Test reconnect

```bash
# Spustit daemon
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -v &

# Restartovat broker
sudo systemctl restart mosquitto

# Daemon by se měl automaticky znovu připojit
```

### Test LWT

```bash
# Subscriber
mosquitto_sub -h localhost -t "sensors/r4dcb08/1/status" -v

# Spustit daemon a pak ho násilně ukončit
./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost &
kill -9 $!

# Mělo by se objevit "offline"
```

## Signály

| Signál | Akce |
|--------|------|
| `SIGTERM` | Graceful shutdown |
| `SIGINT` | Graceful shutdown (Ctrl+C) |
| `SIGHUP` | (rezervováno pro reload konfigurace) |
| `SIGPIPE` | Ignorován |

## Chybové stavy

Daemon se pokouší o automatické zotavení při:
- Ztrátě MQTT spojení (exponenciální backoff 1-60s)
- Chybě čtení ze sériového portu (reopen)

Po 10 po sobě jdoucích chybách daemon ukončí činnost.

## Architektura

```
┌─────────────────────────────────────────────────────┐
│              r4dcb08-mqtt daemon                     │
├─────────────────────────────────────────────────────┤
│  mqtt_main.c      │  mqtt_client.c (libmosquitto)   │
│  mqtt_config.c    │  mqtt_publish.c                 │
│  mqtt_error.c     │                                 │
├─────────────────────────────────────────────────────┤
│  Znovupoužité moduly z r4dcb08:                     │
│  serial.c  packet.c  monada.c  now.c                │
│  median_filter.c  maf_filter.c  error.c             │
└─────────────────────────────────────────────────────┘
```

### Soubory

| Soubor | Popis |
|--------|-------|
| `mqtt_main.c` | Hlavní smyčka, daemonizace, signály |
| `mqtt_config.c/h` | Parsování CLI a INI konfigurace |
| `mqtt_error.c/h` | Logování (syslog/stderr), chybové kódy |
| `mqtt_client.c/h` | Wrapper pro libmosquitto |
| `mqtt_publish.c/h` | Čtení teplot a publikování |

## Omezení

- Single-threaded (kvůli statickým bufferům v `monada.c`)
- Libmosquitto používá vlastní síťový thread (`mosquitto_loop_start`)
- Maximálně 8 teplotních kanálů
- Modbus adresa 1-254

## Licence

Stejná licence jako hlavní projekt r4dcb08.

## Autor

Generováno pomocí Claude Code.
