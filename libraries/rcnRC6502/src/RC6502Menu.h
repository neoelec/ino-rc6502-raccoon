// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_MENU_H
#define RCN_RC6502_MENU_H

#include <stdint.h>

#include <SerialMenuCmd.h>

#include "RC6502Dev.h"
#include "RC6502Kbd.h"
#include "RC6502Loader.h"
#include "RC6502Pgm.h"
#include "RC6502Sd.h"
#include "RC6502Video.h"

class RC6502MenuClass
{
public:
  void begin(RC6502Dev &dev);
  void enter(void);
  bool run(void);
  SerialMenuCmd *getMenuCmd(void);
  void doCmdHelp(void);
  void doCmdExit(void);
  void doCmdListPrograms(void);
  void doCmdLoadProgram(void);
  void doCmdPIOReset(void);
  void doCmdSelectDirectory(void);
  void doCmdWarmReset(void);
  bool isDone(void) const;

private:
  void initializeMenuCmd(void);
  void listPrograms(uint16_t page_number, uint16_t pgm_per_page);

private:
  SerialMenuCmd menu_cmd_;
  RC6502Clock *clock_{nullptr};
  RC6502Kbd *kbd_{nullptr};
  RC6502Sd *sd_{nullptr};
  RC6502Video *video_{nullptr};

  bool done_{true};
  RC6502Pgm pgm_;
  uint8_t dir_number_{0};
};

extern RC6502MenuClass RC6502Menu;

#endif // RCN_RC6502_MENU_H
