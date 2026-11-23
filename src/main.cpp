#include <Arduino.h>
#include <SD.h>
#include <U8g2lib.h>
#include <ctype.h>

constexpr uint8_t MOSI_TARGET = 22, MISO_TARGET = 23, SCK_TARGET = 24, RESET_TARGET = 25;
constexpr uint8_t ENCODER_A = 2, ENCODER_B = 3, ENCODER_PUSH = 4, CONFIRM = 5, BACK = 6;
constexpr uint8_t SD_CS = 53, TARGET_SENSE = A0;
constexpr uint16_t FLASH_SIZE = 32768;
constexpr uint8_t PAGE_SIZE = 128, MAX_PROFILES = 32;

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
char profiles[MAX_PROFILES][13];
uint8_t profileCount = 0, selected = 0;
uint8_t halfPeriod = 5;

struct Profile {
  char hex[13];
  uint8_t signature;
  uint8_t low, high, extended;
};

void screen(const char *title, const char *line = "", const char *detail = "") {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 13, title);
  display.drawStr(0, 34, line);
  display.drawStr(0, 55, detail);
  display.sendBuffer();
}

bool targetPowered() {
  // 10k/10k divider and DEFAULT ADC reference (Mega supply).
  const int sample = analogRead(TARGET_SENSE);
  return sample >= 470 && sample <= 550;
}

uint8_t transfer(uint8_t value) {
  uint8_t result = 0;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    digitalWrite(MOSI_TARGET, value & 0x80 ? HIGH : LOW);
    delayMicroseconds(halfPeriod);
    digitalWrite(SCK_TARGET, HIGH);
    result = (result << 1) | digitalRead(MISO_TARGET);
    delayMicroseconds(halfPeriod);
    digitalWrite(SCK_TARGET, LOW);
    value <<= 1;
  }
  return result;
}

uint8_t command(uint8_t a, uint8_t b, uint8_t c, uint8_t d = 0) {
  transfer(a);
  transfer(b);
  transfer(c);
  return transfer(d);
}

void endIsp() {
  pinMode(MOSI_TARGET, INPUT);
  pinMode(SCK_TARGET, INPUT);
  pinMode(RESET_TARGET, INPUT);
  digitalWrite(RESET_TARGET, LOW);
}

bool beginIsp() {
  if (!targetPowered()) return false;
  pinMode(MOSI_TARGET, OUTPUT);
  pinMode(SCK_TARGET, OUTPUT);
  digitalWrite(SCK_TARGET, LOW);
  digitalWrite(RESET_TARGET, LOW);
  pinMode(RESET_TARGET, OUTPUT);
  const uint8_t periods[] = {5, 25, 100, 250};
  for (const uint8_t period : periods) {
    halfPeriod = period;
    // Release RESET through the target's pull-up, then assert it again.
    pinMode(RESET_TARGET, INPUT);
    delay(2);
    pinMode(RESET_TARGET, OUTPUT);
    delay(25);
    transfer(0xAC);
    transfer(0x53);
    const uint8_t reply = transfer(0);
    transfer(0);
    if (reply == 0x53) return true;
  }
  endIsp();
  return false;
}

bool ready() {
  const unsigned long start = millis();
  do {
    if (!targetPowered()) return false;
    if (!(command(0xF0, 0, 0) & 1)) return true;
  } while (millis() - start < 100);
  return false;
}

enum class Line { End, Ok, Invalid };

Line readLine(File &file, char *line, size_t capacity) {
  size_t used = 0;
  while (file.available()) {
    const int ch = file.read();
    if (ch < 0 || ch == 0) return Line::Invalid;
    if (ch == '\n') break;
    if (used + 1 >= capacity) return Line::Invalid;
    line[used++] = static_cast<char>(ch);
  }
  if (used && line[used - 1] == '\r') --used;
  line[used] = 0;
  return used ? Line::Ok : Line::End;
}

int nibble(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return -1;
}

int byteFromHex(const char *pair) {
  const int high = nibble(pair[0]), low = nibble(pair[1]);
  return (high < 0 || low < 0) ? -1 : (high << 4) | low;
}

