# Srovnání MQTT implementací: r4dcb08-mqtt vs tempcol

**Datum:** 2026-01-31
**Verze:** 1.0
**Autor:** Analýza kódu

---

## 1. Přehled

Tento dokument srovnává dvě MQTT implementace pro teplotní senzor R4DCB08:

| Projekt | Jazyk | MQTT knihovna | Binárka |
|---------|-------|---------------|---------|
| **r4dcb08-mqtt** | C | libmosquitto | `r4dcb08-mqtt` |
| **tempcol** | Rust | paho-mqtt | `tempcol daemon` |

---

## 2. Architektura

### 2.1 r4dcb08-mqtt (C)

```
mqtt_daemon/
├── mqtt_main.c         # Daemon hlavní smyčka
├── mqtt_client.c/h     # libmosquitto wrapper
├── mqtt_publish.c/h    # Publikace teplot
├── mqtt_config.c/h     # INI parser + CLI argumenty
├── mqtt_metrics.c/h    # Diagnostika a metriky
├── mqtt_error.c/h      # Error handling
├── Makefile
├── r4dcb08-mqtt.conf.example
└── r4dcb08-mqtt.service
```

**Charakteristika:**
- Samostatná binárka (~150 KB)
- Modulární design s oddělenými vrstvami
- Využívá moduly z hlavního projektu (serial, packet, monada, filtry)
- Thread-safe s atomic operacemi

### 2.2 tempcol (Rust)

```
src/
├── main.rs             # CLI aplikace
├── mqtt.rs             # MQTT integrace
├── tokio_sync.rs       # Sync Modbus operace
└── ...
```

**Charakteristika:**
- Integrováno v hlavní binárce (~5-10 MB)
- Async/await s Tokio runtime
- YAML konfigurace

---

## 3. MQTT knihovny

### 3.1 libmosquitto (C)

```c
#include <mosquitto.h>

struct mosquitto *mosq = mosquitto_new(client_id, true, userdata);
mosquitto_connect(mosq, host, port, keepalive);
mosquitto_publish(mosq, NULL, topic, len, payload, qos, retain);
mosquitto_loop_start(mosq);  // Background thread
```

**Vlastnosti:**
- Eclipse Mosquitto projekt
- Nativní C knihovna
- Threaded nebo poll-based loop
- Callbacks pro události
- Široká podpora platforem

### 3.2 paho-mqtt (Rust)

```rust
use paho_mqtt::{Client, ConnectOptionsBuilder};

let client = Client::new(create_opts)?;
client.connect(conn_opts)?;
client.publish(Message::new_retained(topic, payload, qos))?;
```

**Vlastnosti:**
- Eclipse Paho projekt
- Rust wrapper nad C knihovnou
- Synchronní i asynchronní API
- Builder pattern pro konfiguraci

---

## 4. Srovnání funkcí

### 4.1 Přehledová tabulka

| Funkce | r4dcb08-mqtt | tempcol | Poznámka |
|--------|:------------:|:-------:|----------|
| **Připojení** |
| TCP | ✓ | ✓ | |
| TLS/SSL | ✓ | ✗ | A: CA, cert, key |
| TLS insecure mode | ✓ | ✗ | Pro testování |
| Autentizace | ✓ | ✓ | User/password |
| Password file | ✓ | ✗ | Bezpečnější než CLI |
| Env variable | ✓ | ✗ | MQTT_PASSWORD |
| **Spolehlivost** |
| Auto reconnect | ✓ | ✓ | |
| Exponential backoff | ✓ | ✓ | A: 1-60s, B: 1-30s |
| Last Will (LWT) | ✓ | ✗ | Offline status při výpadku |
| **QoS** |
| QoS 0 | ✓ | ✓ | At most once |
| QoS 1 | ✓ | ✓ | At least once |
| QoS 2 | ✓ | ✓ | Exactly once |
| Retain | ✓ | ✓ | |
| **Konfigurace** |
| CLI argumenty | ✓ | ✗ | Kompletní CLI |
| Config file | ✓ (INI) | ✓ (YAML) | |
| Keepalive | ✓ | ✓ | |
| Client ID | ✓ | ✓ | Auto-generate |
| **Daemon** |
| Background mode | ✓ | ✓ | |
| PID file | ✓ | ✗ | |
| Systemd notify | ✓ | ✗ | sd_notify() |
| Systemd watchdog | ✓ | ✗ | |
| **Data processing** |
| Median filter | ✓ | ✗ | 3-point |
| MAF filter | ✓ | ✗ | 3-15 samples |
| **Monitoring** |
| Diagnostics | ✓ | ✗ | JSON metriky |
| Uptime tracking | ✓ | ✗ | |
| Error counters | ✓ | ✗ | |

