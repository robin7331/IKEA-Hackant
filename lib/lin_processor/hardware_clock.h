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

#ifndef HARDWARE_CLOCK_H
#define HARDWARE_CLOCK_H

#include <arduino.h>
#include "avr_util.h"

// Provides a free running 16 bit counter with 250 ticks per millisecond and 
// about 280 millis cycle time. Assuming 16Mhz clock.
//
// USES: timer 1, no interrupts.
namespace hardware_clock {
  // Call once from main setup(). Tick count starts at 0.
  extern void setup();

  // Free running 16 bit counter. Starts counting from zero and wraps around
  // every ~280ms.
  // Assumes interrupts are enabled upon entry.
  // DO NOT CALL THIS FROM AN ISR.
  inline uint16 ticksForNonIsr() {
    // We disable interrupt to avoid corruption of the AVR temp byte buffer
    // that is used to read 16 bit values.
    // TODO: can we avoid disabling interrupts (motivation: improve LIN ISR jitter).
    cli();
    const uint16 result TCNT1;
    sei();
    return result;
  }

  // Similar to ticksNonIsr but does not enable interrupts.
  // CALL THIS FROM ISR ONLY.
  inline uint16 ticksForIsr() {
    return TCNT1; 
  }

#if F_CPU != 16000000
#error "The existing code assumes 16Mhz CPU clk."
#endif

  // @ 16Mhz / x64 prescaler. Number of ticks per a millisecond.
  const uint32 kTicksPerMilli = 250;
}  // namespace hardware_clock

#endif  



