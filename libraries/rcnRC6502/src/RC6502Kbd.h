// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_KBD_H
#define RCN_RC6502_KBD_H

#include <stdint.h>

#include <Adafruit_MCP23X17.h>
#include <RingBuf.h>

#include "RC6502Pins.h"

class RC6502Kbd
{
public:
  enum class State : uint8_t
  {
    Idle,
    Write,
    WaitInt,
    PollClear
  };

  void begin(Adafruit_MCP23X17 *mcp);
  void reset(void);
  bool isBufferEmpty(void) const;
  bool isBufferFull(void) const;
  int popFromBuffer(void);
  void pushToBuffer(int ch);
  void run(void);
  void setInterrupt(void);

private:
  void initMcp(void);
  void initPin(void);

  void handleIdle(void);
  void handleWrite(void);
  void handleWaitInt(void);
  void handlePollClear(void);

private:
  Adafruit_MCP23X17 *mcp_{nullptr};
  RingBuf<uint8_t, 64> serial_buf_;
  volatile bool interrupt_{false};
  State state_{State::Idle};
};

#endif // RCN_RC6502_KBD_H
