# TODO - Future Enhancements

## Automatic Report Feature

**Registr:** 0x00FD

**Popis:** Zařízení R4DCB08 podporuje automatické odesílání teplot v pravidelných intervalech.

**Hodnoty:**
- 0 = vypnuto (výchozí, query mode)
- 1-255 = interval v sekundách

**Možná implementace:**
- `-i [seconds]` - nastavit interval automatického reportu
- `-I` - přečíst aktuální nastavení intervalu

**Priorita:** Nízká (funkce není běžně používána)

**Poznámka:** Tato funkce je implementována v Rust projektu R4DCB08-Temperature-Collector.

---

*Vytvořeno: 2026-01-21*
