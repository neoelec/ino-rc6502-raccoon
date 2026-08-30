// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_PIO_H
#define RCN_RC6502_PIO_H

#include "RC6502Dev.h"

class RC6502PioClass
{
public:
  enum class Mode : uint8_t
  {
    Auto,
    Classic,
    Modded
  };

  enum class State : uint8_t
  {
    Classic,
    Keyboard,
    MenuEnter,
    MenuRun
  };

  void begin(Mode mode = Mode::Auto);
  void beginClassic(void);
  void run(void);
  static bool isClassicMode(void);

private:
  void beginCommon(void);
  void printBanner(void);
  void printClassicBanner(void);
  void handleStateClassic(void);
  void handleStateKeyboard(void);
  void handleStateMenuEnter(void);
  void handleStateMenuRun(void);

private:
  RC6502Dev dev_;

  RC6502Clock *clk_src_{nullptr};
  RC6502Kbd *kbd_{nullptr};
  RC6502Video *video_{nullptr};

  State state_{State::Keyboard};
};

extern RC6502PioClass RC6502Pio;

#endif // RCN_RC6502_PIO_H
