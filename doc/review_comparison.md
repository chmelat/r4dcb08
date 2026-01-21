# Srovnávací analýza projektů R4DCB08

**Datum:** 2026-01-21
**Autor:** Code Review
**Verze:** 1.0

---

## 1. Úvod

Tento dokument obsahuje kritické srovnání dvou implementací komunikační utility pro teplotní modul R4DCB08:

- **Projekt A:** Lokální C implementace (r4dcb08, verze 1.9)
- **Projekt B:** R4DCB08-Temperature-Collector (Rust, verze 0.4.0)

Oba projekty implementují Modbus RTU protokol pro komunikaci s 8-kanálovým teplotním senzorem R4DCB08.

---

## 2. Základní charakteristiky

| Parametr | Projekt A (C) | Projekt B (Rust) |
|----------|---------------|------------------|
| Programovací jazyk | C (ANSI/POSIX) | Rust 2021 |
| Verze | 1.9 | 0.4.0 |
| Licence | MIT | MIT / Apache-2.0 |
| Název binárky | `r4dcb08` | `tempcol` |
| Velikost binárky | ~100 KB | ~5-10 MB |
| Závislosti | Žádné (stdlib) | 10+ crates |

---

## 3. Porovnání funkcionality

### 3.1 Podporované operace

| Funkce | Projekt A | Projekt B | Poznámka |
|--------|:---------:|:---------:|----------|
| Čtení teplot | ✓ | ✓ | A: 1-8 kanálů volitelně, B: vždy 8 |
| Modbus RTU | ✓ | ✓ | Obě implementace |
| Modbus TCP | ✗ | ✓ | Pouze B |
| Korekce teplot (čtení) | ✓ | ✓ | |
| Korekce teplot (zápis) | ✓ | ✓ | |
| Změna adresy zařízení | ✓ | ✓ | |
| Změna baudrate | ✓ | ✓ | |
| Skenování sběrnice | ✗ | ✓ | Pouze B |
| Factory reset | ✗ | ✓ | Pouze B |
| Automatic report | ✗ | ✓ | Pouze B |
| Daemon režim | ✗ | ✓ | Pouze B |
| MQTT publikace | ✗ | ✓ | Pouze B |
| Mediánový filtr | ✓ | ✗ | Pouze A |
| Volitelný timestamp | ✓ | ✗ | Pouze A (-f flag) |
| Volitelný počet kanálů | ✓ | ✗ | Pouze A (-n flag) |

### 3.2 Příkazová řádka

**Projekt A:**
```
./r4dcb08 [-p port] [-a addr] [-b baud] [-t interval] [-n channels]
          [-c] [-w addr] [-x baud_code] [-s ch,value] [-m] [-f] [-h]
```

**Projekt B:**
```
tempcol rtu --device <port> --address <addr> --baud-rate <baud> <COMMAND>
tempcol tcp <IP:PORT> <COMMAND>
tempcol rtu-scan --device <port>
```

---

## 4. Architektura

### 4.1 Projekt A (C)

```
r4dcb08/
├── main.c              # Vstupní bod, zpracování argumentů
├── serial.c/.h         # Sériová komunikace, flock()
├── packet.c/.h         # Modbus pakety, CRC16
├── config.c/.h         # Konfigurace programu
├── error.c/.h          # Chybové kódy
├── median_filter.c/.h  # 3-bodový mediánový filtr
├── signal_handler.c/.h # POSIX signály
├── read_functions.c/.h # Čtení z registrů
├── write_functions.c/.h# Zápis do registrů
├── now.c/.h            # Časové funkce
├── monada.c/.h         # Pomocné funkce
└── Makefile
```

**Charakteristika:**
- Procedurální přístup
- Vlastní implementace Modbus RTU
- Minimální závislosti
- POSIX kompatibilita

### 4.2 Projekt B (Rust)

```
R4DCB08-Temperature-Collector/
├── src/
│   ├── main.rs              # CLI aplikace
│   ├── lib.rs               # Knihovní rozhraní
│   ├── protocol.rs          # Datové struktury protokolu
│   ├── commandline.rs       # Clap definice
│   ├── mqtt.rs              # MQTT integrace
│   ├── tokio_sync.rs        # Sync Modbus operace
│   ├── tokio_async.rs       # Async Modbus operace
│   ├── tokio_sync_safe_client.rs
│   └── tokio_async_safe_client.rs
├── Cargo.toml
└── .github/workflows/       # CI/CD
```

**Charakteristika:**
- Knihovna + CLI binárka
- Využívá tokio-modbus crate
- Thread-safe klienti
- Async/await podpora
- Feature flags pro modulární sestavení

---

## 5. Kvalita kódu

### 5.1 Projekt A

| Kritérium | Hodnocení | Komentář |
|-----------|-----------|----------|
| Čitelnost | Dobrá | Jasná struktura, komentáře |
| Modularita | Dobrá | Oddělené moduly podle funkce |
| Bezpečnost | Dobrá | Validace vstupů, kontrola NULL |
| Testování | Chybí | Žádné automatizované testy |
| Dokumentace | Dobrá | README s příklady |
| Portabilita | Dobrá | POSIX (Linux/Unix) |

