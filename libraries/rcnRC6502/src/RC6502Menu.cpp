// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>

#include <Arduino.h>

#include "RC6502Menu.h"
#include "RC6502Utils.h"

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

  if (sd_)
  {
    sd_->mount();
  }

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
  const uint16_t pgm_per_page = 20;

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

  Serial.println();
  Serial.print(F("RCN: PROGRAM NUMBER [0 <= pgm <= 999]"));
  if (!menu_cmd_.getStrValue(str_bm) || str_bm.length() == 0)
  {
    menu_cmd_.giveCmdPrompt();
    return;
  }

  long pgm_number = str_bm.toInt();
  if (pgm_number < 0 || pgm_number > 999)
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

  loadPgmFile();
  menu_cmd_.giveCmdPrompt();
}

void RC6502MenuClass::doCmdSelectDirectory(void)
{
  String str_bm;

  Serial.println();
  Serial.print(F("RCN: DIRECTORY NUMBER [0 <= dir <= 99]"));
  if (!menu_cmd_.getStrValue(str_bm) || str_bm.length() == 0)
  {
    menu_cmd_.giveCmdPrompt();
    return;
  }

  long dir_number = str_bm.toInt();
  if (dir_number < 0 || dir_number > 99)
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

void RC6502MenuClass::feedOneCharacter(int c)
{
  if (!kbd_)
  {
    return;
  }

  c = (c == '\n') ? '\r' : c;
  kbd_->pushToBuffer(c);
}

void RC6502MenuClass::busyWaitConsole(void)
{
  if (!kbd_ || !video_)
  {
    return;
  }

  while (!kbd_->isBufferEmpty())
  {
    kbd_->run();
    video_->run();
  }
}

void RC6502MenuClass::loadPgmFile(void)
{
  if (!openPgmFile())
  {
    Serial.println();
    Serial.println(F("RCN: Could not open program file. Load aborted!"));
    return;
  }

  Serial.println();
  Serial.print(F("RCN: Loading program ("));
  Serial.print(pgm_.getPgmFile());
  Serial.println(F(")..."));

  if (!executeLoadPgmFile())
  {
    Serial.println();
    Serial.println(F("RCN: Load aborted due to error!"));
    return;
  }

  Serial.println();
  pgm_.printProgram();
  Serial.println();
}

bool RC6502MenuClass::executeLoadPgmFile(void)
{
  if (!sd_ || !kbd_ || !video_)
  {
    return false;
  }

  const char *pgm_file = pgm_.getPgmFile();
  bool empty_file = true;
  uint8_t buffer[64];
  uint8_t error = FR_OK;
  uint8_t sz_read = 0;

  do
  {
    error = sd_->read(buffer, sizeof(buffer), sz_read);
    if (sz_read > 0)
    {
      empty_file = false;
    }

    if (error != FR_OK)
    {
      sd_->printError(error, RC6502Sd::READ, pgm_file);
      return false;
    }

    for (uint8_t i = 0; i < sz_read; i++)
    {
      // If the keyboard buffer is full, process FSM until space is available
      while (kbd_->isBufferFull())
      {
        kbd_->run();
        video_->run();
      }

      feedOneCharacter(buffer[i]);

      // Step the FSM forward immediately to keep transmission pipeline saturated
      kbd_->run();
      video_->run();
    }
  } while (sz_read == sizeof(buffer));

  if (empty_file)
  {
    Serial.println();
    Serial.println(F("RCN: Empty file - Load aborted!"));
    return false;
  }

  // Drain all remaining queued characters
  busyWaitConsole();

  // Send final carriage return and drain
  feedOneCharacter('\r');
  busyWaitConsole();

  return true;
}

bool RC6502MenuClass::openPgmFile(void)
{
  if (!sd_)
  {
    return false;
  }

  const char *pgm_file = pgm_.getPgmFile();
  uint8_t error = sd_->mount();
  if (error != FR_OK)
  {
    sd_->printError(error, RC6502Sd::MOUNT);
    return false;
  }

  error = sd_->open(pgm_file);
  if (error != FR_OK)
  {
    sd_->printError(error, RC6502Sd::OPEN, pgm_file);
    return false;
  }

  return true;
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
