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

#ifndef SYS_CLOCK_H
#define SYS_CLOCK_H

#include <arduino.h>
#include "hardware_clock.h"

// Uses the hardware clock to provide a 32 bit milliseconds time since program start.
// The 32 milliseconds time has about 54 days cycle time.
namespace system_clock {
  // Call once per main loop(). Updates the internal millis clock based on the hardware
  // clock. A calling interval of larger than 280ms will result in loosing time due
  // to hardware clock overflow.
  extern void loop();

  // Return time of last update() in millis since program start. Returns zero if update() was
  // never called. 
  extern uint32 timeMillis();
 
}  // namespace system_clock

#endif  



