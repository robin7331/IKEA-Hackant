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

#ifndef SIO_H
#define SIO_H

#include <arduino.h>
#include "avr_util.h"

// A serial output that uses hardware UART0 and no interrupts (for lower
// interrupt jitter). Requires periodic calls to update() to send buffered
// bytes to the uart. 
//
// TX Output - TXD (PD1) - pin 31
// TX Input  - TXD (PD0) - pin 30 (currently not used).
namespace sio {
  
  // Call from main setup() and loop() respectivly.
  extern void setup();
  extern void loop();
  
  // Momentary size of free space in the output buffer. Sending at most this number
  // of characters will not loose any byte.
  extern uint8 capacity(); 
  
  extern void printchar(uint8 b);
  extern void print(const __FlashStringHelper *str);
  extern void println(const __FlashStringHelper *str);
  extern void print(const char* str);
  extern void println(const char* str);
  extern void println();
  extern void printf(const __FlashStringHelper *format, ...);
  extern void printhex2(uint8 b); 
 
  // Wait in a busy loop until all bytes were flushed to the UART. 
  // Avoid using this when possible. Useful when needing to print
  // during setup() more than the output buffer can contain.
  void waitUntilFlushed(); 
}  // namespace sio

#endif  


