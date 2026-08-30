// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <rcnRC6502.h>

void setup(void)
{
  RC6502Pio.begin();
}

void loop(void)
{
  RC6502Pio.run();
}
