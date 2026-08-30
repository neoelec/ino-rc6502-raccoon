// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>

#include <rcnRC6502.h>

static bool isClassicMode(void)
{
  return analogRead(PIN_PIO_MODE) <= 512;
}

void setup(void)
{
  MCUSR = 0;
  wdt_disable();

  if (isClassicMode())
  {
    RC6502Pio.beginClassic();
  }
  else
  {
    RC6502Pio.begin();
  }
}

void loop(void)
{
  RC6502Pio.run();
}
