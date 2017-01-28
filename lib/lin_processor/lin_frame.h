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

#ifndef LIN_FRAME_H
#define LIN_FRAME_H

#include "avr_util.h"

// A buffer for a single frame.
class LinFrame {
public:
  LinFrame() {
   reset();
  }
  
  // Number of bytes in a shotest frame. This is a frame with ID byte and no
  // slave response (and thus no data or checksum).
  static const uint8 kMinBytes = 1;

  // Number of bytes in the longest frame. One ID byte, 8 data bytes, one checksum byte.
  static const uint8 kMaxBytes = 1 + 8 + 1;

  // Compute the to checkum bits [P1,P0] of the lin id in bits [5:0] and return
  // [P1,P0][5:0] which is the wire representation of this id.
  static uint8 setLinIdChecksumBits(uint8 id);
  
  boolean isValid() const;
  
  // Compute LIN frame checksum. Assuming buffer has at least one byte. A valid 
  // frame should contain one byte for id, 1-8 bytes for data, one byte for checksum.
  uint8 computeChecksum() const;

  inline void reset() {
    num_bytes_ = 0;
  }

  inline uint8 num_bytes() const {
    return num_bytes_;
  }
  
  // Get a the frame byte of given inde.
  // Byte index 0 is the frame id byte, followed by 1-8 data bytes
  // followed by 1 checksum byte.
  //
  // Caller should verify that index < num_bytes.
  inline uint8 get_byte(uint8 index) const {
    return bytes_[index];
  }
  
  // Caller should check that num_bytes < kMaxBytes;
  inline void append_byte(uint8 value) {
    bytes_[num_bytes_++] = value;
  }
  
  // TODO: make this stuff private without sacrifying performance.
  
private:
  // Number of bytes in bytes_ buffer. At most kMaxBytes.
  uint8 num_bytes_;

  // Recieved frame bytes. Includes id, data and checksum. Does not 
  // include the 0x55 sync byte.
  uint8 bytes_[kMaxBytes];
};

#endif  



