// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_MENU_H
#define RCN_RC6502_MENU_H

#include <stdint.h>

#include <Arduino.h>

#include "RC6502Dev.h"
#include "RC6502Kbd.h"
#include "RC6502Loader.h"
#include "RC6502Pgm.h"
#include "RC6502Sd.h"
#include "RC6502Video.h"

class RC6502MenuClass
{
public:
  enum class State : uint8_t
  {
    Command,
    PromptPrefix,
    PromptLoad,
    PromptInfo,
    PromptMemory,
    CatalogPaging,
    PrefixPaging
  };

  static constexpr uint8_t PGM_PER_PAGE = 16;

  void begin(RC6502Dev &dev);
  void enter(void);
  bool run(void);
  void doCmdHelp(void);
  void doCmdExit(void);
  void doCmdPIOReset(void);
  void doCmdWarmReset(void);
  bool isDone(void) const;

private:
  void showMenu(void);
  void printPrompt(void);
  void handleCommand(char c);
  void handleInputPrompt(char c);
  void handleCatalogPagingInput(char c);
  void handlePrefixPagingInput(char c);
  void processPromptSubmit(void);
  void startPrompt(State next_state, const __FlashStringHelper *prompt_msg);

  bool loadDefaultPrefix(void);
  uint16_t countCatalogEntries(void);
  uint16_t countPrefixEntries(void);
  void listCatalogPage(uint16_t page_num);
  void listPrefixesPage(uint16_t page_num);
  bool resolvePrefix(const char *input, char *out_prefix, size_t max_len);
  void executeLoad(const char *query);
  void executeSetPrefix(const char *input);
  void displayProgramInfo(const char *query);

  void doCmdCatalog(void);
  void doCmdPrefix(void);
  void doCmdLoad(void);
  void doCmdInfo(void);
  void doCmdMemory(void);
  void doCmdTerse(void);

private:
  RC6502Clock *clock_{nullptr};
  RC6502Kbd *kbd_{nullptr};
  RC6502Sd *sd_{nullptr};
  RC6502Video *video_{nullptr};

  bool done_{true};
  bool terse_mode_{false};
  State state_{State::Command};
  char input_buf_[32]{0};
  uint8_t input_len_{0};

  uint16_t cat_page_{0};
  uint16_t cat_total_entries_{0};
  uint16_t pfx_page_{0};
  uint16_t pfx_total_entries_{0};

  RC6502Pgm pgm_;
  char prefix_[32]{0};
};

extern RC6502MenuClass RC6502Menu;

#endif // RCN_RC6502_MENU_H
