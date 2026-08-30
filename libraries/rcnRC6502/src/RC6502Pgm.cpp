// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#include "RC6502Pgm.h"
#include "RC6502Utils.h"

#define SZ_CSV_BUF 64

#define CSV_NAME_FMT "yy/PGMxxx.CSV"

static char *trimWhitespace(char *str)
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
  while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || str[len - 1] == ' ' || str[len - 1] == '\t'))
  {
    str[--len] = '\0';
  }
  return str;
}

bool RC6502Pgm::begin(RC6502Sd *sd)
{
  sd_ = sd;
  return beginPgmNumber(0, 0);
}

bool RC6502Pgm::begin(RC6502Sd *sd, uint8_t dir_number, uint16_t pgm_number)
{
  sd_ = sd;
  return beginPgmNumber(dir_number, pgm_number);
}

bool RC6502Pgm::begin(RC6502Sd *sd, const char *csv_name)
{
  sd_ = sd;
  return beginCsvName(csv_name);
}

const char *RC6502Pgm::getDescription(void) const
{
  return description_;
}

RC6502Pgm::Type RC6502Pgm::getType(void) const
{
  return type_;
}

const char *RC6502Pgm::getTypeT(void) const
{
  return type_t_;
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
  size_t n;

  Serial.print(F("RCN: "));
  Serial.print(description_);
  Serial.print(F(" ("));
  Serial.print(type_t_);
  Serial.println(F(")"));

  Serial.print(F("     F> "));
  n = Serial.print(pgm_file_);
  if (sizeof(pgm_file_) > n)
  {
    RC6502Utils::printSpaces(sizeof(pgm_file_) - n);
  }

  Serial.print(F(" L> "));
  n = Serial.print(load_address_, HEX);
  if (4 > n)
  {
    RC6502Utils::printSpaces(4 - n);
  }

  Serial.print(F(" R> "));
  Serial.print(run_address_, HEX);
}

bool RC6502Pgm::openCsv(const char *csv_name)
{
  if (!sd_)
  {
    return false;
  }

  uint8_t error = sd_->open(csv_name);
  if (error == FR_NO_FILE)
  {
    return false;
  }

  if (error == FR_OK)
  {
    return true;
  }

  sd_->printError(error, RC6502Sd::Operation::Open, csv_name);
  return false;
}

bool RC6502Pgm::readCsv(char *csv, uint8_t sz_csv, const char *csv_name)
{
  if (!sd_)
  {
    return false;
  }

  uint8_t sz_read = 0;
  uint8_t error = sd_->read(csv, sz_csv, sz_read);
  if (error != FR_OK)
  {
    sd_->printError(error, RC6502Sd::Operation::Read, csv_name);
    return false;
  }

  return true;
}

void RC6502Pgm::parseCsv(char *csv)
{
  if (!csv)
  {
    return;
  }

  // Truncate at first newline to ensure clean single-line parsing
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
    token = trimWhitespace(token);
    parseToken(token, i++);
  }
}

void RC6502Pgm::parseToken(char *token, uint8_t i)
{
  if (!token)
  {
    return;
  }

  unsigned long tmp;

  switch (i)
  {
  case 0: // description_
    strncpy(description_, token, sizeof(description_) - 1);
    description_[sizeof(description_) - 1] = '\0';
    break;
  case 1: // type_
    strncpy(type_t_, token, sizeof(type_t_) - 1);
    type_t_[sizeof(type_t_) - 1] = '\0';
    if (!strcmp(token, "HEX"))
    {
      type_ = Type::Hex;
    }
    else if (!strcmp(token, "BIN"))
    {
      type_ = Type::Bin;
    }
    else
    {
      type_ = Type::Unknown;
    }
    break;
  case 2: // pgm_file_
    strncpy(pgm_file_, token, sizeof(pgm_file_) - 1);
    pgm_file_[sizeof(pgm_file_) - 1] = '\0';
    /* NOTE: Petit FatFs library can only recognize uppercase characters. */
    for (size_t idx = 0; idx < sizeof(pgm_file_) && pgm_file_[idx] != '\0'; idx++)
    {
      pgm_file_[idx] = toupper(static_cast<unsigned char>(pgm_file_[idx]));
    }
    break;
  case 3: // load_address_
    tmp = strtoul(token, nullptr, 16);
    load_address_ = (tmp <= 0xFFFFUL) ? static_cast<uint16_t>(tmp) : 0x0000;
    break;
  case 4: // run_address_
    tmp = strtoul(token, nullptr, 16);
    run_address_ = (tmp <= 0xFFFFUL) ? static_cast<uint16_t>(tmp) : 0x0000;
    break;
  }
}

bool RC6502Pgm::beginPgmNumber(uint8_t dir_number, uint16_t pgm_number)
{
  char csv_name[] = CSV_NAME_FMT;

  updateCsvName(csv_name, dir_number, pgm_number);

  return beginCsvName(csv_name);
}

bool RC6502Pgm::beginCsvName(const char *csv_name)
{
  char csv[SZ_CSV_BUF]{0};

  description_[0] = '\0';
  type_ = Type::Unknown;
  type_t_[0] = '\0';
  pgm_file_[0] = '\0';
  load_address_ = 0;
  run_address_ = 0;

  bool is_ok = openCsv(csv_name);
  if (!is_ok)
  {
    return false;
  }

  is_ok = readCsv(csv, sizeof(csv) - 1, csv_name);
  if (is_ok)
  {
    parseCsv(csv);
  }

  return is_ok;
}

void RC6502Pgm::updateCsvName(char *csv_name, uint8_t dir_number, uint16_t pgm_number)
{
  if (dir_number > 99)
  {
    dir_number = 99;
  }
  if (pgm_number > 999)
  {
    pgm_number = 999;
  }

  csv_name[0] = static_cast<char>((dir_number / 10) + '0');
  csv_name[1] = static_cast<char>((dir_number % 10) + '0');

  csv_name[6] = static_cast<char>((pgm_number / 100) + '0');
  csv_name[7] = static_cast<char>(((pgm_number % 100) / 10) + '0');
  csv_name[8] = static_cast<char>((pgm_number % 10) + '0');
}
