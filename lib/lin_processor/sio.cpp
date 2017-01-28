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

#include "sio.h"

#include <stdarg.h>

namespace sio {
  // TODO: do we need to set the i/o pins (PD0, PD1)? Do we rely on setting by 
  // the bootloader?
  
  // Size of output bytes queue. Shuold be <= 128 to avoid overflow.
  // TODO: reduce buffer size? Do we have enough RAM?
  // TODO: increase index size to 16 bit and increase buffer size to 200?
  static const uint8 kQueueSize = 120;
  static uint8 buffer[kQueueSize];
  // Index of the oldest entry in buffer.
  static uint8 start;
  // Number of bytes in queue.
  static uint8 count;

  // Caller need to verify that count < kQueueSize before calling this.
  static void unsafe_enqueue(byte b) {
    // kQueueSize is small enough that this will not overflow.
    uint8 next = start + count;
    if (next >= kQueueSize) {
      next -= kQueueSize;
    } 
    buffer[next] = b;  
    count++; 
  }

  // Caller need to verify that count > 1 before calling this.
  static byte unsafe_dequeue() {
    const uint8 b = buffer[start];
    if (++start >= kQueueSize) {
      start = 0;
    }
    count--;
    return b;  
  }

  void setup() {
    start = 0;
    count = 0;
    
#if F_CPU != 16000000
#error "The existing code assumes 16Mhz CPU clk."
#endif
    // For devisors see table 19-12 in the atmega328p datasheet.
    // U2X0, 16 -> 115.2k baud @ 16MHz. 
    // U2X0, 207 -> 9600 baud @ 16Mhz.
    UBRR0H = 0;
    UBRR0L = 16;
    UCSR0A = H(U2X0);
    // Enable  the transmitter. Reciever is disabled.
    UCSR0B = H(TXEN0);
    UCSR0C = H(UDORD0) | H(UCPHA0);  //(3 << UCSZ00);  
  }

  void printchar(uint8 c) {
    // If buffer is full, drop this char.
    // TODO: drop last byte to make room for the new byte?
    if (count >= kQueueSize) {
      return;
    }
    unsafe_enqueue(c);
  }

  void loop() {
    if (count && (UCSR0A & H(UDRE0))) {
      UDR0 = unsafe_dequeue();
    }
  }

  uint8 capacity() {
    return kQueueSize - count;
  }

  void waitUntilFlushed() {
    // Busy loop until all flushed to UART. 
    while (count) {
      loop();
    }  
  }

  // Assuming n is in [0, 15].
  static void printHexDigit(uint8 n) {
    if (n < 10) {
      printchar((char)('0' + n));
    } 
    else {
      printchar((char)(('a' - 10) + n));
    }    
  }

  void printhex2(uint8 b) {
    printHexDigit(b >> 4);
    printHexDigit(b & 0xf);
  }

  void println() {
    printchar('\n');
  }

  void print(const __FlashStringHelper *str) {
    const char* PROGMEM p = (const char PROGMEM *)str;
    for(;;) {
      const unsigned char c = pgm_read_byte(p++);
      if (!c) {
        return;
      }
      printchar(c);
    }
  }

  void println(const __FlashStringHelper *str) {
    print(str);
    println();
  }


  void print(const char* str) {
    for(;;) {
      const char c = *(str++);
      if (!c) {
        return;
      }
      printchar(c);
    } 
  }

  void println(const char* str) {
    print(str);
    println();
  }


  void printf(const __FlashStringHelper *format, ...)
  {
    // Assuming single thread, using static buffer.
    static char buf[80];
    va_list ap;
    va_start(ap, format);
    vsnprintf_P(buf, sizeof(buf), (const char *)format, ap); // progmem for AVR
    for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
    {
      printchar(*p);
    }
    va_end(ap);
  }
}  // namespace sio




