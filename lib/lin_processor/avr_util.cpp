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

#include "avr_util.h"

namespace avr_util_private {

// Using lookup to avoid individual bit shifting. This is faster
// than (1 << n) or a switch/case statement.
const byte kBitMaskArray[] = {
  H(0),
  H(1),
  H(2),
  H(3),
  H(4),
  H(5),
  H(6),
  H(7),
};

}  // namespace avr_util_private