bool readProfile(Profile &profile) {
  File file = SD.open(profiles[selected]);
  if (!file) return false;
  char line[40];
  // Exactly: IMAGE.HEX 0F E2 D9 FF
  if (readLine(file, line, sizeof(line)) != Line::Ok || file.available()) return false;
  const char *space = strchr(line, ' ');
  if (!space || space - line < 5 || space - line > 12 || strlen(space) != 12) return false;
  const size_t nameSize = space - line;
  if (strncmp(space - 4, ".HEX", 4) ||
      space[3] != ' ' || space[6] != ' ' || space[9] != ' ') return false;
  for (size_t index = 0; index < nameSize - 4; ++index) {
    if (!isalnum(line[index]) && line[index] != '_') return false;
  }
  memcpy(profile.hex, line, nameSize);
  profile.hex[nameSize] = 0;
  const int signature = byteFromHex(space + 1), low = byteFromHex(space + 4);
  const int high = byteFromHex(space + 7), extended = byteFromHex(space + 10);
  if ((signature != 0x0F && signature != 0x14) || low < 0 || high < 0 ||
      extended < 0 || (high & 0xE0) != 0xC0 || (extended & 7) < 5) return false;
  profile.signature = signature;
  profile.low = low;
  profile.high = high;
  profile.extended = extended | 0xF8;
  return true;
}

enum class Pass { Validate, Write, Verify };

bool finishPage(Pass pass, uint16_t address, const uint8_t *page) {
  if (!targetPowered()) return false;
  if (pass == Pass::Write) {
    for (uint8_t offset = 0; offset < PAGE_SIZE; ++offset) {
      command(offset & 1 ? 0x48 : 0x40, 0, offset >> 1, page[offset]);
    }
    const uint16_t word = address >> 1;
    command(0x4C, word >> 8, word);
    return ready();
  }
  for (uint8_t offset = 0; offset < PAGE_SIZE; ++offset) {
    const uint16_t word = (address + offset) >> 1;
    if (command(offset & 1 ? 0x28 : 0x20, word >> 8, word) != page[offset]) return false;
  }
  return true;
}

bool processHex(const char *name, Pass pass) {
  File file = SD.open(name);
  if (!file) return false;
  char line[525];
  uint8_t page[PAGE_SIZE];
  memset(page, 0xFF, sizeof(page));
  uint16_t pageAddress = 0, nextAddress = 0;
  bool hasData = false, pageTouched = false;
  while (readLine(file, line, sizeof(line)) == Line::Ok) {
    const size_t length = strlen(line);
    if (length < 11 || line[0] != ':') return false;
    const int count = byteFromHex(line + 1);
    if (count < 0 || length != static_cast<size_t>(11 + 2 * count)) return false;
    uint8_t checksum = 0;
    for (size_t index = 1; index < length; index += 2) {
      const int value = byteFromHex(line + index);
      if (value < 0) return false;
      checksum += value;
    }
    if (checksum) return false;
    const unsigned address = (unsigned(byteFromHex(line + 3)) << 8) | byteFromHex(line + 5);
    const int type = byteFromHex(line + 7);
    if (type == 1) {
      if (count || address || !hasData || file.available()) return false;
      if (pass == Pass::Validate) return true;
      if (!finishPage(pass, pageAddress, page)) return false;
      if (pass == Pass::Verify) {
        memset(page, 0xFF, sizeof(page));
        for (uint16_t blank = pageAddress + PAGE_SIZE; blank < FLASH_SIZE; blank += PAGE_SIZE) {
          if (!finishPage(pass, blank, page)) return false;
        }
      }
      return true;
    }
    if ((type == 2 || type == 4) && !hasData && !address && count == 2 &&
        !byteFromHex(line + 9) && !byteFromHex(line + 11)) continue;
    if (type || !count || address < nextAddress || uint32_t(address) + count > FLASH_SIZE) return false;
    hasData = true;
    nextAddress = address + count;
    if (pass == Pass::Validate) continue;
    for (int index = 0; index < count; ++index) {
      const uint16_t current = address + index;
      while (current >= pageAddress + PAGE_SIZE) {
        if (pass == Pass::Verify || pageTouched) {
          // Write only pages touched by HEX data. Verify gaps as erased.
          if (!finishPage(pass, pageAddress, page)) return false;
        }
        pageAddress += PAGE_SIZE;
        pageTouched = false;
        memset(page, 0xFF, sizeof(page));
      }
      pageTouched = true;
      page[current % PAGE_SIZE] = byteFromHex(line + 9 + 2 * index);
    }
  }
  return false;
}

