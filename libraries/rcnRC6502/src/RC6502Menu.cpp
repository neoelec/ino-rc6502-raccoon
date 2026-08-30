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
  state_ = State::Command;
  input_len_ = 0;
  input_buf_[0] = '\0';

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

  char c = static_cast<char>(Serial.read());

  if (state_ == State::Command)
  {
    handleCommand(c);
  }
  else
  {
    handleInputPrompt(c);
  }

  return true;
}

void RC6502MenuClass::handleCommand(char c)
{
  char cmd = static_cast<char>(tolower(c));

  // Handle Enter (CR or LF)
  if (cmd == '\r' || cmd == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }
    printPrompt();
    return;
  }

  Serial.println(cmd);

  switch (cmd)
  {
  case 's':
    startPrompt(State::PromptDirNumber, F("DIRECTORY NUMBER [0 <= dir <= 99]"));
    break;
  case 'l':
    startPrompt(State::PromptPageNumber, F("PAGE NUMBER [>=0]"));
    break;
  case 'o':
    startPrompt(State::PromptPgmNumber, F("PROGRAM NUMBER [0 <= pgm <= 999]"));
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
}

void RC6502MenuClass::startPrompt(State next_state, const __FlashStringHelper *prompt_msg)
{
  state_ = next_state;
  input_len_ = 0;
  input_buf_[0] = '\0';

  Serial.println();
  Serial.print(F("RCN: "));
  Serial.print(prompt_msg);
  printPrompt();
}

void RC6502MenuClass::handleInputPrompt(char c)
{
  if (c == 0x1B) // Escape key
  {
    Serial.println();
    state_ = State::Command;
    input_len_ = 0;
    input_buf_[0] = '\0';
    printPrompt();
    return;
  }

  if (c == '\b' || c == 0x7F) // Backspace / Delete
  {
    if (input_len_ > 0)
    {
      input_len_--;
      input_buf_[input_len_] = '\0';
      Serial.print(F("\b \b"));
    }
    return;
  }

  if (c == '\r' || c == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }

    if (input_len_ == 0)
    {
      state_ = State::Command;
      printPrompt();
      return;
    }

    input_buf_[input_len_] = '\0';
    processPromptSubmit();
    return;
  }

  if (isdigit(static_cast<unsigned char>(c)) && input_len_ < sizeof(input_buf_) - 1)
  {
    input_buf_[input_len_++] = c;
    input_buf_[input_len_] = '\0';
    Serial.write(c);
  }
}

void RC6502MenuClass::processPromptSubmit(void)
{
  long val = atol(input_buf_);
  State current_state = state_;
  state_ = State::Command;
  input_len_ = 0;
  input_buf_[0] = '\0';

  switch (current_state)
  {
  case State::PromptDirNumber:
    if (val < 0 || val > MAX_DIR_NUMBER)
    {
      printWrongValue(val, 0, MAX_DIR_NUMBER);
      return;
    }
    dir_number_ = static_cast<uint8_t>(val);
    Serial.println();
    Serial.print(F("RCN: DIRECTORY - "));
    Serial.println(dir_number_, DEC);
    Serial.println();
    printPrompt();
    break;

  case State::PromptPageNumber:
    if (val < 0 || val > 9999)
    {
      printWrongValue(val, 0, 9999);
      return;
    }
    listPrograms(static_cast<uint16_t>(val), DEFAULT_PGM_PER_PAGE);
    printPrompt();
    break;

  case State::PromptPgmNumber:
    if (val < 0 || val > MAX_PGM_NUMBER)
    {
      printWrongValue(val, 0, MAX_PGM_NUMBER);
      return;
    }
    if (!pgm_.begin(sd_, dir_number_, static_cast<uint16_t>(val)))
    {
      Serial.println();
      Serial.println(F("RCN: Failed to find program!"));
      printPrompt();
      return;
    }
    {
      RC6502Loader loader;
      loader.load(sd_, kbd_, video_, pgm_);
    }
    printPrompt();
    break;

  default:
    printPrompt();
    break;
  }
}

void RC6502MenuClass::printWrongValue(long val, long min_val, long max_val)
{
  Serial.println();
  Serial.print(F("Wrong Value "));
  Serial.print(val, DEC);
  Serial.print(F(". It should be "));
  Serial.print(min_val, DEC);
  Serial.print(F(" <= value <= "));
  Serial.println(max_val, DEC);
  printPrompt();
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
