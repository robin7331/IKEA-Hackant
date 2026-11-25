// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h>
#include "avr_util.h"
#include "custom_defs.h"
#include "hardware_clock.h"
#include "io_pins.h"
#include "lin_processor.h"
#include "system_clock.h"
#include <EEPROM.h>

uint16_t lastPosition = 0;
uint16_t memOne = 0;
uint16_t memTwo = 0;
uint16_t memThree = 0;
// Initial memory 3 position if the EEPROM is still empty (a comfortable
// standing height on my desk; memory 1/2 default to 3500).
const uint16_t kDefaultMemThree = 1926;

uint16_t currentTarget = 0;
uint8_t initializedTarget = false;
uint8_t targetThreshold = 0;
uint8_t currentTableMovement = 0;

// After the motor is switched off the desk coasts ~150 position units and then
// springs back ~40, so it ends ~114 units past the position at which it was
// stopped (measured on a BEKANT: 111-117, both directions). The stop happens
// at the first position report with remaining distance <= kOverrun; reports
// come every ~35 units, so kOverrun = 114 + 35/2 centres the end position
// error (about -20..+15).
const uint16_t kOverrun = 130;
// After a stop, wait until the position has been stable this long before an
// automatic correction move is started (the desk is still coasting before).
const unsigned long kSettleMs = 500;
unsigned long lastPositionChange = 0;

// Target relative to the current position, clamped to the uint16_t range.
uint16_t targetFromPosition(long offset)
{
  long value = (long)lastPosition + offset;
  if (value < 0)
  {
    return 0;
  }
  if (value > 65535L)
  {
    return 65535;
  }
  return (uint16_t)value;
}

const int moveTableUpPin = PD4;
const int moveTableDownPin = PD7;

const int moveUpButton = PD3;
const int moveM2Button = PD6;
const int moveM1Button = PD5;
const int moveDownButton = 8;

// Button bit masks. All four buttons are read at once, so M1 + M2 pressed
// together can be detected (memory 3).
const uint8_t kBtnUp = 1;
const uint8_t kBtnDown = 2;
const uint8_t kBtnM1 = 4;
const uint8_t kBtnM2 = 8;
const uint8_t kBtnMem = kBtnM1 | kBtnM2;

// Press duration from which a memory button press stores instead of moves.
const unsigned long kLongPressMs = 1000;
// Memory buttons must be released this long before the press is evaluated
// (contact bounce, M1 and M2 not released at exactly the same time). Also
// used to suppress repeated "Button ... Pressed" messages while bouncing.
const unsigned long kReleaseMs = 50;

uint8_t buttons = 0;     // currently pressed buttons
uint8_t lastButtons = 0; // buttons of the previous loop (for messages)
// Last time each button (bit index 0..3) was seen pressed (for messages).
unsigned long buttonLastPressed[4] = {0, 0, 0, 0};
uint8_t manualDirection = 0; // 1/2 while Up/Down is held
uint8_t memSeen = 0;         // memory buttons pressed during this press
unsigned long memPressStart = 0;
unsigned long memLastPressed = 0;
uint8_t memBothHeld = false; // M1 and M2 currently held together
unsigned long memBothSince = 0;
unsigned long memBothMs = 0; // longest time M1 + M2 were held together

void printValues()
{
  Serial.println(F("======= VALUES ======="));
  Serial.print(F("Memory 1 is at: "));
  Serial.println(memOne);
  Serial.print(F("Memory 2 is at: "));
  Serial.println(memTwo);
  Serial.print(F("Memory 3 is at: "));
  Serial.println(memThree);
  Serial.print(F("Threshold is: "));
  Serial.println(targetThreshold);
  Serial.print(F("Current Position: "));
  Serial.println(lastPosition);
  Serial.println(F("======================"));
}

void printHelp()
{
  Serial.println(F("======= Serial Commands ======="));
  Serial.println(F("Send 'STOP' to stop"));
  Serial.println(F("Send 'HELP' to show this view"));
  Serial.println(F("Send 'VALUES' to show the current values"));
  Serial.println(F("Send 'T123' to set the threshold to 123 (255 max!)"));
  Serial.println(F("Send 'M1' to move to position stored in memory 1"));
  Serial.println(F("Send 'M2' to move to position stored in memory 2"));
  Serial.println(F("Send 'M3' to move to position stored in memory 3"));
  Serial.println(F("Send 'S1' to store current position in memory 1"));
  Serial.println(F("Send 'S2' to store current position in memory 2"));
  Serial.println(F("Send 'S3' to store current position in memory 3"));
  Serial.println(F("Send 'M15000' to move set the mem 1 position to 5000"));
  Serial.println(F("Send 'M25000' to move set the mem 2 position to 5000"));
  Serial.println(F("Send 'M35000' to move set the mem 3 position to 5000"));
  Serial.println(F("Buttons: M1+M2 short = memory 3, hold 1 s = store memory 3"));
  Serial.println(F("Send '1580' to move to position 1580."));
  Serial.println(F("==============================="));
}

