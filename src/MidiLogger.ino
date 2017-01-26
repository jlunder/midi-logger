// include the library code:
#include <LiquidCrystal.h>

#define LENGTHOF(x) (sizeof (x) / sizeof (*x))

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);
static uint8_t const lcdWidth = 16;
static uint8_t const barPatterns[6][8] = {
  {0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00},
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00},
  {0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x00},
  {0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00},
  {0x15, 0x14, 0x15, 0x14, 0x15, 0x14, 0x15, 0x00},
  {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00},
};

struct MidiMessage {
  enum class Flags : uint8_t {
    timestampGood = 0x01,
    runningStatus = 0x02,
  };

  uint32_t timestamp;
  uint8_t flags;
  uint8_t status;
  uint8_t d0;
  uint8_t d1;
};

uint32_t msgTime;
uint8_t msgCur;
MidiMessage msgHistory[32];

static uint32_t const resetMs = 2000;

enum class DisplayMode : uint8_t {
  trafficSummary = 0,
  clockAndTime = 1,
  controlChange = 2,
  messageHistory = 3,
  controlValues = 4,
  noteValues = 5,
};

static uint8_t const numDisplayModes = 6;

DisplayMode displayMode;
uint8_t channelFilter;
bool upHeld;
bool downHeld;
bool shiftHeld;
bool maybeReset;
uint32_t resetTimer;

static uint8_t const invalidChannel = 16;

uint32_t lastMillis;

void setup() {
  displayMode = DisplayMode::trafficSummary;
  channelFilter = 0;
  maybeReset = false;

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  Serial.begin(31250);
  lcd.createChar(1, (uint8_t *)barPatterns[0]);
  lcd.createChar(2, (uint8_t *)barPatterns[1]);
  lcd.createChar(3, (uint8_t *)barPatterns[2]);
  lcd.createChar(4, (uint8_t *)barPatterns[3]);
  lcd.createChar(5, (uint8_t *)barPatterns[4]);
  lcd.createChar(6, (uint8_t *)barPatterns[5]);
  lcd.begin(lcdWidth, 2);
  lcd.display();

  lastMillis = millis();
}

