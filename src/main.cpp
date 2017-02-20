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

uint16_t currentTarget = 0;
uint8_t initializedTarget = false;
uint8_t targetThreshold = 0;
uint8_t currentTableMovement = 0;

uint16_t minPosition = 530;
uint16_t maxPosition = 5500;

const int moveTableUpPin = PD4;
const int moveTableDownPin = PD7;

const int moveUpButton = 3;
const int moveM2Button = 6;
const int moveM1Button = 5;
const int moveDownButton = 8;

int pressedButton = 0;
int lastPressedButton = 0;
unsigned long lastPressed = 0;
uint8_t doOnce = false;


void printValues() {
  Serial.println("======= VALUES =======");
  Serial.print("Memory 1 is at: ");
  Serial.println(memOne);
  Serial.print("Memory 2 is at: ");
  Serial.println(memTwo);
  Serial.print("Threshold is: ");
  Serial.println(targetThreshold);
  Serial.print("Current Position: ");
  Serial.println(lastPosition);
  Serial.println("======================");
}

void printHelp() {
  Serial.println("======= Serial Commands =======");
  Serial.println("Send 'STOP' to stop");
  Serial.println("Send 'HELP' to show this view");
  Serial.println("Send 'VALUES' to show the current values");
  Serial.println("Send 'T123' to set the threshold to 123 (255 max!)");
  Serial.println("Send 'M1' to move to position stored in memory 1");
  Serial.println("Send 'M2' to move to position stored in memory 2");
  Serial.println("Send 'S1' to store current position in memory 1");
  Serial.println("Send 'S2' to store current position in memory 2");
  Serial.println("Send 'M15000' to move set the mem 1 position to 5000");
  Serial.println("Send 'M25000' to move set the mem 2 position to 5000");
  Serial.println("Send '1580' to move to position 1580.");
  Serial.println("===============================");
}

void storeM1(uint16_t value) {
  if (value > 150 && value < 6400) {
    memOne = value;
    Serial.print("New Memory 1: ");
    Serial.println(value);
    EEPROM.put(1, value);
  } else {
    Serial.println("Not stored. Keep your value between 150 and 6400");
  }
}

void storeM2(uint16_t value) {
  if (value > 150 && value < 6400) {
    memTwo = value;
    Serial.print("New Memory 2: ");
    Serial.println(value);
    EEPROM.put(3, value);
  } else {
    Serial.println("Not stored. Keep your value between 150 and 6400");
  }
}

void storeThreshold(uint8_t value) {
  if (value > 50 && value < 254) {
    targetThreshold = value;
    Serial.print("New Threshold: ");
    Serial.println(value);
    EEPROM.put(0, value);
  } else {
    Serial.println("Not stored. Keep your value between 50 and 254");
  }
}


// direction == 0 => Table stops
// direction == 1 => Table goes upwards
// direction == 2 => Table goes downwards
//
void moveTable(uint8_t direction) {
  if (direction != currentTableMovement) {
    currentTableMovement = direction;
    if (direction == 0) {
      Serial.println("Table stops");
      digitalWrite(moveTableUpPin, HIGH);
      digitalWrite(moveTableDownPin, HIGH);
    } else if (direction == 1) {
      Serial.println("Table goes up");
      digitalWrite(moveTableDownPin, HIGH);
      digitalWrite(moveTableUpPin, LOW);
    } else {
      Serial.println("Table goes down");
      digitalWrite(moveTableUpPin, HIGH);
      digitalWrite(moveTableDownPin, LOW);
    }
  }
}

// direction == 0 => Table is levelled
// direction == 1 => Target is above table
// direction == 2 => Target is below table
uint8_t desiredTableDirection() {

  int distance = lastPosition - currentTarget;
  uint16_t absDistance = abs(distance);

  if (absDistance > targetThreshold) {
    if (distance <= 0) { // table has to move up
      return 1;
    }
    return 2;
  }
  return 0;

}


void processLINFrame(LinFrame frame) {
  // Get the first byte which is the LIN ID
  uint8_t id = frame.get_byte(0);

  // 0x92 is the ID of the LIN node that sends the table position
  if (id == 0x92) {

    // the table position is a two byte value. LSB is sent first.
    uint8_t varA = frame.get_byte(2); //1st byte of the value (LSB)
    uint8_t varB = frame.get_byte(1); //2nd byte (MSB)
    uint16_t temp = 0;

    temp = varA;
    temp <<= 8;
    temp = temp | varB;

    if (temp != lastPosition) {
      lastPosition = temp;
      String myString = String(temp);
      char buffer[5];
      myString.toCharArray(buffer, 5);
      Serial.print("Current Position: ");
      Serial.println(buffer);

      if (initializedTarget == false) {
        currentTarget = temp;
        initializedTarget = true;
      }
    }

  }
}

void readButtons() {

  if (!digitalRead(moveUpButton)) {

    pressedButton = moveUpButton;
    if (lastPressedButton != pressedButton) {
      Serial.println("Button UP Pressed");
      lastPressedButton = pressedButton;
    }
    return;
  }

  if (!digitalRead(moveM1Button)) {
    pressedButton = moveM1Button;
    if (lastPressedButton != pressedButton) {
      Serial.println("Button M1 Pressed");
      lastPressedButton = pressedButton;

    }
    return;
  }

  if (!digitalRead(moveM2Button)) {
    pressedButton = moveM2Button;
    if (lastPressedButton != pressedButton) {
      Serial.println("Button M2 Pressed");
      lastPressedButton = pressedButton;

    }
    return;
  }

  if (!digitalRead(moveDownButton)) {
    pressedButton = moveDownButton;
    if (lastPressedButton != pressedButton) {
      Serial.println("Button DN Pressed");
      lastPressedButton = pressedButton;
    }
    return;
  }


  pressedButton = 0;

}