void storeM1(uint16_t value)
{
  if (value > 150 && value < 6400)
  {
    memOne = value;
    Serial.print(F("New Memory 1: "));
    Serial.println(value);
    EEPROM.put(1, value);
  }
  else
  {
    Serial.println(F("Not stored. Keep your value between 150 and 6400"));
  }
}

void storeM2(uint16_t value)
{
  if (value > 150 && value < 6400)
  {
    memTwo = value;
    Serial.print(F("New Memory 2: "));
    Serial.println(value);
    EEPROM.put(3, value);
  }
  else
  {
    Serial.println(F("Not stored. Keep your value between 150 and 6400"));
  }
}

void storeM3(uint16_t value)
{
  if (value > 150 && value < 6400)
  {
    memThree = value;
    Serial.print(F("New Memory 3: "));
    Serial.println(value);
    EEPROM.put(7, value); // addresses 5-6 hold currentTarget
  }
  else
  {
    Serial.println(F("Not stored. Keep your value between 150 and 6400"));
  }
}

void storeThreshold(uint8_t value)
{
  if (value > 50 && value < 254)
  {
    targetThreshold = value;
    Serial.print(F("New Threshold: "));
    Serial.println(value);
    EEPROM.put(0, value);
  }
  else
  {
    Serial.println(F("Not stored. Keep your value between 50 and 254"));
  }
}

// Add a helper to store currentTarget
void storeCurrentTarget(uint16_t value)
{
  currentTarget = value;
  EEPROM.put(5, value); // Use address 5 (next free after memTwo)
}

// direction == 0 => Table stops
// direction == 1 => Table goes upwards
// direction == 2 => Table goes downwards
//
void moveTable(uint8_t direction)
{
  if (direction != currentTableMovement)
  {
    currentTableMovement = direction;
    if (direction == 0)
    {
      Serial.println(F("Table stops"));
      digitalWrite(moveTableUpPin, HIGH);
      digitalWrite(moveTableDownPin, HIGH);
    }
    else if (direction == 1)
    {
      Serial.println(F("Table goes up"));
      digitalWrite(moveTableDownPin, HIGH);
      digitalWrite(moveTableUpPin, LOW);
    }
    else
    {
      Serial.println(F("Table goes down"));
      digitalWrite(moveTableUpPin, HIGH);
      digitalWrite(moveTableDownPin, LOW);
    }
  }
}

// direction == 0 => Table is levelled
// direction == 1 => Target is above table
// direction == 2 => Target is below table
//
// While moving: keep moving only while the remaining distance is larger than
// kOverrun, so the table coasts to the target after the stop.
// While standing: start only after the position has settled and only if the
// distance is larger than the threshold and larger than kOverrun (a shorter
// move would be stopped again immediately).
uint8_t desiredTableDirection()
{
  long distance = (long)currentTarget - (long)lastPosition; // > 0: target above

  if (currentTableMovement == 1)
  {
    return (distance > (long)kOverrun) ? 1 : 0;
  }
  if (currentTableMovement == 2)
  {
    return (-distance > (long)kOverrun) ? 2 : 0;
  }

  if (millis() - lastPositionChange < kSettleMs)
  {
    return 0;
  }

  long startDistance = max((long)targetThreshold, (long)kOverrun);
  if (distance > startDistance)
  {
    return 1;
  }
  if (-distance > startDistance)
  {
    return 2;
  }
  return 0;
}

void processLINFrame(LinFrame frame)
{
  // Get the first byte which is the LIN ID
  uint8_t id = frame.get_byte(0);

  // 0x92 is the ID of the LIN node that sends the table position
  if (id == 0x92)
  {

    // the table position is a two byte value. LSB is sent first.
    uint8_t varA = frame.get_byte(2); // 1st byte of the value (LSB)
    uint8_t varB = frame.get_byte(1); // 2nd byte (MSB)
    uint16_t temp = 0;

    temp = varA;
    temp <<= 8;
    temp = temp | varB;

    if (temp != lastPosition)
    {
      lastPosition = temp;
      lastPositionChange = millis();
      Serial.print(F("Current Position: "));
      Serial.println(temp);

      if (initializedTarget == false)
      {
        storeCurrentTarget(temp);
        initializedTarget = true;
      }
    }
  }
}

