# Verdrahtung

## OLED und Encoder

| Modul | ATmega2560 Pro |
|---|---|
| VCC / GND | 3,3 V / gemeinsame Masse |
| SDA / SCL | D20 / D21 über bidirektionalen I²C-Pegelwandler |
| TRA / TRB | D2 / D3 |
| PSH / CON / BAK | D4 / D5 / D6 |

I²C-Pegelwandler mit 3,3 V auf LV und 5 V auf HV betreiben. SH1106 und
Adresse 0x3C sind die Startannahmen. Die Eingänge für Tasten und Encoder
verwenden externe Pull-ups nach 3,3 V (auf dem Modul vorhanden oder, nach
Messung, ergänzen). Vor Anschluss die Ruhezustände prüfen. Den internen
5-V-Pull-up des Mega deaktiviert lassen.

## microSD

| Modul | Mega |
|---|---|
| MISO / MOSI / SCK / CS | D50 / D51 / D52 / D53 |
| GND | gemeinsame Masse |
| VCC | laut Datenblatt des konkreten Moduls |

Modulversorgung und 5-V-Logiktauglichkeit anhand der IC-Aufdrucke prüfen.
D50–D52 sind auch am ICSP-Anschluss des Mega verfügbar.

## Ziel-ISP

| 6-poliger AVR-ISP-Pin | Signal | Programmer |
|---|---|---|
| 1 | MISO | D23 |
| 2 | VTARGET | Messung über Spannungsteiler |
| 3 | SCK | D24 |
| 4 | MOSI | D22 |
| 5 | RESET | D25, zieht den Ziel-Reset nach Masse |
| 6 | GND | gemeinsame Masse |

Den Spannungsteiler als VTARGET → 10 kΩ → A0 → 10 kΩ → GND anschließen.
Alle Ziele werden extern mit 5 V versorgt. VTARGET ausschließlich mit
dem Messzweig verbinden. Am Ziel einen RESET-Pull-up (z. B. 10 kΩ nach
Ziel-VCC) verwenden. Beim gesockelten DIP-Test VCC und AVCC sowie beide
Massepins anschließen und Versorgungspins abblocken. Quarz bzw. Taktquelle
müssen zu den Ziel-Fuses passen. Das AVR-ISP-Flachbandkabel kurz halten.

## Referenzen

- [ATmega328P-Datenblatt](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)
- [Arduino Mega 2560](https://docs.arduino.cc/hardware/mega-2560)
- [Vergleichbares OLED-Frontpanel](https://www.hestore.eu/en/prod_10048023.html)
