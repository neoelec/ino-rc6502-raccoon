// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Clock.h"
#include "RC6502Pins.h"

void RC6502Clock::begin(void)
{
  // Configure Timer 1 for 1MHz CTC toggle mode on OC1A (PIN 9 / PIN_CLK_1MHZ)
  // Base clock 16MHz, prescaler = 1, TOP = 7 -> f = 16MHz / (2 * 1 * (1 + 7)) = 1MHz
  TCCR1A = 0;                      // Disconnect OC1A, Normal port operation
  TCCR1B = 0;                      // Stop Timer1 clock
  TCNT1  = 0;                      // Reset counter value
  OCR1A  = 7;                      // 1MHz TOP toggle value (16-bit write)
  TIMSK1 = 0;                      // Disable all Timer1 interrupts
  TCCR1B = _BV(WGM12) | _BV(CS10); // CTC mode, Prescaler = 1

  disable();
}

void RC6502Clock::enable(void)
{
  TCCR1A |= _BV(COM1A0);
  TCCR1A &= ~_BV(COM1A1);
}

void RC6502Clock::disable(void)
{
  TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0));

  pinMode(PIN_nRESET, INPUT); // Hi-Z

  pinMode(PIN_CLK_1MHZ, OUTPUT);
  digitalWrite(PIN_CLK_1MHZ, LOW);
}

void RC6502Clock::reset(void)
{
  digitalWrite(PIN_nRESET, LOW); // Set PORT LOW before enabling output to avoid high glitch
  pinMode(PIN_nRESET, OUTPUT);
  delay(15);                     // Hold reset low for at least 6 clock cycles
  pinMode(PIN_nRESET, INPUT);    // Release to Hi-Z (the external pull-up R2 already exists)
  digitalWrite(PIN_nRESET, LOW); // Ensure internal pull-up is disabled
}