bool fuse(uint8_t writeCode, uint8_t readCode, uint8_t readArg, uint8_t value, uint8_t mask = 0xFF) {
  if ((command(readCode, readArg, 0) & mask) == (value & mask)) return true;
  if (!targetPowered()) return false;
  command(0xAC, writeCode, 0, value);
  return ready() && (command(readCode, readArg, 0) & mask) == (value & mask);
}

const char *program(const Profile &profile) {
  if (command(0x30, 0, 0) != 0x1E || command(0x30, 0, 1) != 0x95 ||
      command(0x30, 0, 2) != profile.signature) return "Signatur falsch";
  const uint8_t oldHigh = command(0x58, 0x08, 0);
  // Refuse a destructive erase when the profile requests preservation.
  if (!(profile.high & 8) && (oldHigh & 8)) return "EESAVE nicht gesetzt";
  screen("Loeschen", profile.hex);
  if (!targetPowered()) return "Zielspannung fehlt";
  command(0xAC, 0x80, 0);
  if (!ready()) return "Loeschen fehlgeschl.";
  screen("Flash schreiben", profile.hex);
  if (!processHex(profile.hex, Pass::Write)) return "Schreibfehler";
  screen("Flash pruefen", profile.hex);
  if (!processHex(profile.hex, Pass::Verify)) return "Verify-Fehler";
  screen("Fuses pruefen", profile.hex);
  if (!fuse(0xA8, 0x50, 0x08, profile.extended, 7) ||
      !fuse(0xA4, 0x58, 0x08, profile.high) ||
      !fuse(0xA0, 0x50, 0, profile.low)) return "Fuse-Fehler";
  return nullptr;
}

void menu() {
  screen("AVR ISP - SD", profiles[selected], "Drehen / CONFIRM");
}

bool pressed(uint8_t pin) {
  if (digitalRead(pin)) return false;
  delay(20);
  if (digitalRead(pin)) return false;
  while (!digitalRead(pin)) delay(1);
  return true;
}

void setup() {
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(ENCODER_PUSH, INPUT);
  pinMode(CONFIRM, INPUT);
  pinMode(BACK, INPUT);
  display.begin();
  if (!SD.begin(SD_CS)) {
    screen("SD-Fehler", "Karte pruefen");
    return;
  }
  File root = SD.open("/");
  while (root && profileCount < MAX_PROFILES) {
    File entry = root.openNextFile();
    if (!entry) break;
    const size_t length = strlen(entry.name());
    if (!entry.isDirectory() && length > 4 && length <= 12 &&
        !strcasecmp(entry.name() + length - 4, ".CFG")) {
      strcpy(profiles[profileCount++], entry.name());
    }
    entry.close();
  }
  if (profileCount) menu();
  else screen("Keine Profile", "*.CFG im SD-Root");
}

void loop() {
  if (!profileCount) return;
  static uint8_t last = digitalRead(ENCODER_A);
  static bool alternate = false;
  const uint8_t current = digitalRead(ENCODER_A);
  if (current != last) {
    last = current;
    if (!current) {
      selected = (selected + (digitalRead(ENCODER_B) ? 1 : profileCount - 1)) % profileCount;
      alternate = false;
      menu();
    }
  }
  if (pressed(BACK)) menu();
  if (pressed(ENCODER_PUSH)) {
    Profile profile;
    if (readProfile(profile)) {
      char detail[22];
      if (alternate) snprintf(detail, sizeof(detail), "Fuses %02X %02X %02X", unsigned(profile.low), unsigned(profile.high), unsigned(profile.extended));
      else snprintf(detail, sizeof(detail), "%s", profile.signature == 0x0F ? "ATmega328P" : "ATmega328");
      alternate = !alternate;
      screen(profiles[selected], profile.hex, detail);
    } else screen("Profil ungueltig");
  }
  if (pressed(CONFIRM)) {
    Profile profile;
    if (!readProfile(profile)) screen("Profil ungueltig");
    else {
      screen("HEX pruefen", profile.hex);
      if (!processHex(profile.hex, Pass::Validate)) screen("HEX ungueltig");
      else if (!targetPowered()) screen("Zielspannung fehlt");
      else if (!beginIsp()) screen("ISP antwortet nicht");
      else {
        const char *error = program(profile);
        endIsp();
        screen(error ? error : "OK - verifiziert", profile.hex);
      }
    }
  }
}
