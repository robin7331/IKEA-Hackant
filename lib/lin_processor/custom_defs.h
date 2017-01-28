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

#ifndef CUSTOM_DEFS_H
#define CUSTOM_DEFS_H

#include "avr_util.h"

// Custom application specific parameters.
//
// Like all the other custom_* files, this file should be adapted to the specific application.
// The example provided is for a Sport Mode button press injector for 981/Cayman.
namespace custom_defs {

  // True for LIN checksum V2 (enahanced). False for LIN checksum version 1.
  const boolean kUseLinChecksumVersion2 = false;

  // LIN bus bits per second rate.
  // Supported baud range is 1000 to 20000. If out of range, using silently default
  // baud of 9600.
  const uint16 kLinSpeed = 19200;

}  // namepsace custom_defs

#endif
