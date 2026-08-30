// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_UTILS_H
#define RCN_RC6502_UTILS_H

#include <string.h>
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
    while (n--)
    {
      Serial.write(' ');
    }
  }

  inline char *trim(char *str)
  {
    if (!str)
    {
      return nullptr;
    }
    while (*str && (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n'))
    {
      str++;
    }
    if (*str == '\0')
    {
      return str;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n'))
    {
      str[--len] = '\0';
    }
    return str;
  }
}

#endif // RCN_RC6502_UTILS_H
