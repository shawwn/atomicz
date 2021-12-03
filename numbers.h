#pragma once

static const int kFastToBufferSize = 32;
static const int kSixDigitsToBufferSize = 16;

// Helper function for fast formatting of floating-point values.
// The result is the same as printf's "%g", a.k.a. "%.6g"; that is, six
// significant digits are returned, trailing zeros are removed, and numbers
// outside the range 0.0001-999999 are output using scientific notation
// (1.23456e+06). This routine is heavily optimized.
// Required buffer size is `kSixDigitsToBufferSize`.
size_t SixDigitsToBuffer(double d, char* buffer);

