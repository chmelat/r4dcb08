# Plán implementace MQTT daemona pro R4DCB08

## Přehled

Vytvoření samostatného daemona `r4dcb08-mqtt`, který čte teploty z R4DCB08 senzorů přes Modbus RTU a publikuje je na MQTT broker pomocí knihovny **libmosquitto**.

## Architektura

```
┌─────────────────────────────────────────────────────┐
│              r4dcb08-mqtt daemon                     │
├─────────────────────────────────────────────────────┤
│  mqtt_main.c      │  mqtt_client.c (libmosquitto)   │
│  mqtt_config.c    │  mqtt_publish.c                 │
├─────────────────────────────────────────────────────┤
│  Znovupoužité moduly z r4dcb08:                     │
│  serial.c  packet.c  monada.c  now.c  filters       │
└─────────────────────────────────────────────────────┘
```

## Struktura souborů

```
r4dcb08/
├── mqtt_daemon/
│   ├── mqtt_main.c           # Hlavní vstupní bod
│   ├── mqtt_client.c/h       # Wrapper pro libmosquitto
│   ├── mqtt_config.c/h       # Parsování konfigurace
│   ├── mqtt_publish.c/h      # Logika publikování
│   ├── mqtt_error.c/h        # MQTT error handling
│   ├── Makefile              # Build pro daemon
│   └── r4dcb08-mqtt.service  # Systemd service
└── r4dcb08-mqtt.conf.example # Ukázková konfigurace
```

## MQTT topic struktura

```
sensors/r4dcb08/{address}/temperature/ch1   "23.5"  (QoS 1, retain)
sensors/r4dcb08/{address}/temperature/ch2   "24.1"
sensors/r4dcb08/{address}/status            "online"/"offline"/"error"
sensors/r4dcb08/{address}/timestamp         "2026-01-29 10:15:32.45"
```

## Klíčové soubory k úpravě/vytvoření

### Nové soubory (mqtt_daemon/):
1. `mqtt_main.c` - Main loop, CLI parsing, daemon logika
2. `mqtt_client.c/h` - libmosquitto wrapper, connect/reconnect/publish
3. `mqtt_config.c/h` - INI parser, CLI options
4. `mqtt_publish.c/h` - Formátování a publikování teplot
5. `mqtt_error.c/h` - Error kódy a logging (syslog)
6. `Makefile` - Build s -lmosquitto
7. `r4dcb08-mqtt.service` - Systemd unit

### Znovupoužité moduly:
- `serial.c/h` - Otevření RS485 portu
- `packet.c/h` - Modbus RTU pakety
- `monada.c/h` - High-level komunikace
- `now.c/h` - Timestamp
- `median_filter.c/h`, `maf_filter.c/h` - Filtry
- `signal_handler.c/h` - SIGINT/SIGTERM

## Implementační kroky

### 1. Základní infrastruktura
- [x] Vytvořit `mqtt_daemon/` adresář
- [x] `mqtt_config.c/h` - konfigurace a CLI parsing
- [x] `mqtt_error.c/h` - error handling + syslog
- [x] Základní `Makefile`

### 2. MQTT klient
- [x] `mqtt_client.c/h` - wrapper pro libmosquitto
- [x] Connect s LWT (Last Will Testament)
- [x] Reconnect s exponential backoff
- [x] Publish funkce

### 3. Integrace teplotního čtení
- [x] Funkce pro čtení teplot (inspirace: `read_functions.c`)
- [x] Podpora median a MAF filtrů
- [x] `mqtt_publish.c/h` - formátování payloadu

### 4. Main daemon
- [x] `mqtt_main.c` - hlavní smyčka
- [x] CLI options (getopt_long)
- [x] INI config file support
- [x] Graceful shutdown

### 5. Systémová integrace
- [x] `r4dcb08-mqtt.service` - systemd
- [x] `r4dcb08-mqtt.conf.example`
- [x] Makefile install/uninstall targets

## Klíčové CLI options

```
-p, --port <device>      Sériový port (default: /dev/ttyUSB0)
-a, --address <addr>     Modbus adresa 1-254 (default: 1)
-n, --channels <num>     Počet kanálů 1-8 (default: 8)
-H, --mqtt-host <host>   MQTT broker (default: localhost)
-P, --mqtt-port <port>   MQTT port (default: 1883)
-u, --mqtt-user <user>   MQTT username
-w, --mqtt-pass <pass>   MQTT password
-t, --topic <prefix>     Topic prefix (default: sensors/r4dcb08)
-I, --interval <sec>     Interval měření (default: 10)
-c, --config <file>      Konfigurační soubor
-d, --daemon             Běžet jako daemon
-m, --median-filter      Povolit median filtr
-M, --maf-filter <size>  Povolit MAF filtr
```

## Závislosti

```bash
# Instalace na Debian/Ubuntu:
sudo apt-get install libmosquitto-dev
```

## Verifikace

1. **Build test:**
   ```bash
   cd mqtt_daemon && make
   ```

2. **Manuální test:**
   ```bash
   # Terminal 1 - subscriber
   mosquitto_sub -h localhost -t "sensors/r4dcb08/#" -v

   # Terminal 2 - daemon
   ./r4dcb08-mqtt -p /dev/ttyUSB0 -a 1 -H localhost -I 5
   ```

3. **Systemd test:**
   ```bash
   sudo make install
   sudo systemctl start r4dcb08-mqtt
   sudo journalctl -u r4dcb08-mqtt -f
   ```

4. **Reconnect test:**
   ```bash
   # Restart broker a ověřit, že daemon se znovu připojí
   sudo systemctl restart mosquitto
   ```

## Poznámky

- `monada.c` používá statické buffery - daemon musí být single-threaded
- libmosquitto má vlastní network thread (mosquitto_loop_start)
- LWT zajistí publikování "offline" při neočekávaném odpojení
- QoS 1 + retain pro spolehlivé doručení a perzistenci poslední hodnoty
