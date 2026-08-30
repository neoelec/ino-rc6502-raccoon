// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_PINS_H
#define RCN_RC6502_PINS_H

#include <stdint.h>

#include <Arduino.h>

// Arduino Nano Pin Mapping
constexpr uint8_t PIN_KBD_CLR        = 2;  // INT0: Keyboard Clear / Ready Interrupt
constexpr uint8_t PIN_VIDEO_DA       = 3;  // Video Data Available Input
constexpr uint8_t PIN_KBD_STR        = 4;  // Keyboard Strobe Output
constexpr uint8_t PIN_VIDEO_nRDA     = 5;  // Video Read Data Acknowledge Output
constexpr uint8_t PIN_SD_nSS         = 7;  // Micro-SD SPI Chip Select
constexpr uint8_t PIN_nRESET         = 8;  // 6502 CPU Hardware Reset Output
constexpr uint8_t PIN_CLK_1MHZ       = 9;  // Timer1 OC1A: 1MHz Clock Output
constexpr uint8_t PIN_MCP23S17_nSS   = 10; // MCP23S17 SPI Chip Select
constexpr uint8_t PIN_PIO_MODE       = A7; // Mode Select Pin (Analog Input only: ATmega328P A6/A7 have no digital I/O)

// MCP23S17 Port A Pin Mapping (Video Data Input)
constexpr uint8_t PIN_VIDEO_D0       = 0;  // Port A0 (Bit 0)
constexpr uint8_t PIN_VIDEO_D1       = 1;  // Port A1 (Bit 1)
constexpr uint8_t PIN_VIDEO_D2       = 2;  // Port A2 (Bit 2)
constexpr uint8_t PIN_VIDEO_D3       = 3;  // Port A3 (Bit 3)
constexpr uint8_t PIN_VIDEO_D4       = 4;  // Port A4 (Bit 4)
constexpr uint8_t PIN_VIDEO_D5       = 5;  // Port A5 (Bit 5)
constexpr uint8_t PIN_VIDEO_D6       = 6;  // Port A6 (Bit 6)

// MCP23S17 Port B Pin Mapping (Keyboard Data Output)
constexpr uint8_t PIN_KBD_D0         = 8;  // Port B0 (Bit 0)
constexpr uint8_t PIN_KBD_D1         = 9;  // Port B1 (Bit 1)
constexpr uint8_t PIN_KBD_D2         = 10; // Port B2 (Bit 2)
constexpr uint8_t PIN_KBD_D3         = 11; // Port B3 (Bit 3)
constexpr uint8_t PIN_KBD_D4         = 12; // Port B4 (Bit 4)
constexpr uint8_t PIN_KBD_D5         = 13; // Port B5 (Bit 5)
constexpr uint8_t PIN_KBD_D6         = 14; // Port B6 (Bit 6)
constexpr uint8_t PIN_KBD_DA         = 15; // Port B7 (Data Available Flag)

#endif // RCN_RC6502_PINS_H
