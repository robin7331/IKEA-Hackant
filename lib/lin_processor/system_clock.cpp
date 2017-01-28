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

#include "system_clock.h"

#include <arduino.h>
#include "avr_util.h"
#include "hardware_clock.h"

namespace system_clock {
  static const uint16 kTicksPerMilli = hardware_clock::kTicksPerMilli;
  static const uint16 kTicksPer10Millis = 10 * kTicksPerMilli;

  static uint16 accounted_ticks = 0;
  static uint32 time_millis = 0;

  void loop() {
    const uint16 current_ticks = hardware_clock::ticksForNonIsr();

    // This 16 bit unsigned arithmetic works well also in case of a timer overflow.
    // Assuming at least two loops per timer cycle.
    uint16  delta_ticks = current_ticks - accounted_ticks;

    // A course increment loop in case we have a large update interval. Improves
    // runtime over the single milli update loop below.
    while (delta_ticks >= kTicksPer10Millis) {
      delta_ticks -= kTicksPer10Millis;
      accounted_ticks += kTicksPer10Millis;
      time_millis += 10; 
    }

    // Single millis update loop.
    while (delta_ticks >= kTicksPerMilli) {
      delta_ticks -= kTicksPerMilli;
      accounted_ticks += kTicksPerMilli;
      time_millis++; 
    }
  }

  uint32 timeMillis() {
    return time_millis;
  }

}  // namespace system_clock



