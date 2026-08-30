// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#include "RC6502Pgm.h"
#include "RC6502Utils.h"

#define SZ_LINE_BUF 64

static bool isAllDigits(const char *str)
{
  if (!str || *str == '\0')
  {
    return false;
  }
  while (*str)
  {
    if (!isdigit(static_cast<unsigned char>(*str)))
    {
      return false;
    }
    str++;
  }
  return true;
}

static bool containsIgnoreCase(const char *haystack, const char *needle)
{
  if (!haystack || !needle)
  {
    return false;
  }
  if (*needle == '\0')
  {
    return true;
  }
  size_t needle_len = strlen(needle);
  size_t haystack_len = strlen(haystack);
  if (needle_len > haystack_len)
  {
    return false;
  }
  for (size_t i = 0; i <= haystack_len - needle_len; i++)
  {
    if (strncasecmp(&haystack[i], needle, needle_len) == 0)
    {
      return true;
    }
  }
  return false;
}

void RC6502Pgm::buildCatalogPath(char *out_path, size_t max_len, const char *prefix)
{
  if (!out_path || max_len == 0)
  {
    return;
  }

  out_path[0] = '\0';

  if (!prefix || prefix[0] == '\0' || strcmp(prefix, "/") == 0)
  {
    strncpy_P(out_path, PSTR("CATALOG.CSV"), max_len - 1);
    out_path[max_len - 1] = '\0';
  }
  else
  {
    if (prefix[0] == '/')
    {
      prefix++;
    }
    strncpy(out_path, prefix, max_len - 1);
    out_path[max_len - 1] = '\0';

    size_t plen = strlen(out_path);
    if (plen > 0 && out_path[plen - 1] == '/')
    {
      strncat_P(out_path, PSTR("CATALOG.CSV"), max_len - plen - 1);
    }
    else
    {
      strncat_P(out_path, PSTR("/CATALOG.CSV"), max_len - plen - 1);
    }
  }

  for (size_t i = 0; out_path[i] != '\0'; i++)
  {
    out_path[i] = static_cast<char>(toupper(static_cast<unsigned char>(out_path[i])));
  }
}

bool RC6502Pgm::readLine(RC6502Sd *sd, char *buf, uint8_t max_len)
{
  if (!sd || !buf || max_len == 0)
  {
    return false;
  }

  uint8_t len = 0;
  buf[0] = '\0';
  bool overflow = false;

  while (true)
  {
    uint8_t ch = 0;
    uint8_t sz_read = 0;
    uint8_t err = sd->read(&ch, 1, sz_read);
    if (err != FR_OK || sz_read == 0)
    {
      break;
    }

    if (ch == '\r' || ch == '\n')
    {
      if (len > 0 || overflow)
      {
        break;
      }
      continue;
    }

    if (len < max_len - 1)
    {
      buf[len++] = static_cast<char>(ch);
    }
    else
    {
      // Line longer than buffer: discard remaining characters on this line
      overflow = true;
    }
  }

  buf[len] = '\0';
  return (len > 0);
}

void RC6502Pgm::reset(void)
{
  description_[0] = '\0';
  type_ = Type::Unknown;
  pgm_file_[0] = '\0';
  load_address_ = 0;
  run_address_ = 0;
}

bool RC6502Pgm::begin(RC6502Sd *sd)
{
  return begin(sd, "SYSTEM", 0);
}

bool RC6502Pgm::begin(RC6502Sd *sd, uint8_t dir_number, uint16_t index)
{
  if (dir_number == 0)
  {
    return begin(sd, "SYSTEM", index);
  }
  else if (dir_number == 1)
  {
    return begin(sd, "GAMES", index);
  }
  else if (dir_number == 2)
  {
    return begin(sd, "EXTBAS", index);
  }

  char pfx[8]{0};
  pfx[0] = '0' + ((dir_number / 10) % 10);
  pfx[1] = '0' + (dir_number % 10);
  pfx[2] = '\0';
  return begin(sd, pfx, index);
}

bool RC6502Pgm::begin(RC6502Sd *sd, const char *prefix, uint16_t index)
{
  sd_ = sd;
  reset();

  if (!sd_)
  {
    return false;
  }

  char cat_path[32]{0};
  buildCatalogPath(cat_path, sizeof(cat_path), prefix);

  uint8_t err = sd_->open(cat_path);
  if (err != FR_OK)
  {
    return false;
  }

  char line[SZ_LINE_BUF]{0};
  uint16_t cur_idx = 0;

  while (readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }

    if (cur_idx == index)
    {
      parseCsv(trimmed);
      return true;
    }
    cur_idx++;
  }

  return false;
}

bool RC6502Pgm::find(RC6502Sd *sd, uint8_t dir_number, const char *query)
{
  if (dir_number == 0)
  {
    return find(sd, "SYSTEM", query);
  }
  else if (dir_number == 1)
  {
    return find(sd, "GAMES", query);
  }
  else if (dir_number == 2)
  {
    return find(sd, "EXTBAS", query);
  }

  char pfx[8]{0};
  pfx[0] = '0' + ((dir_number / 10) % 10);
  pfx[1] = '0' + (dir_number % 10);
  pfx[2] = '\0';
  return find(sd, pfx, query);
}