// Reads all four buttons (active HIGH) into the bit mask `buttons` and
// reports newly pressed buttons on the serial port.
void readButtons()
{
  buttons = 0;
  if (digitalRead(moveUpButton) == HIGH)
  {
    buttons |= kBtnUp;
  }
  if (digitalRead(moveDownButton) == HIGH)
  {
    buttons |= kBtnDown;
  }
  if (digitalRead(moveM1Button) == HIGH)
  {
    buttons |= kBtnM1;
  }
  if (digitalRead(moveM2Button) == HIGH)
  {
    buttons |= kBtnM2;
  }

  // Report a press only if the button was released for at least kReleaseMs
  // before, so contact bounce does not produce repeated messages.
  unsigned long now = millis();
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t bit = 1 << i;
    if (!(buttons & bit))
    {
      continue;
    }
    if (!(lastButtons & bit) && now - buttonLastPressed[i] >= kReleaseMs)
    {
      switch (bit)
      {
      case kBtnUp:
        Serial.println(F("Button UP Pressed"));
        break;
      case kBtnDown:
        Serial.println(F("Button DN Pressed"));
        break;
      case kBtnM1:
        Serial.println(F("Button M1 Pressed"));
        break;
      default:
        Serial.println(F("Button M2 Pressed"));
        break;
      }
    }
    buttonLastPressed[i] = now;
  }
  lastButtons = buttons;
}

// Up/Down: move while held (only one of them pressed), stop on release.
void loopMoveButtons()
{
  uint8_t moveButtons = buttons & (kBtnUp | kBtnDown);

  // Keep the target far enough ahead so the table keeps moving while held.
  if (moveButtons == kBtnUp)
  {
    moveTable(1);
    storeCurrentTarget(targetFromPosition(2L * kOverrun));
    manualDirection = 1;
  }
  else if (moveButtons == kBtnDown)
  {
    moveTable(2);
    storeCurrentTarget(targetFromPosition(-2L * kOverrun));
    manualDirection = 2;
  }
  else if (manualDirection != 0)
  {
    // Released: target = expected end position after coasting. The
    // remaining distance is exactly kOverrun, so the table stops now.
    if (manualDirection == 1)
    {
      storeCurrentTarget(targetFromPosition(kOverrun));
    }
    else
    {
      storeCurrentTarget(targetFromPosition(-(long)kOverrun));
    }
    manualDirection = 0;
  }
}

void printPressDuration(const __FlashStringHelper *name, unsigned long ms)
{
  Serial.print(F("Button "));
  Serial.print(name);
  Serial.print(F(" pressed for "));
  Serial.print(ms);
  Serial.println(F(" ms."));
}

// Memory buttons, evaluated after all memory buttons were released:
//   M1 or M2 alone:  short = move to memory, >= 1 s = store current position
//   M1 + M2:         short = move to memory 3, both held >= 1 s = store
// M1 and M2 do not have to be pressed or released at exactly the same time.
void loopMemoryButtons()
{
  unsigned long now = millis();
  uint8_t memButtons = buttons & kBtnMem;

  if (memButtons != 0)
  {
    if (memSeen == 0)
    {
      memPressStart = now;
      memBothMs = 0;
    }
    memSeen |= memButtons;
    memLastPressed = now;

    if (memButtons == kBtnMem)
    {
      if (!memBothHeld)
      {
        memBothHeld = true;
        memBothSince = now;
      }
      if (now - memBothSince > memBothMs)
      {
        memBothMs = now - memBothSince;
      }
    }
    else
    {
      memBothHeld = false;
    }
    return;
  }

  memBothHeld = false;
  if (memSeen == 0 || now - memLastPressed < kReleaseMs)
  {
    return;
  }

  if (memSeen == kBtnMem)
  {
    printPressDuration(F("M1+M2"), memBothMs);
    if (memBothMs >= kLongPressMs)
    {
      storeM3(lastPosition);
    }
    else
    {
      storeCurrentTarget(memThree);
    }
  }
  else
  {
    unsigned long duration = memLastPressed - memPressStart;
    if (memSeen == kBtnM1)
    {
      printPressDuration(F("M1"), duration);
      if (duration >= kLongPressMs)
      {
        storeM1(lastPosition);
      }
      else
      {
        storeCurrentTarget(memOne);
      }
    }
    else
    {
      printPressDuration(F("M2"), duration);
      if (duration >= kLongPressMs)
      {
        storeM2(lastPosition);
      }
      else
      {
        storeCurrentTarget(memTwo);
      }
    }
  }

  memSeen = 0;
}

