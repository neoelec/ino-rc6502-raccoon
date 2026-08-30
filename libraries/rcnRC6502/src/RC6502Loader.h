// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_LOADER_H
#define RCN_RC6502_LOADER_H

#include <stdint.h>

#include "RC6502Kbd.h"
#include "RC6502Pgm.h"
#include "RC6502Sd.h"
#include "RC6502Video.h"

class RC6502Loader
{
public:
  bool load(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const RC6502Pgm &pgm);

private:
  bool openFile(RC6502Sd *sd, const char *file_name);
  bool streamTextFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name);
  bool streamBinaryFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name, uint16_t start_address);
  void feedOneCharacter(RC6502Kbd *kbd, int c);
  void feedHexByte(RC6502Kbd *kbd, RC6502Video *video, uint8_t val);
  void feedHexAddress(RC6502Kbd *kbd, RC6502Video *video, uint16_t addr);
  void feedCharPipelined(RC6502Kbd *kbd, RC6502Video *video, char c);
  void busyWaitConsole(RC6502Kbd *kbd, RC6502Video *video);
  void drainVideo(RC6502Kbd *kbd, RC6502Video *video, uint32_t quiet_ms = 250, uint32_t max_wait_ms = 2000);
};

#endif // RCN_RC6502_LOADER_H
