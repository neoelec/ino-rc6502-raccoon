// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>

#include <Arduino.h>

#include "RC6502Menu.h"
#include "RC6502Utils.h"

constexpr uint8_t DEFAULT_PGM_PER_PAGE = 20;
constexpr uint8_t MAX_DIR_NUMBER       = 99;
constexpr uint16_t MAX_PGM_NUMBER      = 999;

RC6502MenuClass RC6502Menu;

void RC6502MenuClass::begin(RC6502Dev &dev)
{
  clock_ = dev.getClock();
  kbd_ = dev.getKbd();
  sd_ = dev.getSd();
  video_ = dev.getVideo();

  initializeMenuCmd();
}

void RC6502MenuClass::enter(void)
{
  Serial.println();
  Serial.print(F("RCN: Entering menu ..."));

  done_ = false;
  dir_number_ = 0;

  menu_cmd_.ShowMenu();
  menu_cmd_.giveCmdPrompt();
}

bool RC6502MenuClass::run(void)
{
  if (isDone())
  {
    return false;
  }

  uint8_t cmd = menu_cmd_.UserRequest();
  if (cmd)
  {
    menu_cmd_.ExeCommand(cmd);
  }

  return true;
}

SerialMenuCmd *RC6502MenuClass::getMenuCmd(void)
{
  return &menu_cmd_;
}

void RC6502MenuClass::doCmdHelp(void)
{
  menu_cmd_.ShowMenu();
  menu_cmd_.giveCmdPrompt();
}

void RC6502MenuClass::doCmdExit(void)
{
  Serial.println();
  Serial.println(F("RCN: Exiting from menu ..."));
  Serial.println();

  done_ = true;
}

void RC6502MenuClass::doCmdListPrograms(void)
{
  String str_bm;
  str_bm.reserve(8);
  const uint16_t pgm_per_page = DEFAULT_PGM_PER_PAGE;

  Serial.println();
  Serial.print(F("RCN: PAGE NUMBER [>=0]"));
  if (!menu_cmd_.getStrValue(str_bm) || str_bm.length() == 0)
  {
    menu_cmd_.giveCmdPrompt();
    return;
  }

  long page_number = str_bm.toInt();
  if (page_number < 0)
  {
    Serial.println();
    Serial.print(F("Wrong Value "));
    Serial.print(page_number, DEC);
    Serial.println(F(". It should be page_number >= 0"));
    menu_cmd_.giveCmdPrompt();
    return;
  }

  listPrograms(static_cast<uint16_t>(page_number), pgm_per_page);
  menu_cmd_.giveCmdPrompt();
}

void RC6502MenuClass::doCmdLoadProgram(void)
{
  String str_bm;
  str_bm.reserve(8);

  Serial.println();
  Serial.print(F("RCN: PROGRAM NUMBER [0 <= pgm <= 999]"));
  if (!menu_cmd_.getStrValue(str_bm) || str_bm.length() == 0)
  {
    menu_cmd_.giveCmdPrompt();
    return;
  }

  long pgm_number = str_bm.toInt();
  if (pgm_number < 0 || pgm_number > MAX_PGM_NUMBER)
  {
    Serial.println();
    Serial.print(F("Wrong Value "));
    Serial.print(pgm_number, DEC);
    Serial.println(F(". It should be 0 <= pgm_number <= 999"));
    menu_cmd_.giveCmdPrompt();
    return;
  }

  if (!pgm_.begin(sd_, dir_number_, static_cast<uint16_t>(pgm_number)))
  {
    Serial.println();
    Serial.println(F("RCN: Failed to find program!"));
    menu_cmd_.giveCmdPrompt();
    return;
  }

  RC6502Loader loader;
  loader.load(sd_, kbd_, video_, pgm_);
  menu_cmd_.giveCmdPrompt();
}