**Pozitivní aspekty:**
- Použití sigaction() místo deprecated signal()
- Exkluzivní zámek sériového portu (flock)
- Kontrola přetečení bufferů
- Nahrazení usleep() za nanosleep()

### 5.2 Projekt B

| Kritérium | Hodnocení | Komentář |
|-----------|-----------|----------|
| Čitelnost | Velmi dobrá | Rust idiomy, dokumentace |
| Modularita | Výborná | Knihovna + CLI odděleny |
| Bezpečnost | Výborná | Rust memory safety |
| Testování | Výborné | 1400+ řádků unit testů |
| Dokumentace | Výborná | Rustdoc, README, příklady |
| Portabilita | Dobrá | Cross-platform (s omezeními) |

**Pozitivní aspekty:**
- Komprehensivní error handling (thiserror)
- Type-safe API (newtype pattern)
- Serde podpora pro serializaci
- CI/CD integrace

---

## 6. Výkonnostní charakteristiky

| Metrika | Projekt A | Projekt B |
|---------|-----------|-----------|
| Čas kompilace | < 1s | 30-60s |
| Velikost binárky | ~100 KB | ~5-10 MB |
| Paměťová náročnost | Minimální | Vyšší (Tokio runtime) |
| Startovací čas | Okamžitý | Mírně pomalejší |
| CPU při idle | Zanedbatelné | Zanedbatelné |

---

## 7. Unikátní funkce

### 7.1 Pouze v Projektu A

1. **Mediánový filtr (-m)**
   - 3-bodový mediánový filtr pro odstranění špiček
   - Užitečné pro zašuměná měření

2. **Volitelný počet kanálů (-n)**
   - Čtení pouze potřebných kanálů
   - Efektivnější komunikace

3. **Výstup bez timestamp (-f)**
   - Jednořádkový výstup pro skripty
   - Snadné parsování

4. **Konfigurovatelný interval (-t)**
   - Přesně definovaný interval měření

### 7.2 Pouze v Projektu B

1. **Modbus TCP**
   - Komunikace přes Ethernet/WiFi

2. **MQTT integrace**
   - Publikace do MQTT brokeru
   - IoT ready

3. **Daemon režim**
   - Dlouhodobý běh na pozadí

4. **Factory reset**
   - Obnovení továrního nastavení

5. **Bus scan**
   - Automatické vyhledání zařízení

6. **Automatic report**
   - Konfigurace automatického reportování

7. **Knihovní API**
   - Použitelné jako závislost v jiných projektech

---

## 8. Registrová mapa (společná)

| Registr | Popis | Operace |
|---------|-------|---------|
| 0x0000-0x0007 | Teploty kanálů 1-8 | R |
| 0x0008-0x000F | Korekce teplot 1-8 | R/W |
| 0x00FD | Automatic report interval | R/W |
| 0x00FE | Adresa zařízení | R/W |
| 0x00FF | Baudrate / Factory reset | R/W |

---

## 9. Doporučení

### 9.1 Pro Projekt A (C)

**Zvážit implementaci:**
- [ ] Factory reset (zápis 5 na 0x00FF)
- [ ] Čtení automatic report (0x00FD)
- [ ] Základní unit testy (alespoň CRC)
- [ ] TCP podpora (volitelně)

**Zachovat:**
- Mediánový filtr - praktická funkce
- Flexibilní výstupní formát
- Volbu počtu kanálů
- Nízkou paměťovou náročnost

### 9.2 Pro Projekt B (Rust)

**Zvážit implementaci:**
- [ ] Mediánový filtr
- [ ] Volitelný počet kanálů
- [ ] Jednodušší výstupní formát

---

## 10. Závěr

| Aspekt | Vítěz | Odůvodnění |
|--------|-------|------------|
| Jednoduchost | A | Minimální závislosti, snadná kompilace |
| Funkcionalita | B | TCP, MQTT, daemon, factory reset |
| Výkon | A | Menší binárka, rychlejší start |
| Bezpečnost kódu | B | Rust memory safety |
| Testování | B | Automatizované testy |
| Praktičnost (embedded) | A | Nízké nároky na zdroje |
| Praktičnost (IoT) | B | MQTT, TCP |
| Flexibilita výstupu | A | Volba kanálů, formát |

**Celkové hodnocení:**

Oba projekty jsou kvalitní implementace pro své účely. Projekt A (C) je optimalizován pro jednoduchost, nízké nároky na zdroje a praktické použití v shellových skriptech. Projekt B (Rust) je komplexnější řešení vhodné pro IoT aplikace a jako knihovna pro další vývoj.

Pro většinu případů použití na embedded Linux systémech (jako Orange Pi) je Projekt A plně dostačující a nabízí unikátní funkce (mediánový filtr, flexibilní výstup), které Projekt B postrádá.

---

**Konec dokumentu**