void loop() {
  uint32_t dt = millis() - lastMillis;
  lastMillis += dt;

  readMidi();
  processInterface(dt);
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

void processInterface(uint32_t dt) {
  bool upState = !digitalRead(2);
  bool upPressed = upState && !upHeld;
  bool downState = !digitalRead(3);
  bool downPressed = downState && !downHeld;
  bool shiftState = !digitalRead(4);
  bool resetPressed = false;

  upHeld = upState;
  downHeld = downState;

  if(shiftState) {
    if(!shiftHeld) {
      resetTimer = 0;
      shiftHeld = true;
      maybeReset = !(upState || downState);
    } else {
      if(maybeReset) {
        if(upState || downState) {
          maybeReset = false;
        } else {
          if(resetTimer > resetMs) {
            resetPressed = true;
          } else {
            resetTimer += dt;
          }
        }
      }
    }
  } else {
    shiftHeld = false;
  }

  if(!shiftState && upPressed) {
    displayMode = static_cast<DisplayMode>(
      (static_cast<uint8_t>(displayMode) + numDisplayModes - 1)
      % numDisplayModes);
  }
  if(!shiftState && downPressed) {
    displayMode = static_cast<DisplayMode>(
      (static_cast<uint8_t>(displayMode) + 1) % numDisplayModes);
  }

  switch(displayMode) {
    case DisplayMode::trafficSummary:
      processInterfaceTrafficSummary(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
    case DisplayMode::clockAndTime:
      processInterfaceClockAndTime(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
    case DisplayMode::controlChange:
      processInterfaceControlChange(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
    case DisplayMode::messageHistory:
      processInterfaceMessageHistory(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
    case DisplayMode::controlValues:
      processInterfaceControlValues(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
    case DisplayMode::noteValues:
      processInterfaceNoteValues(dt, shiftState && upPressed,
        shiftState && downPressed, resetPressed);
      break;
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
  char buf[2][lcdWidth + 1];

  switch(displayMode) {
    case DisplayMode::trafficSummary:
      generateDisplayTrafficSummary(buf[0], buf[1]);
      break;
    case DisplayMode::clockAndTime:
      generateDisplayClockAndTime(buf[0], buf[1]);
      break;
    case DisplayMode::controlChange:
      generateDisplayControlChange(buf[0], buf[1]);
      break;
    case DisplayMode::messageHistory:
      generateDisplayMessageHistory(buf[0], buf[1]);
      break;
    case DisplayMode::controlValues:
      generateDisplayControlValues(buf[0], buf[1]);
      break;
    case DisplayMode::noteValues:
      generateDisplayNoteValues(buf[0], buf[1]);
      break;
  }

  lcd.home();
  lcd.write(buf[0], lcdWidth);
  lcd.setCursor(0, 1);
  lcd.write(buf[1], lcdWidth);
}

void setupTrafficSummary() {

}

void processMidiTrafficSummary(MidiMessage const * msg) {

}

void processInterfaceTrafficSummary(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayTrafficSummary(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Traffic Summary 0");
  snprintf(buf1, lcdWidth + 1, "Traffic Summary 1");
}

uint32_t clkTime;
uint32_t clkBeatMicros[10];
uint32_t clkBpm;
uint32_t sppTime;
uint16_t sppSongPosition;
uint32_t tcTime;
uint8_t tcFrameRate;
uint8_t tcFrame;
uint8_t tcSecond;
uint8_t tcMinute;
uint8_t tcHour;

void setupClockAndTime() {

}

void processMidiClockAndTime(MidiMessage const * msg) {

}

void processInterfaceClockAndTime(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayClockAndTime(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Traffic Summary 0");
  snprintf(buf1, lcdWidth + 1, "Traffic Summary 1");
}

uint32_t ccTime;
uint8_t ccChannel;
uint8_t ccNumber;
uint16_t ccValue;

void setupControlChange() {
  ccTime = lastMillis - 1000000;
  ccChannel = invalidChannel;
}

void processMidiControlChange(MidiMessage const * msg) {

}

void processInterfaceControlChange(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayControlChange(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Control Change 0");
  snprintf(buf1, lcdWidth + 1, "Control Change 1");
}

void processMidiMessageHistory(MidiMessage const * msg) {

}

void processInterfaceMessageHistory(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayMessageHistory(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Message History 0");
  snprintf(buf1, lcdWidth + 1, "Message History 1");
}

static uint8_t const ctrlValueOff = 0x80;
uint8_t ctrlValues[128];

void initControlValues() {
  for(size_t i = 0; i < LENGTHOF(ctrlValues); ++i) {
    ctrlValues[i] = ctrlValueOff;
  }
}

void processMidiControlValues(MidiMessage const * msg) {

}

void processInterfaceControlValues(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayControlValues(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Control Values 0");
  snprintf(buf1, lcdWidth + 1, "Control Values 1");
}

static uint8_t const noteValueOff = 0x80;
uint8_t noteValues[128];

void initNoteValues() {
  for(size_t i = 0; i < LENGTHOF(noteValues); ++i) {
    noteValues[i] = noteValueOff;
  }
}

void processMidiNoteValues(MidiMessage const * msg) {

}

void processInterfaceNoteValues(uint32_t dt, bool shiftUpPressed, bool shiftDownPressed, bool resetPressed) {
  if(shiftUpPressed) {
  }
  if(shiftDownPressed) {
  }
  if(resetPressed) {
  }
}

void generateDisplayNoteValues(char * buf0, char * buf1) {
  snprintf(buf0, lcdWidth + 1, "Note Values 0");
  snprintf(buf1, lcdWidth + 1, "Note Values 1");
}
