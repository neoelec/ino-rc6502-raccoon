// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>

#include <Arduino.h>

#include "RC6502Pio.h"
#include "RC6502Menu.h"
#include "RC6502Utils.h"

constexpr uint8_t KEY_CODE_RS = 0x12; // Ctrl+R

RC6502PioClass RC6502Pio;

bool RC6502PioClass::isClassicMode(void)
{
  return analogRead(PIN_PIO_MODE) <= 512;
}

void RC6502PioClass::begin(Mode mode)
{
  MCUSR = 0;
  wdt_disable();

  if (mode == Mode::Classic || (mode == Mode::Auto && isClassicMode()))
  {
    beginClassic();
    return;
  }

  dev_.begin();
  beginCommon();
  RC6502Menu.begin(dev_);
  state_ = State::Keyboard;

  printBanner();
}

void RC6502PioClass::beginClassic(void)
{
  MCUSR = 0;
  wdt_disable();

  dev_.beginNoSd();
  beginCommon();
  state_ = State::Classic;

  printClassicBanner();
}

void RC6502PioClass::printBanner(void)
{
  Serial.print(F("\033[2J\033[H"));
  Serial.println(F("RC6502 Apple 1 Replica - CFFA-1 Mode"));
  Serial.println(F("  - Ctrl+R - CFFA-1 Menu"));
}

void RC6502PioClass::printClassicBanner(void)
{
  Serial.print(F("\033[2J\033[H"));
  Serial.println(F("RC6502 Apple 1 Replica - Classic Mode"));
}

void RC6502PioClass::run(void)
{
  switch (state_)
  {
  case State::Classic:
    handleStateClassic();
    break;
  case State::Keyboard:
    handleStateKeyboard();
    break;
  case State::MenuEnter:
    handleStateMenuEnter();
    break;
  case State::MenuRun:
    handleStateMenuRun();
    break;
  }
}

void RC6502PioClass::beginCommon(void)
{
  clk_src_ = dev_.getClock();
  kbd_ = dev_.getKbd();
  video_ = dev_.getVideo();

  if (clk_src_)
  {
    clk_src_->enable();
    clk_src_->reset();
  }
}

void RC6502PioClass::handleStateClassic(void)
{
  if (!kbd_ || !video_)
  {
    return;
  }

  while (Serial.available() && !kbd_->isBufferFull())
  {
    kbd_->pushToBuffer(Serial.read());
  }

  kbd_->run();
  video_->run();
}

void RC6502PioClass::handleStateKeyboard(void)
{
  if (!kbd_ || !video_)
  {
    return;
  }

  while (Serial.available() && !kbd_->isBufferFull())
  {
    int c = Serial.read();

    if (c == KEY_CODE_RS)
    {
      state_ = State::MenuEnter;
      break;
    }

    kbd_->pushToBuffer(c);
  }

  kbd_->run();
  video_->run();
}

void RC6502PioClass::handleStateMenuEnter(void)
{
  RC6502Menu.enter();
  RC6502Utils::flushTtyRx();
  state_ = State::MenuRun;
}

void RC6502PioClass::handleStateMenuRun(void)
{
  if (!RC6502Menu.run())
  {
    RC6502Utils::flushTtyRx();
    state_ = State::Keyboard;
  }

  if (video_)
  {
    video_->run();
  }
}
