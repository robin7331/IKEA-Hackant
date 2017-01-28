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

#ifndef AVR_UTIL_H
#define AVR_UTIL_H

#include <arduino.h>

// Get rid of the _t type suffix.
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;

// Bit index to bit mask.
// AVR registers bit indices are defined in iom328p.h.
#define H(x) (1 << (x))
#define L(x) (0 << (x))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Private data. Do not use from other modules.
namespace avr_util_private {
  extern const byte kBitMaskArray[];
}

// Similar to (1 << bit_index) but more efficient for non consts. For
// const masks use H(n). Undefined result if it_index not in [0, 7].
inline byte bitMask(byte bit_index) {
  return *(avr_util_private::kBitMaskArray + bit_index);
}

#endif

