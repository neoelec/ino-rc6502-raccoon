// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Dev.h"
#include "RC6502Pins.h"

static RC6502Kbd * volatile s_kbd = nullptr;     // pointer for interrupt service routine
static RC6502Video * volatile s_video = nullptr; // pointer for interrupt service routine

static void ISR_RC6502KbdSetInterrupt(void)
{
  if (s_kbd)
  {
    s_kbd->setInterrupt();
  }
}

static void ISR_RC6502VideoSetInterrupt(void)
{
  if (s_video)
  {
    s_video->setInterrupt();
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
  initVideo();

  clock_.begin();
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

  // Trigger on RISING edge (6821 PIA KBD Read pulse rising edge)
  attachInterrupt(digitalPinToInterrupt(PIN_KBD_CLR), ISR_RC6502KbdSetInterrupt, RISING);
  EIFR = _BV(INTF0); // Clear any pending flag generated during configuration
}

void RC6502Dev::initVideo(void)
{
  video_.begin(&mcp_);
  s_video = &video_;

  // Trigger on RISING edge (6821 PIA Video DA pulse rising edge)
  attachInterrupt(digitalPinToInterrupt(PIN_VIDEO_DA), ISR_RC6502VideoSetInterrupt, RISING);
  EIFR = _BV(INTF1); // Clear any pending flag generated during configuration
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
}