bool RC6502Pgm::find(RC6502Sd *sd, const char *prefix, const char *query)
{
  if (!sd || !query || *query == '\0')
  {
    return false;
  }

  sd_ = sd;
  reset();

  bool is_digits = isAllDigits(query);
  long target_idx = is_digits ? static_cast<long>(RC6502Utils::parseDec16(query)) : -1;

  char cat_path[32]{0};
  buildCatalogPath(cat_path, sizeof(cat_path), prefix);

  uint8_t err = sd_->open(cat_path);
  if (err != FR_OK)
  {
    return false;
  }

  char line[SZ_LINE_BUF]{0};
  uint16_t cur_idx = 0;
  bool found_match = false;

  while (readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }

    if (is_digits)
    {
      if (target_idx >= 0 && cur_idx == static_cast<uint16_t>(target_idx))
      {
        parseCsv(trimmed);
        found_match = true;
        break;
      }
    }
    else
    {
      parseCsv(trimmed);
      if (containsIgnoreCase(description_, query) || containsIgnoreCase(pgm_file_, query))
      {
        found_match = true;
        break;
      }
    }

    cur_idx++;
  }

  return found_match;
}

const char *RC6502Pgm::getDescription(void) const
{
  return description_;
}

RC6502Pgm::Type RC6502Pgm::getType(void) const
{
  return type_;
}

const __FlashStringHelper *RC6502Pgm::getTypeT(void) const
{
  switch (type_)
  {
  case Type::Hex:
    return F("HEX");
  case Type::Bin:
    return F("BIN");
  case Type::Bas:
    return F("BAS");
  case Type::Dir:
    return F("DIR");
  default:
    return F("???");
  }
}

const char *RC6502Pgm::getPgmFile(void) const
{
  return pgm_file_;
}

uint16_t RC6502Pgm::getLoadAddress(void) const
{
  return load_address_;
}

uint16_t RC6502Pgm::getRunAddress(void) const
{
  return run_address_;
}

void RC6502Pgm::printProgram(void) const
{
  Serial.print(F("NAME: "));
  Serial.print(description_);
  Serial.print(F(" ("));
  Serial.print(getTypeT());
  Serial.println(F(")"));

  Serial.print(F("FILE: "));
  Serial.println(pgm_file_);

  Serial.print(F("ADDR: LOAD $"));
  if (load_address_ < 0x1000) Serial.print('0');
  if (load_address_ < 0x0100) Serial.print('0');
  if (load_address_ < 0x0010) Serial.print('0');
  Serial.print(load_address_, HEX);

  Serial.print(F(" | RUN $"));
  if (run_address_ < 0x1000) Serial.print('0');
  if (run_address_ < 0x0100) Serial.print('0');
  if (run_address_ < 0x0010) Serial.print('0');
  Serial.print(run_address_, HEX);
  Serial.println();
}

void RC6502Pgm::parseCsv(char *csv)
{
  reset();

  if (!csv)
  {
    return;
  }

  char *newline = strpbrk(csv, "\r\n");
  if (newline)
  {
    *newline = '\0';
  }

  static const char *delim = ",";
  char *ptr = csv;
  char *token;
  uint8_t i = 0;

  while ((token = strsep(&ptr, delim)) != nullptr)
  {
    token = RC6502Utils::trim(token);
    parseToken(token, i++);
  }
}

void RC6502Pgm::parseToken(char *token, uint8_t i)
{
  if (!token)
  {
    return;
  }

  switch (i)
  {
  case 0: // description_
    strncpy(description_, token, sizeof(description_) - 1);
    description_[sizeof(description_) - 1] = '\0';
    break;
  case 1: // type_
    if (!strcasecmp_P(token, PSTR("HEX")))
    {
      type_ = Type::Hex;
    }
    else if (!strcasecmp_P(token, PSTR("BIN")))
    {
      type_ = Type::Bin;
    }
    else if (!strncasecmp_P(token, PSTR("BAS"), 3))
    {
      type_ = Type::Bas;
    }
    else if (!strcasecmp_P(token, PSTR("DIR")))
    {
      type_ = Type::Dir;
    }
    else
    {
      type_ = Type::Unknown;
    }
    break;
  case 2: // pgm_file_
    strncpy(pgm_file_, token, sizeof(pgm_file_) - 1);
    pgm_file_[sizeof(pgm_file_) - 1] = '\0';
    for (size_t idx = 0; idx < sizeof(pgm_file_) && pgm_file_[idx] != '\0'; idx++)
    {
      pgm_file_[idx] = static_cast<char>(toupper(static_cast<unsigned char>(pgm_file_[idx])));
    }
    break;
  case 3: // load_address_
    load_address_ = RC6502Utils::parseHex16(token);
    break;
  case 4: // run_address_
    run_address_ = RC6502Utils::parseHex16(token);
    break;
  }
}
