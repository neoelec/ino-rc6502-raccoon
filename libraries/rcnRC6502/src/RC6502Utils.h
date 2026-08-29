// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_UTILS_H
#define RCN_RC6502_UTILS_H

#include <Arduino.h>

namespace RC6502Utils
{
  template <typename T, size_t N>
  constexpr size_t arraySize(const T (&)[N]) noexcept
  {
    return N;
  }

  inline void flushTtyRx(void)
  {
    while (Serial.available() > 0)
    {
      Serial.read();
    }
  }

  inline void printSpaces(size_t n)
  {
    for (size_t i = 0; i < n; i++)
    {
      Serial.print(F(" "));
    }
  }
}

#endif // RCN_RC6502_UTILS_H
