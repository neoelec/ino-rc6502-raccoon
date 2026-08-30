// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_VIDEO_H
#define RCN_RC6502_VIDEO_H

#include <Adafruit_MCP23X17.h>

class RC6502Video
{
public:
  void begin(Adafruit_MCP23X17 *mcp);
  void reset(void);
  void run(void);
  void setInterrupt(void);

private:
  void initMcp(void);
  void initPin(void);
  inline void putChar(int c);
  inline void printNewline(void);
  inline void putCharDirect(int c);

private:
  Adafruit_MCP23X17 *mcp_{nullptr};
  volatile bool interrupt_{false};
};

#endif // RCN_RC6502_VIDEO_H
