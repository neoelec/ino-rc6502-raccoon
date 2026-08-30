// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>
#include <ctype.h>
#include <stdlib.h>

#include <Arduino.h>

#include "RC6502Menu.h"
#include "RC6502Utils.h"

constexpr uint8_t DEFAULT_PGM_PER_PAGE = 20;
constexpr uint8_t MAX_DIR_NUMBER       = 99;
constexpr uint16_t MAX_PGM_NUMBER      = 999;

RC6502MenuClass RC6502Menu;

void RC6502MenuClass::begin(RC6502Dev &dev)
{
  clock_ = dev.getClock();
  kbd_ = dev.getKbd();
  sd_ = dev.getSd();
  video_ = dev.getVideo();
}

void RC6502MenuClass::enter(void)
{
  Serial.println();
  Serial.print(F("RCN: Entering menu ..."));

  done_ = false;
  dir_number_ = 0;

  showMenu();
  printPrompt();
}

bool RC6502MenuClass::run(void)
{
  if (isDone())
  {
    return false;
  }

  if (Serial.available() <= 0)
  {
    return true;
  }

  char c = static_cast<char>(tolower(Serial.read()));

  // Handle Enter (CR or LF)
  if (c == '\r' || c == '\n')
  {
    printPrompt();
    return true;
  }

  Serial.println(c);

  switch (c)
  {
  case 's':
    doCmdSelectDirectory();
    break;
  case 'l':
    doCmdListPrograms();
    break;
  case 'o':
    doCmdLoadProgram();
    break;
  case 'x':
    doCmdExit();
    break;
  case 'p':
    doCmdPIOReset();
    break;
  case 'w':
    doCmdWarmReset();
    break;
  case '?':
    doCmdHelp();
    break;
  default:
    Serial.println(F("RCN: Unknown command. Press '?' for help."));
    printPrompt();
    break;
  }

  return true;
}

void RC6502MenuClass::showMenu(void)
{
  Serial.println();
  Serial.println();
  Serial.println(F("s - Select Directory"));
  Serial.println(F("l - List Programs"));
  Serial.println(F("o - Load Program"));
  Serial.println(F("x - Exit"));
  Serial.println(F("p - PIO Reset"));
  Serial.println(F("w - Warm Reset"));
  Serial.println(F("? - Help"));
}

void RC6502MenuClass::printPrompt(void)
{
  Serial.println();
  Serial.print(F("->"));
}

bool RC6502MenuClass::readNumber(const __FlashStringHelper *prompt, long &result, long min_val, long max_val)
{
  Serial.println();
  Serial.print(F("RCN: "));
  Serial.print(prompt);
  printPrompt();

  char buf[8]{0};
  uint8_t len = 0;

  while (true)
  {
    if (Serial.available() <= 0)
    {
      continue;
    }

    char c = static_cast<char>(Serial.read());

    if (c == '\r' || c == '\n')
    {
      if (len == 0)
      {
        printPrompt();
        return false;
      }
      buf[len] = '\0';
      break;
    }

    if (c == 0x1B) // Escape key
    {
      Serial.println();
      printPrompt();
      return false;
    }

    if (c == '\b' || c == 0x7F) // Backspace / Delete
    {
      if (len > 0)
      {
        len--;
        Serial.print(F("\b \b"));
      }
      continue;
    }

    if (isdigit(static_cast<unsigned char>(c)) && len < sizeof(buf) - 1)
    {
      buf[len++] = c;
      Serial.write(c);
    }
  }

  // Drain trailing line feeds if CRLF
  delay(2);
  if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
  {
    Serial.read();
  }

  long val = atol(buf);
  if (val < min_val || val > max_val)
  {
    Serial.println();
    Serial.print(F("Wrong Value "));
    Serial.print(val, DEC);
    Serial.print(F(". It should be "));
    Serial.print(min_val, DEC);
    Serial.print(F(" <= value <= "));
    Serial.println(max_val, DEC);
    printPrompt();
    return false;
  }

  result = val;
  return true;
}

void RC6502MenuClass::doCmdHelp(void)
{
  showMenu();
  printPrompt();
}

void RC6502MenuClass::doCmdExit(void)
{
  Serial.println();
  Serial.println(F("RCN: Exiting from menu ..."));
  Serial.println();

  done_ = true;
}

void RC6502MenuClass::doCmdListPrograms(void)
{
  long page_number = 0;
  if (!readNumber(F("PAGE NUMBER [>=0]"), page_number, 0, 9999))
  {
    return;
  }

  listPrograms(static_cast<uint16_t>(page_number), DEFAULT_PGM_PER_PAGE);
  printPrompt();
}

void RC6502MenuClass::doCmdLoadProgram(void)
{
  long pgm_number = 0;
  if (!readNumber(F("PROGRAM NUMBER [0 <= pgm <= 999]"), pgm_number, 0, MAX_PGM_NUMBER))
  {
    return;
  }

  if (!pgm_.begin(sd_, dir_number_, static_cast<uint16_t>(pgm_number)))
  {
    Serial.println();
    Serial.println(F("RCN: Failed to find program!"));
    printPrompt();
    return;
  }

  RC6502Loader loader;
  loader.load(sd_, kbd_, video_, pgm_);
  printPrompt();
}

void RC6502MenuClass::doCmdSelectDirectory(void)
{
  long dir_number = 0;
  if (!readNumber(F("DIRECTORY NUMBER [0 <= dir <= 99]"), dir_number, 0, MAX_DIR_NUMBER))
  {
    return;
  }

  dir_number_ = static_cast<uint8_t>(dir_number);
  Serial.println();
  Serial.print(F("RCN: DIRECTORY - "));
  Serial.println(dir_number_, DEC);
  Serial.println();

  printPrompt();
}

void RC6502MenuClass::doCmdPIOReset(void)
{
  Serial.println();
  Serial.println(F("RCN: PIO reset ..."));
  Serial.flush();
  wdt_enable(WDTO_15MS);
  while (true)
  {
    // Wait for watchdog reset
  }
}

void RC6502MenuClass::doCmdWarmReset(void)
{
  Serial.println();
  Serial.println(F("RCN: Warm reset ..."));
  if (clock_)
  {
    clock_->reset();
  }
  if (kbd_)
  {
    kbd_->reset();
  }
  if (video_)
  {
    video_->reset();
  }
  Serial.flush();
  done_ = true;
}

bool RC6502MenuClass::isDone(void) const
{
  return done_;
}

void RC6502MenuClass::listPrograms(uint16_t page_number, uint16_t pgm_per_page)
{
  RC6502Pgm pgm_tmp;
  uint16_t pgm_begin = page_number * pgm_per_page;
  Serial.println();
  size_t n;

  for (uint16_t i = 0; i < pgm_per_page; i++)
  {
    uint16_t pgm_number = i + pgm_begin;

    if (!pgm_tmp.begin(sd_, dir_number_, pgm_number))
    {
      break;
    }

    n = Serial.print(pgm_number, DEC);
    Serial.print(F("."));
    if (4 > n)
    {
      RC6502Utils::printSpaces(4 - n);
    }

    n = Serial.print(pgm_tmp.getDescription());
    if (26 > n)
    {
      RC6502Utils::printSpaces(26 - n);
    }
    Serial.print(F(" ("));
    Serial.print(pgm_tmp.getTypeT());
    Serial.println(F(")"));
  }

  Serial.println();
}
