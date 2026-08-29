// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Dev.h"
#include "RC6502Pins.h"

static RC6502Kbd *s_kbd = nullptr; // pointer for interrupt service routine

static void ISR_RC6502KbdSetInterrupt(void)
{
  if (s_kbd)
  {
    s_kbd->setInterrupt();
  }
}

void RC6502Dev::begin(void)
{
  beginNoSd();
  initSd();
}

void RC6502Dev::beginNoSd(void)
{
  initPin();
  initTty();
  initMcp();
  initKbd();

  clock_.begin();
  video_.begin(&mcp_);
}

RC6502Clock *RC6502Dev::getClock(void)
{
  return &clock_;
}

RC6502Kbd *RC6502Dev::getKbd(void)
{
  return &kbd_;
}

Adafruit_MCP23X17 *RC6502Dev::getMcp(void)
{
  return &mcp_;
}

RC6502Sd *RC6502Dev::getSd(void)
{
  return &sd_;
}

RC6502Video *RC6502Dev::getVideo(void)
{
  return &video_;
}

void RC6502Dev::initKbd(void)
{
  kbd_.begin(&mcp_);
  s_kbd = &kbd_;

  // Clear any pending INT0 interrupt flag before attaching
  EIFR = _BV(INTF0);
  attachInterrupt(digitalPinToInterrupt(PIN_KBD_CLR), ISR_RC6502KbdSetInterrupt, CHANGE);
}

void RC6502Dev::initMcp(void)
{
  mcp_.begin_SPI(PIN_MCP23S17_nSS);

  for (int pin = 0; pin < 16; pin++)
  {
    mcp_.pinMode(pin, INPUT_PULLUP);
  }
}

void RC6502Dev::initPin(void)
{
  pinMode(PIN_MCP23S17_nSS, OUTPUT);
  digitalWrite(PIN_MCP23S17_nSS, HIGH);

  pinMode(PIN_SD_nSS, OUTPUT);
  digitalWrite(PIN_SD_nSS, HIGH);
}

void RC6502Dev::initSd(void)
{
  sd_.begin(PIN_SD_nSS);
}

void RC6502Dev::initTty(void)
{
  Serial.begin(115200);

  // Clear screen and move cursor to home (0, 0)
  Serial.print(F("\033[2J\033[H"));
  Serial.println(F("RC6502 Apple 1 Replica - Raccoon's Mod"));
}
