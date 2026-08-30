// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Sd.h"
#include "RC6502Utils.h"

bool RC6502Sd::begin(uint8_t pin_ss)
{
  pin_ss_ = pin_ss;
  FRESULT error = FR_DISK_ERR;
  uint8_t attempts = 5;

  do
  {
    error = PetitFatFs.begin(&fatfs_, pin_ss_);
    delay(2);
  } while (--attempts && (error != FR_OK));

  if (error != FR_OK)
  {
    printError(static_cast<uint8_t>(error), Operation::Mount);
    return false;
  }

  return true;
}

uint8_t RC6502Sd::mount(void)
{
  FRESULT error = PetitFatFs.begin(&fatfs_, pin_ss_);
  return static_cast<uint8_t>(error);
}

uint8_t RC6502Sd::open(const char *file_name)
{
  FRESULT error = PetitFatFs.open(file_name);
  return static_cast<uint8_t>(error);
}

void RC6502Sd::printError(uint8_t error, Operation operation, const char *file_name)
{
  printErrorCode(static_cast<FRESULT>(error));
  printOperation(operation);

  if (file_name)
  {
    Serial.print(F(" - File : "));
    Serial.print(file_name);
  }

  Serial.println();
}

void RC6502Sd::waitKey(void)
{
  RC6502Utils::flushTtyRx();

  Serial.println(F("Check SD and press a key to repeat"));
  Serial.println();

  waitTtyRx();
  RC6502Utils::flushTtyRx();
}

void RC6502Sd::printErrorCode(FRESULT error)
{
  Serial.print(F("SD error "));
  Serial.print(static_cast<unsigned int>(error));
  Serial.print(F(" ("));
  switch (error)
  {
  case FR_OK:
    Serial.print(F("OK"));
    break;
  case FR_DISK_ERR:
    Serial.print(F("DISK_ERR"));
    break;
  case FR_NOT_READY:
    Serial.print(F("NOT_READY"));
    break;
  case FR_NO_FILE:
    Serial.print(F("NO_FILE"));
    break;
  case FR_NOT_OPENED:
    Serial.print(F("NOT_OPENED"));
    break;
  case FR_NOT_ENABLED:
    Serial.print(F("NOT_ENABLED"));
    break;
  case FR_NO_FILESYSTEM:
    Serial.print(F("NO_FILESYSTEM"));
    break;
  default:
    Serial.print(F("UNKNOWN"));
    break;
  }
  Serial.print(F(")"));
}

void RC6502Sd::printOperation(Operation operation)
{
  Serial.print(F(" on "));

  switch (operation)
  {
  case Operation::Mount:
    Serial.print(F("MOUNT"));
    break;
  case Operation::Open:
    Serial.print(F("OPEN"));
    break;
  case Operation::Read:
    Serial.print(F("READ"));
    break;
  case Operation::Write:
    Serial.print(F("WRITE"));
    break;
  case Operation::Seek:
    Serial.print(F("SEEK"));
    break;
  default:
    Serial.print(F("UNKNOWN"));
    break;
  }
}

void RC6502Sd::waitTtyRx(void)
{
  while (Serial.available() < 1)
  {
    // wait for character input
  }
}
