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

#ifndef PASSIVE_TIMER_H
#define PASSIVE_TIMER_H

#include <arduino.h>
#include "system_clock.h"

// A wrapper around system clock that provides millisecond based time measurments.
class PassiveTimer {
public:
  PassiveTimer() {
    restart();
  }

  inline void restart() {
    start_time_millis_ = system_clock::timeMillis();
  }

  void copy(const PassiveTimer &other) {
    start_time_millis_ = other.start_time_millis_;
  }

  inline uint32 timeMillis() const {
    return system_clock::timeMillis() - start_time_millis_;
  }

private:
  uint32 start_time_millis_;
};

#endif


