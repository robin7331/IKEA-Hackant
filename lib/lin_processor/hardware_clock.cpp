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

#include "hardware_clock.h"

#include <arduino.h>
#include "avr_util.h"

namespace hardware_clock {

#if F_CPU != 16000000
#error "The existing code assumes 16Mhz CPU clk."
#endif

void setup() {
    // Normal mode (free running [0, ffff]).
    TCCR1A = L(COM1A1) | L(COM1A0) | L(COM1B1) | L(COM1B0) | L(WGM11) | L(WGM10);
    // Prescaler: X64 (250 clocks per ms @ 16MHz). 2^16 clock cycle every ~260ms.
    // This also defines the max update() interval to avoid missing a counter overflow. 
    TCCR1B = L(ICNC1) | L(ICES1) | L(WGM13) | L(WGM12) | L(CS12) | H(CS11) | H(CS10);
    // Clear counter.
    TCNT1 = 0;
    // Compare A. Not used.
    OCR1A = 0;
    // Compare B. Used to output cycle pulses, for debugging.
    OCR1B = 0;
    // Diabled interrupts.
    TIMSK1 = L(ICIE1) | L(OCIE1B) | L(OCIE1A) | L(TOIE1);
    TIFR1 = L(ICF1) | L(OCF1B) | L(OCF1A) | L(TOV1);     
  }
  
}  // namespace hardware_clock