### 4.2 Skóre

| Kategorie | r4dcb08-mqtt | tempcol |
|-----------|:------------:|:-------:|
| Bezpečnost | 5/5 | 2/5 |
| Spolehlivost | 5/5 | 3/5 |
| Konfigurovatelnost | 5/5 | 3/5 |
| Monitoring | 5/5 | 1/5 |
| Data processing | 5/5 | 1/5 |
| **Celkem** | **25/25** | **10/25** |

---

## 5. Detailní srovnání

### 5.1 TLS/SSL podpora

**r4dcb08-mqtt:**
```ini
[tls]
tls = true
tls_ca = /etc/ssl/certs/ca.crt
tls_cert = /etc/ssl/certs/client.crt
tls_key = /etc/ssl/private/client.key
tls_insecure = false
```

```c
mosquitto_tls_set(mosq, ca_file, NULL, cert_file, key_file, NULL);
mosquitto_tls_insecure_set(mosq, insecure);
```

**tempcol:**
- Nepodporuje TLS
- Pouze nešifrované TCP spojení

### 5.2 Last Will and Testament (LWT)

**r4dcb08-mqtt:**
```c
// Při připojení nastaví LWT
mosquitto_will_set(mosq, "sensors/r4dcb08/1/status", 7, "offline", qos, retain);

// Při graceful shutdown publikuje "online"
mqtt_publish_status(client, "online");

// Při neočekávaném výpadku broker automaticky publikuje "offline"
```

**tempcol:**
- Nepodporuje LWT
- Žádná automatická indikace výpadku

### 5.3 Konfigurace

**r4dcb08-mqtt (INI + CLI):**
```bash
# CLI
./r4dcb08-mqtt -H broker.local -P 8883 -S --tls-ca /etc/ssl/ca.crt \
               -u user -W /etc/mqtt-pass -t sensors/temp -I 30 -m -M 5

# Nebo config file
./r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf
```

```ini
[serial]
port = /dev/ttyUSB0
address = 1
baudrate = 9600
channels = 8

[mqtt]
host = broker.local
mqtt_port = 8883
user = sensor1
topic = sensors/r4dcb08
qos = 1
retain = true

[tls]
tls = true
tls_ca = /etc/ssl/certs/ca.crt

[filters]
median_filter = true
maf_filter = true
maf_window = 5

[diagnostics]
diagnostics_interval = 6
```

**tempcol (YAML):**
```yaml
uri: "tcp://localhost:1883"
username: user
password: pass
topic: r4dcb08
qos: 0
client_id: "r4dcb08-abc123"
```

### 5.4 Reconnect strategie

**r4dcb08-mqtt:**
```c
#define MQTT_RECONNECT_MIN_DELAY 1      // 1 sekunda
#define MQTT_RECONNECT_MAX_DELAY 60     // 60 sekund
#define MQTT_RECONNECT_MULTIPLIER 2     // Exponential backoff

// Implementace
client->reconnect_delay *= MQTT_RECONNECT_MULTIPLIER;
if (client->reconnect_delay > MQTT_RECONNECT_MAX_DELAY) {
    client->reconnect_delay = MQTT_RECONNECT_MAX_DELAY;
}
```

**tempcol:**
```rust
.automatic_reconnect(
    Duration::from_secs(1),   // min
    Duration::from_secs(30),  // max
);
```

### 5.5 Thread safety