void setup()
{

  Serial.begin(115200);
  while (!Serial)
  {
    ;
  };

  Serial.println(F("IKEA Hackant v1.0"));
  Serial.println(F("Type 'HELP' to display all commands."));

  pinMode(moveTableUpPin, OUTPUT);
  pinMode(moveTableDownPin, OUTPUT);

  pinMode(moveUpButton, INPUT);
  pinMode(moveDownButton, INPUT);
  pinMode(moveM1Button, INPUT);
  pinMode(moveM2Button, INPUT);

  Serial.print(F("moveUpButton "));
  Serial.println(moveUpButton);
  Serial.print(F("moveDownButton "));
  Serial.println(moveDownButton);
  Serial.print(F("moveM1Button "));
  Serial.println(moveM1Button);
  Serial.print(F("moveM2Button "));
  Serial.println(moveM2Button);

  digitalWrite(moveTableUpPin, HIGH);
  digitalWrite(moveTableDownPin, HIGH);

  // setup everything that the LIN library needs.
  hardware_clock::setup();
  lin_processor::setup();

  // Enable global interrupts.
  sei();

  EEPROM.get(0, targetThreshold);
  if (targetThreshold == 255)
  {
    storeThreshold(120);
  }

  EEPROM.get(1, memOne);
  if (memOne == 65535)
  {
    storeM1(3500);
  }

  EEPROM.get(3, memTwo);
  if (memTwo == 65535)
  {
    storeM2(3500);
  }

  EEPROM.get(7, memThree);
  if (memThree == 65535)
  {
    storeM3(kDefaultMemThree);
  }

  EEPROM.get(5, currentTarget);
  // Optionally validate currentTarget here

  printValues();
}

void loop()
{

  // Periodic updates.
  system_clock::loop();

  // Handle recieved LIN frames.
  LinFrame frame;

  // if there is a LIN frame
  if (lin_processor::readNextFrame(&frame))
  {
    processLINFrame(frame);
  }

  // direction == 0 => Table is levelled
  // direction == 1 => Target is above table
  // direction == 2 => Target is below table
  uint8_t direction = desiredTableDirection();
  moveTable(direction);

  if (Serial.available() > 0)
  {

    // read the incoming byte:
    String val = Serial.readString();

    if (val.indexOf("HELP") != -1 || val.indexOf("help") != -1)
    {
      printHelp();
    }
    else if (val.indexOf("VALUES") != -1 || val.indexOf("values") != -1)
    {
      printValues();
    }
    else if (val.indexOf("STOP") != -1 || val.indexOf("stop") != -1)
    {
      if (direction == 1)
        storeCurrentTarget(targetFromPosition(kOverrun));
      else if (direction == 2)
        storeCurrentTarget(targetFromPosition(-(long)kOverrun));
      Serial.print(F("STOP at "));
      Serial.println(currentTarget);
    }
    else if (val.indexOf('T') != -1 || val.indexOf("t") != -1)
    {
      uint8_t threshold = (uint8_t)val.substring(1).toInt();
      storeThreshold(threshold);
    }
    else if (val.indexOf("M1") != -1 || val.indexOf("m1") != -1)
    {
      if (val.length() == 2)
      {
        storeCurrentTarget(memOne);
      }
      else
      {
        storeM1(val.substring(2).toInt());
      }
    }
    else if (val.indexOf("M2") != -1 || val.indexOf("m2") != -1)
    {
      if (val.length() == 2)
      {
        storeCurrentTarget(memTwo);
      }
      else
      {
        storeM2(val.substring(2).toInt());
      }
    }
    else if (val.indexOf("M3") != -1 || val.indexOf("m3") != -1)
    {
      if (val.length() == 2)
      {
        storeCurrentTarget(memThree);
      }
      else
      {
        storeM3(val.substring(2).toInt());
      }
    }
    else if (val.indexOf("S1") != -1 || val.indexOf("s1") != -1)
    {

      storeM1(lastPosition);
    }
    else if (val.indexOf("S2") != -1 || val.indexOf("s2") != -1)
    {

      storeM2(lastPosition);
    }
    else if (val.indexOf("S3") != -1 || val.indexOf("s3") != -1)
    {
      storeM3(lastPosition);
    }
    else
    {
      if (val.toInt() > 150 && val.toInt() < 6400)
      {
        Serial.print(F("New Target "));
        Serial.println(val);
        storeCurrentTarget(val.toInt());
      }
      else
      {
        Serial.println(F("Not stored. Keep your value between 150 and 6400"));
      }
    }
  }

  readButtons();
  loopMoveButtons();
  loopMemoryButtons();
}
