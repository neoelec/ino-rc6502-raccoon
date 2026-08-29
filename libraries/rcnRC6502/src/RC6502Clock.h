// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_CLOCK_H
#define RCN_RC6502_CLOCK_H

class RC6502Clock
{
public:
  void begin(void);
  void enable(void);
  void disable(void);
  void reset(void);
};

#endif // RCN_RC6502_CLOCK_H