**r4dcb08-mqtt:**
```c
// Atomic operace pro connected flag
static inline void set_connected(MqttClient *client, int value)
{
    __atomic_store_n(&client->connected, value, __ATOMIC_SEQ_CST);
}

static inline int get_connected(const MqttClient *client)
{
    return __atomic_load_n(&client->connected, __ATOMIC_SEQ_CST);
}
```

**tempcol:**
- Rust ownership model zajišťuje thread safety
- `SafeClient` wrapper pro sdílený přístup

### 5.6 Diagnostika

**r4dcb08-mqtt:**
```json
{
  "uptime": 3600,
  "reads": {
    "total": 360,
    "success": 358,
    "failure": 2
  },
  "mqtt_reconnects": 1,
  "consecutive_errors": 0
}
```

Publikováno na topic: `sensors/r4dcb08/{address}/diagnostics`

**tempcol:**
- Žádná diagnostika
- Pouze logging

---

## 6. Topic struktura

### 6.1 r4dcb08-mqtt

```
{prefix}/{address}/temperature/ch1    "23.5"
{prefix}/{address}/temperature/ch2    "24.1"
...
{prefix}/{address}/temperature/ch8    "22.8"
{prefix}/{address}/status             "online" / "offline"
{prefix}/{address}/timestamp          "2026-01-31 12:34:56.78"
{prefix}/{address}/diagnostics        { JSON metrics }
```

**Default prefix:** `sensors/r4dcb08`

### 6.2 tempcol

```
{topic}/CH0    "23.5"
{topic}/CH1    "24.1"
...
{topic}/CH7    "22.8"
```

**Default topic:** `r4dcb08`

### 6.3 Rozdíly

| Aspekt | r4dcb08-mqtt | tempcol |
|--------|--------------|---------|
| Indexování kanálů | 1-8 | 0-7 |
| Adresa v topic | Ano | Ne |
| Status topic | Ano | Ne |
| Timestamp topic | Ano | Ne |
| Diagnostics topic | Ano | Ne |

---

## 7. Systemd integrace

### 7.1 r4dcb08-mqtt

```ini
# /usr/local/lib/systemd/system/r4dcb08-mqtt.service
[Unit]
Description=R4DCB08 MQTT Temperature Daemon
After=network-online.target mosquitto.service
Wants=network-online.target

[Service]
Type=notify
ExecStart=/usr/local/bin/r4dcb08-mqtt -c /etc/r4dcb08-mqtt.conf
Restart=on-failure
RestartSec=5
WatchdogSec=60

[Install]
WantedBy=multi-user.target
```

**Funkce:**
- `Type=notify` - daemon signalizuje připravenost
- `WatchdogSec=60` - systemd restartuje při zaseknutí
- `Restart=on-failure` - automatický restart při chybě

### 7.2 tempcol

- Žádná systemd integrace
- Nutné vytvořit vlastní service file
- Bez watchdog podpory

---

## 8. Error handling

### 8.1 r4dcb08-mqtt

```c
typedef enum {
    MQTT_OK = 0,
    MQTT_ERR_CONFIG_FILE = -1,
    MQTT_ERR_CONFIG_VALUE = -2,
    MQTT_ERR_CONNECT = -10,
    MQTT_ERR_DISCONNECT = -11,
    MQTT_ERR_RECONNECT = -12,
    MQTT_ERR_PUBLISH = -13,
    MQTT_ERR_SERIAL = -20,
    MQTT_ERR_MODBUS = -21,
    MQTT_ERR_READ_TEMP = -22,
    // ...
} MqttStatus;
```

**Recovery:**
- 10 consecutive errors = daemon exit (supervisor restart)
- Serial error = port reopen
- MQTT disconnect = auto reconnect

### 8.2 tempcol

```rust
// anyhow::Result pro error handling
match client.read_temperatures() {
    Ok(temperatures) => { /* publish */ }
    Err(e) => {
        log::error!("Error reading temperatures: {e}");
    }
}
```

**Recovery:**
- Logging errors
- paho-mqtt auto reconnect

---

## 9. Filtry

### 9.1 r4dcb08-mqtt

**Median filter (3-point):**
```
Input:  23.5, 99.9, 23.6  (99.9 je spike)
Output: 23.5, 23.5, 23.6  (spike odstraněn)
```

