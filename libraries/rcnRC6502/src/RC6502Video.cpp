// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Video.h"
#include "RC6502Pins.h"

void RC6502Video::begin(Adafruit_MCP23X17 *mcp)
{
  mcp_ = mcp;

  initMcp();
  initPin();
}

void RC6502Video::reset(void)
{
  interrupt_ = false;
  initMcp();
  initPin();
}

void RC6502Video::setInterrupt(void)
{
  interrupt_ = true;
}

void RC6502Video::run(void)
{
  digitalWrite(PIN_VIDEO_nRDA, HIGH);

  if (interrupt_ || digitalRead(PIN_VIDEO_DA) == HIGH)
  {
    interrupt_ = false;

    if (mcp_)
    {
      int c = mcp_->readGPIOA() & 0x7F;
      putChar(c);
    }

    // Acknowledge read by asserting nRDA LOW (Non-blocking)
    digitalWrite(PIN_VIDEO_nRDA, LOW);
  }
}

void RC6502Video::initMcp(void)
{
  if (!mcp_)
  {
    return;
  }

  for (uint8_t pin = PIN_VIDEO_D0; pin <= PIN_VIDEO_D6; pin++)
  {
    mcp_->pinMode(pin, INPUT);
  }
}

void RC6502Video::initPin(void)
{
  pinMode(PIN_VIDEO_DA, INPUT);

  pinMode(PIN_VIDEO_nRDA, OUTPUT);
  digitalWrite(PIN_VIDEO_nRDA, HIGH);
}

inline void RC6502Video::putChar(int c)
{
  if (c == '\r')
  {
    printNewline();
  }
  else
  {
    putCharDirect(c);
  }
}

inline void RC6502Video::printNewline(void)
{
  Serial.println();
}

inline void RC6502Video::putCharDirect(int c)
{
  // Filter standard printable ASCII and common control characters (BS, TAB, LF, VT)
  if ((c >= 0x20 && c <= 0x7E) || c == 0x08 || c == 0x09 || c == 0x0A || c == 0x0B || c == 0x12 || c == 0x13)
  {
    Serial.write(static_cast<uint8_t>(c));
  }
}
