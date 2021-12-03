#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "numbers.h"

// AlphaNumBuffer allows a way to pass a string to StrCat without having to do
// memory allocation.  It is simply a pair of a fixed-size character array, and
// a size.  Please don't use outside of absl, yet.
template <size_t max_size>
struct AlphaNumBuffer {
  std::array<char, max_size> data;
  size_t size;
};

// Helper function for the future StrCat default floating-point format, %.6g
// This is fast.
inline AlphaNumBuffer<
    kSixDigitsToBufferSize>
SixDigits(double d) {
  AlphaNumBuffer<kSixDigitsToBufferSize>
      result;
  result.size = SixDigitsToBuffer(d, &result.data[0]);
  return result;
}