**MAF filter (Moving Average):**
```
Window: 5 samples
Weights: [0.5, 1.0, 1.0, 1.0, 0.5] (trapezoidální)
Output: vážený průměr posledních 5 hodnot
```

**Kombinace:**
```bash
./r4dcb08-mqtt -m -M 5  # Median -> MAF pipeline
```

### 9.2 tempcol

- Žádné filtry
- Surová data ze senzoru

---

## 10. Výkon a zdroje

| Metrika | r4dcb08-mqtt | tempcol |
|---------|--------------|---------|
| Velikost binárky | ~150 KB | ~5-10 MB |
| RAM při běhu | ~2-3 MB | ~10-15 MB |
| CPU idle | < 1% | < 1% |
| Startup time | < 100 ms | ~500 ms |
| Závislosti | libmosquitto | Tokio runtime |

---

## 11. Bezpečnost

### 11.1 r4dcb08-mqtt

| Aspekt | Podpora |
|--------|---------|
| TLS šifrování | Ano |
| Certifikát klienta | Ano |
| Password file | Ano |
| Env variable | Ano |
| Clear credentials | Ano (po předání knihovně) |

```c
// Po nastavení credentials je vymaže z paměti
void mqtt_config_clear_sensitive(MqttConfig *config)
{
    memset(config->mqtt_pass, 0, sizeof(config->mqtt_pass));
}
```

### 11.2 tempcol

| Aspekt | Podpora |
|--------|---------|
| TLS šifrování | Ne |
| Certifikát klienta | Ne |
| Password file | Ne |
| Env variable | Ne |

---

## 12. Příklady použití

### 12.1 r4dcb08-mqtt - Production setup

```bash
# Instalace
cd mqtt_daemon && make && sudo make install

# Konfigurace
sudo nano /etc/r4dcb08-mqtt.conf

# Spuštění
sudo systemctl enable --now r4dcb08-mqtt

# Monitoring
mosquitto_sub -h localhost -t "sensors/r4dcb08/#" -v
journalctl -u r4dcb08-mqtt -f
```

### 12.2 tempcol - Basic setup

```bash
# Konfigurace
cat > mqtt.yaml << EOF
uri: "tcp://localhost:1883"
topic: r4dcb08
qos: 0
EOF

# Spuštění
tempcol rtu --device /dev/ttyUSB0 daemon --mqtt-config mqtt.yaml
```

---

## 13. Závěr

### 13.1 r4dcb08-mqtt

**Silné stránky:**
- Kompletní TLS podpora
- Last Will and Testament
- Systemd integrace s watchdog
- Filtry pro zpracování dat
- Diagnostické metriky
- Nízké nároky na zdroje
- Bezpečné zacházení s credentials

**Slabé stránky:**
- Pouze C (nutná kompilace)
- Manuální správa paměti

### 13.2 tempcol

**Silné stránky:**
- Rust memory safety
- Async/await
- Jednoduchá integrace s jinými Rust projekty
- Podpora Modbus TCP

**Slabé stránky:**
- Bez TLS
- Bez LWT
- Bez filtrů
- Bez diagnostiky
- Bez systemd integrace
- Větší binárka

### 13.3 Doporučení

| Scénář | Doporučený projekt |
|--------|-------------------|
| Production IoT deployment | **r4dcb08-mqtt** |
| Embedded Linux (resource-constrained) | **r4dcb08-mqtt** |
| Šifrovaná komunikace | **r4dcb08-mqtt** |
| Monitoring a diagnostika | **r4dcb08-mqtt** |
| Rust ekosystém | tempcol |
| Modbus TCP | tempcol |
| Rychlý prototyp | tempcol |

**Celkové hodnocení:** Pro produkční IoT nasazení je **r4dcb08-mqtt** výrazně lepší volbou díky TLS, LWT, systemd integraci a diagnostice.

---

## Reference

- [libmosquitto Documentation](https://mosquitto.org/api/files/mosquitto-h.html)
- [Eclipse Paho MQTT Rust](https://github.com/eclipse/paho.mqtt.rust)
- [MQTT 3.1.1 Specification](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- r4dcb08-mqtt README: `mqtt_daemon/README.md`