void loopButtons() {

  if (pressedButton != 0) {

    if (lastPressedButton == moveUpButton) {
      moveTable(1);
      currentTarget = lastPosition + (targetThreshold * 2);
    } else if (lastPressedButton == moveDownButton) {
      moveTable(2);
      currentTarget = lastPosition - (targetThreshold * 2);
    } else {
      if (doOnce == false) {
        lastPressed = millis();
        doOnce = true;
      }
    }

  } else {

    if (doOnce) {

      unsigned int pressDuration = millis() - lastPressed;

      if (pressDuration > 0 && pressDuration < 1000) { // short press

        Serial.print("Button pressed for ");
        Serial.print(pressDuration);
        Serial.println(" ms.");

        if (lastPressedButton == moveM1Button) {
          currentTarget = memOne;
        } else if (lastPressedButton == moveM2Button) {
          currentTarget = memTwo;
        }

      } else if (pressDuration >= 1000) {

        Serial.print("Button pressed for ");
        Serial.print(pressDuration);
        Serial.println(" ms.");

        if (lastPressedButton == moveM1Button) {
          storeM1(lastPosition);
        } else if (lastPressedButton == moveM2Button) {
          storeM2(lastPosition);
        }

      }

    }


    doOnce = false;
  }

}


void setup() {


  Serial.begin(115200);
  while (!Serial) {;};

  Serial.println("IKEA Hackant v1.0");
  Serial.println("Type 'HELP' to display all commands.");

  pinMode(moveTableUpPin, OUTPUT);
  pinMode(moveTableDownPin, OUTPUT);

  pinMode(moveUpButton, INPUT_PULLUP);
  pinMode(moveDownButton, INPUT_PULLUP);
  pinMode(moveM1Button, INPUT_PULLUP);
  pinMode(moveM2Button, INPUT_PULLUP);

  Serial.print("moveUpButton ");
  Serial.println(moveUpButton);
  Serial.print("moveDownButton ");
  Serial.println(moveDownButton);
  Serial.print("moveM1Button ");
  Serial.println(moveM1Button);
  Serial.print("moveM2Button ");
  Serial.println(moveM2Button);

  digitalWrite(moveTableUpPin, HIGH);
  digitalWrite(moveTableDownPin, HIGH);

  // setup everything that the LIN library needs.
  hardware_clock::setup();
  lin_processor::setup();

  // Enable global interrupts.
  sei();




  EEPROM.get(0, targetThreshold);
  if (targetThreshold == 255) {
    storeThreshold(120);
  }

  EEPROM.get(1, memOne);
  if (memOne == 65535) {
    storeM1(3500);
  }


  EEPROM.get(3, memTwo);
  if (memTwo == 65535) {
    storeM2(3500);
  }

  printValues();

}



void loop() {


  // Periodic updates.
  system_clock::loop();


  // Handle recieved LIN frames.
  LinFrame frame;

  // if there is a LIN frame
  if (lin_processor::readNextFrame(&frame)) {
    processLINFrame(frame);
  }

  // direction == 0 => Table is levelled
  // direction == 1 => Target is above table
  // direction == 2 => Target is below table
  uint8_t direction = desiredTableDirection();

  if( currentTarget > minPosition && currentTarget < maxPosition ){
    moveTable(direction);
  } else {
    // Stop table
    moveTable(0);
    Serial.println("Target exceeds limit! No Movement!");
  }


  if (Serial.available() > 0) {

    // read the incoming byte:
    String val = Serial.readString();

    if (val.indexOf("HELP") != -1 || val.indexOf("help") != -1) {
      printHelp();
    } else if (val.indexOf("VALUES") != -1 || val.indexOf("values") != -1) {
      printValues();
    } else if (val.indexOf("STOP") != -1 || val.indexOf("stop") != -1) {

      if (direction == 1)
        currentTarget = lastPosition + (targetThreshold * 2);
      else if (direction == 2)
        currentTarget = lastPosition - (targetThreshold * 2);

      Serial.print("STOP at ");
      Serial.println(currentTarget);


    } else if (val.indexOf('T') != -1 || val.indexOf("t") != -1) {
      uint8_t threshold = (uint8_t)val.substring(1).toInt();
      storeThreshold(threshold);

    } else if (val.indexOf("M1") != -1 || val.indexOf("m1") != -1) {

      if (val.length() == 2) {
        currentTarget = memOne;
      } else {
        storeM1(val.substring(2).toInt());
      }


    } else if (val.indexOf("M2") != -1 || val.indexOf("m2") != -1) {

      if (val.length() == 2) {
        currentTarget = memTwo;
      } else {
        storeM2(val.substring(2).toInt());
      }

    } else if (val.indexOf("S1") != -1 || val.indexOf("s1") != -1) {

      storeM1(lastPosition);

    } else if (val.indexOf("S2") != -1 || val.indexOf("s2") != -1) {

      storeM2(lastPosition);

    } else {
      if (val.toInt() > 150 && val.toInt() < 6400) {
        Serial.print("New Target ");
        Serial.println(val);
        currentTarget = val.toInt();
      } else {
        Serial.println("Not stored. Keep your value between 150 and 6400");
      }
    }
  }

  readButtons();
  loopButtons();

}