void RC6502MenuClass::doCmdSelectDirectory(void)
{
  String str_bm;
  str_bm.reserve(8);

  Serial.println();
  Serial.print(F("RCN: DIRECTORY NUMBER [0 <= dir <= 99]"));
  if (!menu_cmd_.getStrValue(str_bm) || str_bm.length() == 0)
  {
    menu_cmd_.giveCmdPrompt();
    return;
  }

  long dir_number = str_bm.toInt();
  if (dir_number < 0 || dir_number > MAX_DIR_NUMBER)
  {
    Serial.println();
    Serial.print(F("Wrong Value "));
    Serial.print(dir_number, DEC);
    Serial.println(F(". It should be 0 <= dir_number <= 99"));
    menu_cmd_.giveCmdPrompt();
    return;
  }

  dir_number_ = static_cast<uint8_t>(dir_number);
  Serial.println();
  Serial.print(F("RCN: DIRECTORY - "));
  Serial.println(dir_number_, DEC);
  Serial.println();

  menu_cmd_.giveCmdPrompt();
}

void RC6502MenuClass::doCmdPIOReset(void)
{
  Serial.println();
  Serial.println(F("RCN: PIO reset ..."));
  Serial.flush();
  wdt_enable(WDTO_15MS);
  while (true)
  {
    // Wait for watchdog reset
  }
}

void RC6502MenuClass::doCmdWarmReset(void)
{
  Serial.println();
  Serial.println(F("RCN: Warm reset ..."));
  if (clock_)
  {
    clock_->reset();
  }
  if (kbd_)
  {
    kbd_->reset();
  }
  if (video_)
  {
    video_->reset();
  }
  Serial.flush();
  done_ = true;
}

bool RC6502MenuClass::isDone(void) const
{
  return done_;
}

void RC6502MenuClass::initializeMenuCmd(void)
{
  static tMenuCmdTxt prompt[] PROGMEM = "-";
  static tMenuCmdTxt txt_s[] PROGMEM = "s - Select Directory";
  static tMenuCmdTxt txt_l[] PROGMEM = "l - List Programs";
  static tMenuCmdTxt txt_o[] PROGMEM = "o - Load Program";
  static tMenuCmdTxt txt_x[] PROGMEM = "x - Exit";
  static tMenuCmdTxt txt_p[] PROGMEM = "p - PIO Reset";
  static tMenuCmdTxt txt_w[] PROGMEM = "w - Warm Reset";
  static tMenuCmdTxt txt__[] PROGMEM = "? - Help";

  static stMenuCmd menu_list[] = {
      {txt_s, 's', []() { RC6502Menu.doCmdSelectDirectory(); }},
      {txt_l, 'l', []() { RC6502Menu.doCmdListPrograms(); }},
      {txt_o, 'o', []() { RC6502Menu.doCmdLoadProgram(); }},
      {txt_x, 'x', []() { RC6502Menu.doCmdExit(); }},
      {txt_p, 'p', []() { RC6502Menu.doCmdPIOReset(); }},
      {txt_w, 'w', []() { RC6502Menu.doCmdWarmReset(); }},
      {txt__, '?', []() { RC6502Menu.doCmdHelp(); }}};

  if (menu_cmd_.getNbCmds() > 0)
  {
    return;
  }

  if (!menu_cmd_.begin(menu_list, RC6502Utils::arraySize(menu_list), prompt))
  {
    Serial.println(F("RCN: MENU initialization failed"));
  }
}

void RC6502MenuClass::listPrograms(uint16_t page_number, uint16_t pgm_per_page)
{
  RC6502Pgm pgm_tmp;
  uint16_t pgm_begin = page_number * pgm_per_page;
  Serial.println();
  size_t n;

  for (uint16_t i = 0; i < pgm_per_page; i++)
  {
    uint16_t pgm_number = i + pgm_begin;

    if (!pgm_tmp.begin(sd_, dir_number_, pgm_number))
    {
      break;
    }

    n = Serial.print(pgm_number, DEC);
    Serial.print(F("."));
    if (4 > n)
    {
      RC6502Utils::printSpaces(4 - n);
    }

    n = Serial.print(pgm_tmp.getDescription());
    if (26 > n)
    {
      RC6502Utils::printSpaces(26 - n);
    }
    Serial.print(F(" ("));
    Serial.print(pgm_tmp.getTypeT());
    Serial.println(F(")"));
  }

  Serial.println();
}
