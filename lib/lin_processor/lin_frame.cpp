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

#include "lin_frame.h"

#include "custom_defs.h"

// Compute the checksum of the frame. For now using LIN checksum V2 only.
uint8 LinFrame::computeChecksum() const {
  // LIN V2 checksum includes the ID byte, V1 does not.
  const uint8 startByteIndex = custom_defs::kUseLinChecksumVersion2 ? 0 : 1;
  const uint8* p = &bytes_[startByteIndex];
  
  // Exclude the checksum byte at the end of the frame.
  uint8 numBytesToChecksum = num_bytes_ - startByteIndex - 1;

  // Sum bytes. We should not have 16 bit overflow here since the frame has a limited size.
  uint16 sum = 0;
  while (numBytesToChecksum-- > 0) {
    sum += *(p++);
  }

  // Keep adding the high and low bytes until no carry.
  for (;;) {
    const uint8 highByte = (uint8)(sum >> 8);
    if (!highByte) {
      break;  
    }
    // NOTE: this can add additional carry.  
    sum = (sum & 0xff) + highByte; 
  }

  return (uint8)(~sum);
}


uint8 LinFrame::setLinIdChecksumBits(uint8 id) {
  // The algorithm is optimized for CPU time (avoiding individual shifts per id
  // bit). Using registers for the two checksum bits. P1 is computed in bit 7 of
  // p1_at_b7 and p0 is comptuted in bit 6 of p0_at_b6.
  uint8 p1_at_b7 = ~0;
  uint8 p0_at_b6 = 0;

  // P1: id5, P0: id4
  uint8 shifter = id << 2;
  p1_at_b7 ^= shifter;
  p0_at_b6 ^= shifter;

  // P1: id4, P0: id3
  shifter += shifter;
  p1_at_b7 ^= shifter;

  // P1: id3, P0: id2
  shifter += shifter;
  p1_at_b7 ^= shifter;
  p0_at_b6 ^= shifter;

  // P1: id2, P0: id1
  shifter += shifter;
  p0_at_b6 ^= shifter;

  // P1: id1, P0: id0
  shifter += shifter;
  p1_at_b7 ^= shifter;
  p0_at_b6 ^= shifter;

  return (p1_at_b7 & 0b10000000) | (p0_at_b6 & 0b01000000) | (id & 0b00111111);
}

boolean LinFrame::isValid() const {
  const uint8 n = num_bytes_;

  // Check frame size.
  // One ID byte with optional 1-8 data bytes and 1 checksum byte.
  // TODO: should we enforce only 1, 2, 4, or 8 data bytes?  (total size 
  // 1, 3, 4, 6, or 10)
  //
  // TODO: should we pass through frames with ID only (n == 1, no response from slave).
  if (n != 1 && (n < 3 || n > 10)) {
    return false;
  }

  // Check ID byte checksum bits.
  const uint8 id_byte = bytes_[0];
  if (id_byte != setLinIdChecksumBits(id_byte)) {
    return false;
  }

  // If not an ID only frame, check also the overall checksum.
  if (n > 1) {
    if (bytes_[n - 1] != computeChecksum()) {
      return false;
    }  
  }
  // TODO: check protected id.
  return true;
}

