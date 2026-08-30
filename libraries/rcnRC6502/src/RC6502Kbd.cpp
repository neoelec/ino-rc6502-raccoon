// SPDX-License-Identifier: MIT
// Copyright (c) 2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <ctype.h>

#include "RC6502Kbd.h"

void RC6502Kbd::begin(Adafruit_MCP23X17 *mcp)
{
  mcp_ = mcp;
  interrupt_ = false;
  state_ = State::Idle;
  strobe_start_ms_ = 0;
  serial_buf_.clear();

  reset();
}

void RC6502Kbd::reset(void)
{
  initMcp();
  initPin();
  interrupt_ = false;
  strobe_start_ms_ = 0;
  state_ = State::Idle;
}

void RC6502Kbd::run(void)
{
  switch (state_)
  {
  case State::Idle:
    handleIdle();
    break;
  case State::Write:
    handleWrite();
    break;
  case State::WaitInt:
    handleWaitInt();
    break;
  case State::PollClear:
    handlePollClear();
    break;
  case State::Timeout:
    handleTimeout();
    break;
  }
}

void RC6502Kbd::setInterrupt(void)
{
  interrupt_ = true;
}

void RC6502Kbd::pushToBuffer(int c)
{
  if (c < 0 || c >= 0x80) // ignore negative and ASCII Extended Characters
  {
    return;
  }

  if (c == '\n')
  {
    c = '\r'; // Normalize LF to Apple 1 standard CR
  }

  c = toupper(static_cast<unsigned char>(c));
  serial_buf_.push(static_cast<uint8_t>(c));
}

bool RC6502Kbd::isBufferFull(void) noexcept
{
  return serial_buf_.isFull();
}

int RC6502Kbd::popFromBuffer(void)
{
  uint8_t c = 0;
  serial_buf_.pop(c);
  return static_cast<int>(c);
}

bool RC6502Kbd::isBufferEmpty(void) noexcept
{
  return serial_buf_.isEmpty();
}

bool RC6502Kbd::isIdle(void) noexcept
{
  return serial_buf_.isEmpty() && state_ == State::Idle;
}

void RC6502Kbd::initMcp(void)
{
  if (!mcp_)
  {
    return;
  }

  for (uint8_t pin = PIN_KBD_D0; pin <= PIN_KBD_DA; pin++)
  {
    mcp_->pinMode(pin, OUTPUT);
  }

  mcp_->writeGPIOB(0x00);
}

void RC6502Kbd::initPin(void)
{
  pinMode(PIN_KBD_CLR, INPUT);

  pinMode(PIN_KBD_STR, OUTPUT);
  digitalWrite(PIN_KBD_STR, LOW);
}

void RC6502Kbd::handleIdle(void)
{
  if (isBufferEmpty())
  {
    return;
  }

  state_ = State::Write;
  handleWrite();
}

void RC6502Kbd::handleWrite(void)
{
  int c = popFromBuffer();

  if (mcp_)
  {
    // Apple 1 Keyboard data has bit 7 set as high strobe flag
    mcp_->writeGPIOB(static_cast<uint8_t>(c | 0x80));
  }

  interrupt_ = false; // Reset handshake flag immediately before strobe assertion
  digitalWrite(PIN_KBD_STR, HIGH);
  strobe_start_ms_ = millis();

  state_ = State::WaitInt;
}

void RC6502Kbd::handleWaitInt(void)
{
  if (!interrupt_)
  {
    // Timeout safeguard: if target 6502 CPU does not acknowledge within 100ms
    if (millis() - strobe_start_ms_ > 100)
    {
      state_ = State::Timeout;
      handleTimeout();
    }
    return;
  }

  digitalWrite(PIN_KBD_STR, LOW);

  interrupt_ = false;
  state_ = State::PollClear;
  handlePollClear();
}

void RC6502Kbd::handlePollClear(void)
{
  if (digitalRead(PIN_KBD_CLR) != LOW)
  {
    // Timeout safeguard: if KBD_CLR does not return LOW within 200ms
    if (millis() - strobe_start_ms_ > 200)
    {
      state_ = State::Timeout;
      handleTimeout();
    }
    return;
  }

  state_ = State::Idle;
}

void RC6502Kbd::handleTimeout(void)
{
  digitalWrite(PIN_KBD_STR, LOW);
  if (mcp_)
  {
    mcp_->writeGPIOB(0x00);
  }
  interrupt_ = false;
  state_ = State::Idle;
}
