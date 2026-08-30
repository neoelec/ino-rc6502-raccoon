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

  inline uint16_t parseHex16(const char *str)
  {
    if (!str) return 0;
    while (*str == ' ' || *str == '$') str++;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
    uint16_t val = 0;
    while (*str)
    {
      char c = *str++;
      uint8_t d = 0;
      if (c >= '0' && c <= '9') d = c - '0';
      else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
      else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
      else break;
      val = (val << 4) | d;
    }
    return val;
  }

  inline uint16_t parseDec16(const char *str)
  {
    if (!str) return 0;
    while (*str == ' ' || *str == '\t') str++;
    uint16_t val = 0;
    while (*str >= '0' && *str <= '9')
    {
      val = val * 10 + (*str++ - '0');
    }
    return val;
  }
}

#endif // RCN_RC6502_UTILS_H
