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
  bool streamFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name);
  void feedOneCharacter(RC6502Kbd *kbd, int c);
  void busyWaitConsole(RC6502Kbd *kbd, RC6502Video *video);
};

#endif // RCN_RC6502_LOADER_H
