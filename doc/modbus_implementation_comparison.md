# Porovnání implementace Modbus RTU: r4dcb08 vs libmodbus

**Datum:** 2026-01-31
**Verze:** 1.0
**Autor:** Analýza kódu

---

## 1. Úvod

Tento dokument porovnává implementaci protokolu Modbus RTU použitou v projektu r4dcb08 s referenční knihovnou [libmodbus](https://github.com/stephane/libmodbus), která je de facto standardem pro Modbus komunikaci v C/C++ aplikacích.

### 1.1 Přehled projektů

| Projekt | Popis |
|---------|-------|
| **r4dcb08** | Jednoúčelová utilita pro komunikaci s teplotním senzorem R4DCB08 přes RS485 |
| **libmodbus** | Univerzální open-source knihovna pro Modbus RTU/TCP komunikaci |

---

## 2. Architektura

### 2.1 r4dcb08

Projekt používá modulární architekturu s jasným oddělením vrstev:

```
┌─────────────────────────────────────┐
│          Aplikační vrstva           │
│  (main.c, read_functions.c, ...)    │
├─────────────────────────────────────┤
│        Komunikační wrapper          │
│            (monada.c)               │
├─────────────────────────────────────┤
│         Protokolová vrstva          │
│           (packet.c)                │
├─────────────────────────────────────┤
│       Transportní vrstva            │
│           (serial.c)                │
└─────────────────────────────────────┘
```

**Klíčové soubory:**
- `packet.c/h` - RTU framing, CRC, odesílání/příjem
- `serial.c/h` - Konfigurace sériového portu
- `monada.c/h` - High-level komunikační wrapper
- `typedef.h` - Definice datových struktur

### 2.2 libmodbus

Knihovna používá backend architekturu s kontextovým objektem:

```
┌─────────────────────────────────────┐
│           Unified API               │
│    modbus_read_registers(), ...     │
├─────────────────────────────────────┤
│          Core Layer                 │
│         (modbus.c)                  │
├──────────────┬──────────────────────┤
│  RTU Backend │  TCP Backend         │
│ (modbus-rtu) │ (modbus-tcp)         │
└──────────────┴──────────────────────┘
```

### 2.3 Srovnání přístupu

| Aspekt | r4dcb08 | libmodbus |
|--------|---------|-----------|
| **Přístup** | Jednoúčelový | Univerzální knihovna |
| **Backend** | Pouze RTU/serial | RTU, TCP, TCP-PI |
| **Platformy** | POSIX (Linux) | Linux, Windows, macOS, FreeBSD, QNX |
| **Kontext** | Globální file descriptor | `modbus_t*` context object |
| **Role** | Pouze master/client | Master i slave |

---

## 3. CRC-16 implementace

### 3.1 Specifikace Modbus CRC-16

Obě implementace dodržují standard Modbus CRC-16:
- **Polynom:** 0xA001 (reverzní reprezentace 0x8005)
- **Inicializační hodnota:** 0xFFFF
- **Byte order:** LSB first (little-endian)

### 3.2 r4dcb08 - Bit-by-bit algoritmus

Soubor `packet.c`, řádky 282-306:

```c
static uint16_t CRC16_2(const uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFF;

    if (buf == NULL || len <= 0) {
        return 0;
    }

    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];    /* XOR byte into least sig. byte of crc */

        for (int i = 8; i != 0; i--) {    /* Loop over each bit */
            if ((crc & 0x0001) != 0) {    /* If the LSB is set */
                crc >>= 1;                /* Shift right and XOR 0xA001 */
                crc ^= 0xA001;
            }
            else {                        /* Else LSB is not set */
                crc >>= 1;                /* Just shift right */
            }
        }
    }

    return crc;
}
```

**Charakteristika:**
- 8 bitových operací na každý byte
- Žádné paměťové nároky na lookup tabulky
- Časová složitost: O(8n) kde n = počet bytů

### 3.3 libmodbus - Table-driven algoritmus

```c
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length) {
    uint8_t crc_hi = 0xFF;
    uint8_t crc_lo = 0xFF;
    unsigned int i;

    while (buffer_length--) {
        i = crc_lo ^ *buffer++;
        crc_lo = crc_hi ^ table_crc_hi[i];
        crc_hi = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}
```

**Charakteristika:**
- 1 lookup operace na každý byte
- Vyžaduje 2x 256 bytů pro lookup tabulky (512 B celkem)
- Časová složitost: O(n) kde n = počet bytů

### 3.4 Porovnání CRC implementací

| Aspekt | r4dcb08 | libmodbus |
|--------|---------|-----------|
| **Algoritmus** | Bit-by-bit | Table-driven |
| **Polynom** | 0xA001 | 0xA001 |
| **Init hodnota** | 0xFFFF | 0xFFFF |
| **Byte order** | LSB first | LSB first |
| **Paměť** | 0 B extra | 512 B tabulky |
| **Rychlost** | ~8x pomalejší | Rychlejší |
| **Korektnost** | Správná | Správná |

**Závěr:** Obě implementace produkují identické výsledky. Rozdíl je pouze ve výkonu vs. paměťových nárocích.

---

## 4. Struktura paketu

### 4.1 r4dcb08 - Datová struktura

Definice v `typedef.h`:

```c
#define DMAX 253  /* Maximum data length */

typedef struct {
    uint8_t addr;           /* Slave address (1-247) */
    uint8_t inst;           /* Function code */
    uint8_t len;            /* Data length */
    uint8_t data[DMAX+2];   /* Data buffer */
    uint16_t CRC;           /* CRC-16 checksum */
} PACKET;
```

### 4.2 Formát RTU rámce

```
┌──────┬──────┬─────────────┬───────┬───────┐
│ ADDR │ FUNC │   DATA...   │ CRC_L │ CRC_H │
│ 1B   │ 1B   │   0-253B    │  1B   │  1B   │
└──────┴──────┴─────────────┴───────┴───────┘
```

- **Minimální velikost:** 4 byty (ADDR + FUNC + CRC)
- **Maximální velikost:** 257 bytů (ADDR + FUNC + 253 DATA + CRC)

---

## 5. Podporované Modbus funkce

### 5.1 Přehled podpory

| Kód | Funkce | r4dcb08 | libmodbus |
|-----|--------|---------|-----------|
| 0x01 | Read Coils | - | Ano |
| 0x02 | Read Discrete Inputs | - | Ano |
| 0x03 | Read Holding Registers | **Ano** | Ano |
| 0x04 | Read Input Registers | - | Ano |
| 0x05 | Write Single Coil | - | Ano |
| 0x06 | Write Single Register | **Ano** | Ano |
| 0x0F | Write Multiple Coils | - | Ano |
| 0x10 | Write Multiple Registers | - | Ano |
| 0x17 | Read/Write Multiple Registers | - | Ano |

### 5.2 r4dcb08 - Implementované operace

**Funkce 0x03 - Read Holding Registers:**
```
Request:  [ADDR][0x03][REG_HI][REG_LO][CNT_HI][CNT_LO][CRC_L][CRC_H]
Response: [ADDR][0x03][BYTE_CNT][DATA...][CRC_L][CRC_H]
```

**Funkce 0x06 - Write Single Register:**
```
Request:  [ADDR][0x06][REG_HI][REG_LO][VAL_HI][VAL_LO][CRC_L][CRC_H]
Response: [ADDR][0x06][REG_HI][REG_LO][VAL_HI][VAL_LO][CRC_L][CRC_H]
```

---

## 6. Timeout a komunikace

### 6.1 r4dcb08

Soubor `packet.c`, funkce `read_with_timeout()`:

```c
static int read_with_timeout(int fd, uint8_t *buffer, int size, int timeout_ms)
{
    fd_set readfds;
    struct timeval tv;

    while (bytes_read < size) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        result = select(fd + 1, &readfds, NULL, NULL, &tv);
        // ...
    }
}
```

**Charakteristika:**
- Použití POSIX `select()` pro multiplexing
- Default timeout: 500 ms
- Konfigurovatelný per-call timeout
- Čtení po částech podle očekávané délky

### 6.2 libmodbus

- Nepoužívá časově založený inter-character timeout
- Čte dokud nedostane kompletní očekávanou odpověď
- "All bytes are sent as fast as possible"
- Konfigurovatelné response timeout

### 6.3 Srovnání

| Aspekt | r4dcb08 | libmodbus |
|--------|---------|-----------|
| **Timeout mechanismus** | `select()` | Platform-specific |
| **Default timeout** | 500 ms | 500 ms |
| **Inter-char timeout** | Ne | Ne (optimalizace) |
| **Konfigurovatelnost** | Per-call | Globální kontext |

---

## 7. Error handling

### 7.1 r4dcb08 - Vlastní enum

```c
typedef enum {
    STATUS_OK = 0,

    /* Argument errors (-10 to -19) */
    ERROR_ARG_PORT = -10,
    ERROR_ARG_ADDRESS = -11,
    ERROR_ARG_BAUDRATE = -12,

    /* Communication errors (-20 to -29) */
    ERROR_COMM_PORT_INIT = -20,
    ERROR_PACKET_CRC = -21,
    ERROR_PACKET_TIMEOUT = -22,
    ERROR_PACKET_OVERFLOW = -23,
    // ...
} AppStatus;
```

### 7.2 libmodbus - POSIX konvence

```c
// Návratová hodnota -1 při chybě
if (modbus_read_registers(ctx, 0, 10, tab_reg) == -1) {
    fprintf(stderr, "Error: %s\n", modbus_strerror(errno));
}
```

### 7.3 Srovnání

| Aspekt | r4dcb08 | libmodbus |
|--------|---------|-----------|
| **Konvence** | Vlastní enum | POSIX errno |
| **Chybové zprávy** | `fprintf(stderr, ...)` | `modbus_strerror()` |
| **Granularita** | Detailní kategorie | Standardní POSIX |

---

## 8. API design

### 8.1 r4dcb08 - Procedurální přístup

```c
// Otevření portu
int fd = open_port("/dev/ttyUSB0");
set_port(fd, B9600);

// Nízkoúrovňové operace
PACKET packet;
form_packet(addr, inst, data, len, &packet);
send_packet(fd, &packet, &bytes_sent);
received_packet(fd, &packet, mode);

// Vyšší úroveň
uint8_t *result;
monada(fd, addr, inst, data, mode, &result, verbose);

// Aplikační úroveň
read_temperature(fd, addr, channel, &temp, verbose);
```

### 8.2 libmodbus - Objektově orientovaný přístup

```c
// Vytvoření kontextu
modbus_t *ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);

// Konfigurace
modbus_set_slave(ctx, 1);
modbus_set_response_timeout(ctx, 0, 500000);

// Připojení
modbus_connect(ctx);

// Operace
uint16_t registers[10];
modbus_read_registers(ctx, 0, 10, registers);
modbus_write_register(ctx, 0, 0x1234);

// Cleanup
modbus_close(ctx);
modbus_free(ctx);
```

---

## 9. Výhody a nevýhody

### 9.1 r4dcb08

**Výhody:**
- Jednoduchý, přehledný a snadno auditovatelný kód
- Minimální závislosti (pouze standardní C knihovny)
- Optimalizovaný pro konkrétní zařízení R4DCB08
- Obsahuje užitečné filtry (median, MAF) pro zpracování dat
- Menší binary footprint
- Nižší paměťové nároky (bez CRC tabulek)

**Nevýhody:**
- Pouze 2 Modbus funkce (0x03, 0x06)
- Pomalejší CRC výpočet
- Pouze Linux/POSIX platformy
- Pouze master/client role
- Pouze RTU transport

### 9.2 libmodbus

**Výhody:**
- Kompletní implementace Modbus specifikace
- Multiplatformní (Linux, Windows, macOS, FreeBSD, QNX)
- Podpora RTU, TCP a TCP-PI
- Rychlejší CRC díky lookup tabulkám
- Master i slave mód
- Aktivní vývoj a komunita
- Rozsáhlá dokumentace

**Nevýhody:**
- Větší footprint (kód + tabulky)
- Více závislostí
- Komplexnější API pro jednoduché úlohy
- Overhead pro jednoúčelové aplikace

---

## 10. Závěr

### 10.1 Hodnocení implementace r4dcb08

Implementace Modbus RTU v projektu r4dcb08 je **korektní a plně kompatibilní** se standardem Modbus RTU:

| Kritérium | Hodnocení |
|-----------|-----------|
| CRC-16 algoritmus | Správný (identický výstup s libmodbus) |
| Formát RTU rámce | Správný |
| Byte ordering | Správný (LSB first) |
| Funkce 0x03 | Správně implementována |
| Funkce 0x06 | Správně implementována |
| Error handling | Robustní |
| Timeout handling | Funkční |

### 10.2 Doporučení

**Pro projekt r4dcb08:**
Stávající implementace je pro daný účel (komunikace s teplotním senzorem R4DCB08) plně dostačující. Přechod na libmodbus by přinesl minimální benefit při zvýšené komplexitě.

**Možná vylepšení:**
1. **CRC optimalizace** - Pokud je výkon kritický, lze přidat lookup tabulky (+ 512 B)
2. **Podpora více funkcí** - Pouze pokud zařízení vyžaduje další Modbus funkce

### 10.3 Kdy použít libmodbus

- Potřeba TCP/IP komunikace
- Potřeba slave/server módu
- Multiplatformní požadavky
- Komunikace s více různými zařízeními
- Potřeba kompletní Modbus specifikace

---

## Reference

1. [libmodbus GitHub Repository](https://github.com/stephane/libmodbus)
2. [libmodbus Official Website](https://libmodbus.org/)
3. [Modbus Protocol Specification](https://modbus.org/specs.php)
4. R4DCB08 Modbus RTU Protocol Documentation (./doc/R4DCB08 modbus rtu protocol.pdf)
