// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

uint32_t messageTime;
struct {
  uint32_t timestamp;
  uint8_t flags;
  uint8_t status;
  uint8_t d0;
  uint8_t d1;
} message[32];

uint32_t bpmTime;
uint32_t bpmBeatMicros[10];
uint32_t bpm;

uint32_t songPositionTime;
uint16_t songPosition;

uint32_t timecodeTime;
uint8_t timecodeFrameRate;
uint8_t timecodeFrame;
uint8_t timecodeSecond;
uint8_t timecodeMinute;
uint8_t timecodeHour;

uint32_t controlTime;
uint8_t controlChannel;
uint8_t controlNumber;
uint16_t controlValue;

uint8_t controlValues[128];

uint8_t const barPatterns[3][8] = {
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00},
  {0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00},
  {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00},
};

void setup() {
  Serial.begin(31250);
  lcd.createChar(1, barPatterns[0]);
  lcd.createChar(2, barPatterns[1]);
  lcd.createChar(3, barPatterns[2]);
  lcd.begin(16, 2);
  lcd.display();
}

void loop() {
  readMidi();
  generateDisplay();
}

void readMidi() {
  uint32_t startMillis = millis();
  while (Serial.available()) {
    parseMidi(Serial.read(), false);
  }
  while((millis() - startMillis) < 50) {
    while (Serial.available()) {
      parseMidi(Serial.read(), true);
    }
  }
}

void parseMidi(uint8_t b, bool useTiming) {
  if(useTiming) {
    uint32_t ts = micros();
  }
}

// ################
// ################

// 120.00   1342:23
// CONT --:--:--.--

// ************----
// 13/Pedal   11234

// 99/1423 !+00ff00
// Off 13 127    63
// On  13 127   101
// PB  13      3392
// CC  13 127 14524
// NA  13 127    94
// PC  13 127
// CA  13 127
// SC  X 0304050607
// SC  EOX
// SC  SPP    11928
// SC  SS       127
// SC  Tune
// SR  Beat
// SR  Strt
// SR  Cont
// SR  Stop
// SR  AS
// SR  Rst

void generateDisplay() {
  char buf[50];
  lcd.home();
  //snprintf(buf, sizeof buf, "%3d%% %4d/s %3d\r\n");
  snprintf(buf, sizeof buf, "\x03\x03\x03\x03\x03\x03\x01            ");
  lcd.write(buf);
  lcd.setCursor(0, 1);
  snprintf(buf, sizeof buf, "13/Pedal   11234");
  lcd.write(buf);
}
