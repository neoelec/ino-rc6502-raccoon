// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <avr/wdt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#include "RC6502Menu.h"
#include "RC6502Utils.h"

RC6502MenuClass RC6502Menu;

static inline void printHex4(uint16_t val)
{
  if (val < 0x1000) Serial.print('0');
  if (val < 0x0100) Serial.print('0');
  if (val < 0x0010) Serial.print('0');
  Serial.print(val, HEX);
}

void RC6502MenuClass::begin(RC6502Dev &dev)
{
  clock_ = dev.getClock();
  kbd_ = dev.getKbd();
  sd_ = dev.getSd();
  video_ = dev.getVideo();
}

void RC6502MenuClass::enter(void)
{
  done_ = false;
  if (prefix_[0] == '\0')
  {
    strncpy(prefix_, "SYSTEM", sizeof(prefix_) - 1);
    prefix_[sizeof(prefix_) - 1] = '\0';
  }
  state_ = State::Command;
  input_len_ = 0;
  input_buf_[0] = '\0';

  showMenu();
  printPrompt();
}

bool RC6502MenuClass::run(void)
{
  if (isDone())
  {
    return false;
  }

  if (Serial.available() <= 0)
  {
    return true;
  }

  char c = static_cast<char>(Serial.read());

  if (state_ == State::Command)
  {
    handleCommand(c);
  }
  else if (state_ == State::CatalogPaging)
  {
    handleCatalogPagingInput(c);
  }
  else if (state_ == State::PrefixPaging)
  {
    handlePrefixPagingInput(c);
  }
  else
  {
    handleInputPrompt(c);
  }

  return true;
}

void RC6502MenuClass::handleCommand(char cmd_char)
{
  char cmd = static_cast<char>(toupper(static_cast<unsigned char>(cmd_char)));

  // Handle Enter (CR or LF)
  if (cmd == '\r' || cmd == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }
    printPrompt();
    return;
  }

  Serial.println(cmd);

  switch (cmd)
  {
  case 'C':
    doCmdCatalog();
    break;
  case 'P':
    doCmdPrefix();
    break;
  case 'L':
    doCmdLoad();
    break;
  case 'I':
    doCmdInfo();
    break;
  case 'M':
    doCmdMemory();
    break;
  case 'T':
    doCmdTerse();
    break;
  case 'R':
    doCmdWarmReset();
    break;
  case 'Q':
  case 'X':
    doCmdExit();
    break;
  case '?':
  case 'H':
    doCmdHelp();
    break;
  case 'W':
    doCmdPIOReset();
    break;
  case 'S':
  case 'D':
  case 'N':
  case 'B':
  case '!':
    Serial.println(F("Command not implemented yet."));
    printPrompt();
    break;
  default:
    Serial.println(F("Unknown command. Press '?' for help."));
    printPrompt();
    break;
  }
}

void RC6502MenuClass::startPrompt(State next_state, const __FlashStringHelper *prompt_msg)
{
  state_ = next_state;
  input_len_ = 0;
  input_buf_[0] = '\0';

  Serial.println();
  Serial.print(prompt_msg);
}

void RC6502MenuClass::handleInputPrompt(char c)
{
  if (c == 0x1B) // Escape key
  {
    Serial.println();
    state_ = State::Command;
    input_len_ = 0;
    input_buf_[0] = '\0';
    printPrompt();
    return;
  }

  if (c == '\b' || c == 0x7F) // Backspace / Delete
  {
    if (input_len_ > 0)
    {
      input_len_--;
      input_buf_[input_len_] = '\0';
      Serial.print(F("\b \b"));
    }
    return;
  }

  if (c == '\r' || c == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }

    if (input_len_ == 0)
    {
      state_ = State::Command;
      printPrompt();
      return;
    }

    input_buf_[input_len_] = '\0';
    processPromptSubmit();
    return;
  }

  if (isprint(static_cast<unsigned char>(c)) && input_len_ < sizeof(input_buf_) - 1)
  {
    char uc = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    input_buf_[input_len_++] = uc;
    input_buf_[input_len_] = '\0';
    Serial.write(uc);
  }
}

bool RC6502MenuClass::resolvePrefix(const char *input, char *out_prefix, size_t max_len)
{
  if (!input || input[0] == '\0' || !out_prefix || max_len == 0)
  {
    return false;
  }

  bool is_digits = true;
  for (size_t i = 0; input[i] != '\0'; i++)
  {
    if (!isdigit(static_cast<unsigned char>(input[i])))
    {
      is_digits = false;
      break;
    }
  }
  long target_idx = is_digits ? atol(input) : -1;

  if (sd_)
  {
    uint8_t err = sd_->open("PREFIX.CSV");
    if (err == FR_OK)
    {
      char line[64]{0};
      uint16_t cur_idx = 0;
      while (RC6502Pgm::readLine(sd_, line, sizeof(line)))
      {
        char *trimmed = RC6502Utils::trim(line);
        if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0') continue;

        char *ptr = trimmed;
        char *token_pfx = strsep(&ptr, ",");
        token_pfx = RC6502Utils::trim(token_pfx);
        if (token_pfx && *token_pfx)
        {
          if (is_digits)
          {
            if (target_idx >= 0 && cur_idx == static_cast<uint16_t>(target_idx))
            {
              strncpy(out_prefix, token_pfx, max_len - 1);
              out_prefix[max_len - 1] = '\0';
              return true;
            }
          }
          else
          {
            if (strcasecmp(token_pfx, input) == 0)
            {
              strncpy(out_prefix, token_pfx, max_len - 1);
              out_prefix[max_len - 1] = '\0';
              return true;
            }
          }
        }
        cur_idx++;
      }
    }
  }

  // Fallback for default aliases
  if (strcmp(input, "00") == 0 || strcmp(input, "0") == 0)
  {
    strncpy(out_prefix, "SYSTEM", max_len - 1);
  }
  else if (strcmp(input, "01") == 0 || strcmp(input, "1") == 0)
  {
    strncpy(out_prefix, "GAMES", max_len - 1);
  }
  else if (strcmp(input, "02") == 0 || strcmp(input, "2") == 0)
  {
    strncpy(out_prefix, "EXTBAS", max_len - 1);
  }
  else
  {
    strncpy(out_prefix, input, max_len - 1);
  }
  out_prefix[max_len - 1] = '\0';
  return true;
}

void RC6502MenuClass::executeSetPrefix(const char *input)
{
  if (!input || input[0] == '\0')
  {
    state_ = State::Command;
    printPrompt();
    return;
  }

  char resolved[sizeof(prefix_)];
  if (resolvePrefix(input, resolved, sizeof(resolved)))
  {
    strncpy(prefix_, resolved, sizeof(prefix_) - 1);
    prefix_[sizeof(prefix_) - 1] = '\0';
  }
  else
  {
    strncpy(prefix_, input, sizeof(prefix_) - 1);
    prefix_[sizeof(prefix_) - 1] = '\0';
  }

  state_ = State::Command;
  Serial.println();
  Serial.print(F("PREFIX SET TO: /"));
  Serial.print(prefix_);
  Serial.println(F("/"));
  printPrompt();
}

void RC6502MenuClass::executeLoad(const char *query)
{
  if (!query || query[0] == '\0')
  {
    state_ = State::Command;
    printPrompt();
    return;
  }

  Serial.println();
  if (!pgm_.find(sd_, prefix_, query))
  {
    Serial.print(F("File not found ("));
    Serial.print(query);
    Serial.println(F(")!"));
    state_ = State::Command;
    printPrompt();
    return;
  }

  if (pgm_.getType() == RC6502Pgm::Type::Dir)
  {
    strncpy(prefix_, pgm_.getPgmFile(), sizeof(prefix_) - 1);
    prefix_[sizeof(prefix_) - 1] = '\0';
    size_t plen = strlen(prefix_);
    if (plen > 0 && prefix_[plen - 1] == '/')
    {
      prefix_[plen - 1] = '\0';
    }
    Serial.print(F("ENTERING DIRECTORY: /"));
    Serial.print(prefix_);
    Serial.println(F("/"));
    doCmdCatalog();
    return;
  }

  {
    RC6502Loader loader;
    if (loader.load(sd_, kbd_, video_, pgm_))
    {
      Serial.print(F("LOAD COMPLETE. (START AT $"));
      printHex4(pgm_.getRunAddress());
      Serial.println(F(")"));
    }
  }
  state_ = State::Command;
  printPrompt();
}

void RC6502MenuClass::displayProgramInfo(const char *query)
{
  if (!query || query[0] == '\0')
  {
    state_ = State::Command;
    printPrompt();
    return;
  }

  Serial.println();
  if (!pgm_.find(sd_, prefix_, query))
  {
    Serial.print(F("File not found ("));
    Serial.print(query);
    Serial.println(F(")!"));
    state_ = State::Command;
    printPrompt();
    return;
  }

  Serial.println(F("========================================"));
  Serial.println(F("          PROGRAM INFORMATION           "));
  Serial.println(F("========================================"));
  Serial.print(F("NAME    : "));
  Serial.println(pgm_.getDescription());

  Serial.print(F("TYPE    : "));
  Serial.print(pgm_.getTypeT());
  switch (pgm_.getType())
  {
  case RC6502Pgm::Type::Hex:
    Serial.println(F(" (Woz Hex Dump)"));
    break;
  case RC6502Pgm::Type::Bin:
    Serial.println(F(" (Raw Machine Code)"));
    break;
  case RC6502Pgm::Type::Bas:
    Serial.println(F(" (BASIC Program)"));
    break;
  case RC6502Pgm::Type::Dir:
    Serial.println(F(" (Subdirectory)"));
    break;
  default:
    Serial.println();
    break;
  }

  Serial.print(F("FILE    : "));
  Serial.println(pgm_.getPgmFile());

  Serial.print(F("LOAD AT : $"));
  printHex4(pgm_.getLoadAddress());
  Serial.println();

  Serial.print(F("RUN AT  : $"));
  printHex4(pgm_.getRunAddress());
  Serial.println();

  Serial.println(F("----------------------------------------"));
  Serial.print(F("EXECUTE : "));
  if (pgm_.getType() == RC6502Pgm::Type::Dir)
  {
    Serial.println(F("SELECT VIA LOAD / MENU"));
  }
  else if (pgm_.getRunAddress() == 0)
  {
    Serial.println(F("RUN in BASIC Interpreter"));
  }
  else
  {
    printHex4(pgm_.getRunAddress());
    Serial.println(F("R in Woz Monitor"));
  }
  Serial.println(F("========================================"));

  state_ = State::Command;
  printPrompt();
}

void RC6502MenuClass::processPromptSubmit(void)
{
  State current_state = state_;
  state_ = State::Command;

  switch (current_state)
  {
  case State::PromptPrefix:
  {
    if (strcmp(input_buf_, "?") == 0)
    {
      input_len_ = 0;
      input_buf_[0] = '\0';
      pfx_page_ = 0;
      pfx_total_entries_ = countPrefixEntries();
      listPrefixesPage(0);
      return;
    }

    char pfx_input[sizeof(input_buf_)];
    strncpy(pfx_input, input_buf_, sizeof(pfx_input) - 1);
    pfx_input[sizeof(pfx_input) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    executeSetPrefix(pfx_input);
    break;
  }

  case State::PromptLoad:
  {
    char query[sizeof(input_buf_)];
    strncpy(query, input_buf_, sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    executeLoad(query);
    break;
  }

  case State::PromptInfo:
  {
    char query[sizeof(input_buf_)];
    strncpy(query, input_buf_, sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    displayProgramInfo(query);
    break;
  }

  case State::PromptMemory:
  {
    char range_str[sizeof(input_buf_)];
    strncpy(range_str, input_buf_, sizeof(range_str) - 1);
    range_str[sizeof(range_str) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    RC6502Loader::displayMemory(kbd_, video_, range_str);
    printPrompt();
    break;
  }

  default:
    input_len_ = 0;
    input_buf_[0] = '\0';
    printPrompt();
    break;
  }
}

void RC6502MenuClass::showMenu(void)
{
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("           CFFA-1 FOR RC6502            "));
  Serial.println(F("========================================"));
  Serial.println(F("  C - CATALOG"));
  Serial.println(F("  P - PREFIX"));
  Serial.println(F("  L - LOAD FILE"));
  Serial.println(F("  I - PROGRAM INFO"));
  Serial.println(F("  M - MEMORY DISPLAY"));
  Serial.println(F("  T - TOGGLE TERSE MODE"));
  Serial.println(F("  R - WARM RESET (CPU)"));
  Serial.println(F("  W - PIO RESET (MCU)"));
  Serial.println(F("  Q - QUIT TO MONITOR"));
  Serial.println(F("  ? - HELP"));
  Serial.println();
  Serial.print(F("PFX: /"));
  Serial.print(prefix_);
  Serial.print(F("/ | MODE: "));
  Serial.println(terse_mode_ ? F("TERSE") : F("VERBOSE"));
}

void RC6502MenuClass::printPrompt(void)
{
  Serial.println();
  Serial.print(F("ENTER SELECTION: "));
}

void RC6502MenuClass::doCmdHelp(void)
{
  showMenu();
  printPrompt();
}

void RC6502MenuClass::doCmdCatalog(void)
{
  cat_page_ = 0;
  input_len_ = 0;
  input_buf_[0] = '\0';
  cat_total_entries_ = countCatalogEntries();
  listCatalogPage(0);
}

void RC6502MenuClass::doCmdPrefix(void)
{
  startPrompt(State::PromptPrefix, F("ENTER PREFIX (00-02, NAME, ?): "));
}

void RC6502MenuClass::doCmdLoad(void)
{
  startPrompt(State::PromptLoad, F("ENTER FILE NAME OR PGM#: "));
}

void RC6502MenuClass::doCmdInfo(void)
{
  startPrompt(State::PromptInfo, F("ENTER FILE NAME OR PGM#: "));
}

void RC6502MenuClass::doCmdMemory(void)
{
  startPrompt(State::PromptMemory, F("MEM RANGE (e.g. 1000.10FF): "));
}

void RC6502MenuClass::doCmdTerse(void)
{
  terse_mode_ = !terse_mode_;
  Serial.println();
  Serial.print(F("DISPLAY MODE: "));
  Serial.println(terse_mode_ ? F("TERSE") : F("VERBOSE"));
  printPrompt();
}

void RC6502MenuClass::doCmdExit(void)
{
  Serial.println();
  Serial.println(F("Exiting to Woz Monitor ..."));
  Serial.println();
  Serial.print(F("\\\r\n"));

  done_ = true;
}

void RC6502MenuClass::doCmdPIOReset(void)
{
  Serial.println();
  Serial.println(F("PIO reset ..."));
  Serial.flush();
  cli();
  wdt_enable(WDTO_15MS);
  while (true)
  {
    // Wait for watchdog reset
  }
}

void RC6502MenuClass::doCmdWarmReset(void)
{
  Serial.println();
  Serial.println(F("6502 CPU Warm reset ..."));
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

uint16_t RC6502MenuClass::countPrefixEntries(void)
{
  if (!sd_)
  {
    return 0;
  }

  uint8_t err = sd_->open("PREFIX.CSV");
  if (err != FR_OK)
  {
    return 0;
  }

  char line[64]{0};
  uint16_t count = 0;

  while (RC6502Pgm::readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }
    count++;
  }

  return count;
}

void RC6502MenuClass::listPrefixesPage(uint16_t page_num)
{
  if (!sd_)
  {
    return;
  }

  uint16_t total = pfx_total_entries_;
  if (total == 0)
  {
    Serial.println();
    Serial.println(F("No PREFIX.CSV found on SD card."));
    state_ = State::Command;
    printPrompt();
    return;
  }

  uint16_t total_pages = (total + PGM_PER_PAGE - 1) / PGM_PER_PAGE;
  pfx_page_ = page_num;

  uint8_t err = sd_->open("PREFIX.CSV");
  if (err != FR_OK)
  {
    Serial.println();
    Serial.println(F("Failed to read PREFIX.CSV."));
    state_ = State::Command;
    printPrompt();
    return;
  }

  Serial.println();
  Serial.print(F("AVAILABLE PREFIXES (PAGE "));
  Serial.print(page_num + 1);
  Serial.print(F("/"));
  Serial.print(total_pages);
  Serial.println(F("):"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("#   PREFIX   DESCRIPTION"));
  Serial.println(F("----------------------------------------"));

  char line[64]{0};
  uint16_t idx = 0;
  uint16_t start_idx = page_num * PGM_PER_PAGE;
  uint16_t count_in_page = 0;

  while (RC6502Pgm::readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }

    if (idx >= start_idx && count_in_page < PGM_PER_PAGE)
    {
      char *ptr = trimmed;
      char *token_pfx = strsep(&ptr, ",");
      char *token_desc = strsep(&ptr, ",");

      token_pfx = RC6502Utils::trim(token_pfx);
      token_desc = RC6502Utils::trim(token_desc);

      if (token_pfx && *token_pfx)
      {
        // Format index (#)
        if (idx < 10)
        {
          Serial.print('0');
        }
        size_t n = Serial.print(idx, DEC);
        Serial.print(F("  "));

        // Format prefix name (clamped to 8 chars for 40-col fit)
        char pfx_buf[9]{0};
        strncpy(pfx_buf, token_pfx, 8);
        pfx_buf[8] = '\0';
        n = Serial.print(pfx_buf);
        if (9 > n)
        {
          RC6502Utils::printSpaces(9 - n);
        }

        // Format description (strictly truncated to 27 chars for 40-col fit)
        if (token_desc)
        {
          char desc_buf[28]{0};
          strncpy(desc_buf, token_desc, 27);
          desc_buf[27] = '\0';
          Serial.println(desc_buf);
        }
        else
        {
          Serial.println();
        }
        count_in_page++;
        if (count_in_page >= PGM_PER_PAGE)
        {
          break;
        }
      }
    }
    idx++;
  }
  Serial.println(F("----------------------------------------"));

  if (page_num + 1 < total_pages)
  {
    Serial.print(F("-- MORE ("));
    Serial.print(start_idx + count_in_page);
    Serial.print(F("/"));
    Serial.print(total);
    Serial.print(F(") -- [SPC/Q/#]: "));
    state_ = State::PrefixPaging;
    input_len_ = 0;
    input_buf_[0] = '\0';
  }
  else
  {
    Serial.print(total, DEC);
    Serial.println(F(" PREFIX(ES) LISTED."));
    state_ = State::Command;
    printPrompt();
  }
}

void RC6502MenuClass::handlePrefixPagingInput(char c)
{
  if (c == 0x1B || (input_len_ == 0 && (toupper(static_cast<unsigned char>(c)) == 'Q' || toupper(static_cast<unsigned char>(c)) == 'X')))
  {
    Serial.println();
    state_ = State::Command;
    input_len_ = 0;
    input_buf_[0] = '\0';
    printPrompt();
    return;
  }

  // Advance page on Space or Enter ONLY when no query has been typed
  if (input_len_ == 0 && (c == ' ' || c == '\r' || c == '\n'))
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }
    listPrefixesPage(pfx_page_ + 1);
    return;
  }

  // Submit typed prefix on Enter
  if (c == '\r' || c == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }

    char pfx_input[sizeof(input_buf_)];
    strncpy(pfx_input, input_buf_, sizeof(pfx_input) - 1);
    pfx_input[sizeof(pfx_input) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    executeSetPrefix(pfx_input);
    return;
  }

  if (c == '\b' || c == 0x7F)
  {
    if (input_len_ > 0)
    {
      input_len_--;
      input_buf_[input_len_] = '\0';
      Serial.print(F("\b \b"));
    }
    return;
  }

  if (isprint(static_cast<unsigned char>(c)) && input_len_ < sizeof(input_buf_) - 1)
  {
    char uc = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    input_buf_[input_len_++] = uc;
    input_buf_[input_len_] = '\0';
    Serial.write(uc);
  }
}

uint16_t RC6502MenuClass::countCatalogEntries(void)
{
  if (!sd_)
  {
    return 0;
  }

  char cat_path[32]{0};
  RC6502Pgm::buildCatalogPath(cat_path, sizeof(cat_path), prefix_);

  uint8_t err = sd_->open(cat_path);
  if (err != FR_OK)
  {
    return 0;
  }

  char line[64]{0};
  uint16_t count = 0;

  while (RC6502Pgm::readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }
    count++;
  }

  return count;
}

void RC6502MenuClass::listCatalogPage(uint16_t page_num)
{
  if (!sd_)
  {
    return;
  }

  uint16_t total = cat_total_entries_;
  if (total == 0)
  {
    Serial.println();
    Serial.print(F("No files in /"));
    Serial.print(prefix_);
    Serial.println(F("/"));
    state_ = State::Command;
    printPrompt();
    return;
  }

  uint16_t total_pages = (total + PGM_PER_PAGE - 1) / PGM_PER_PAGE;
  cat_page_ = page_num;

  char cat_path[32]{0};
  RC6502Pgm::buildCatalogPath(cat_path, sizeof(cat_path), prefix_);

  uint8_t err = sd_->open(cat_path);
  if (err != FR_OK)
  {
    Serial.println();
    Serial.print(F("NO CATALOG.CSV IN: /"));
    Serial.print(prefix_);
    Serial.println(F("/"));
    state_ = State::Command;
    printPrompt();
    return;
  }

  Serial.println();
  Serial.print(F("DIR: /"));
  Serial.print(prefix_);
  Serial.print(F("/  (PAGE "));
  Serial.print(page_num + 1);
  Serial.print(F("/"));
  Serial.print(total_pages);
  Serial.println(F(")"));

  if (terse_mode_)
  {
    Serial.println(F("----------------------------------------"));
    Serial.println(F("#   NAME                    TYPE LOAD   "));
    Serial.println(F("----------------------------------------"));
  }
  else
  {
    Serial.println(F("----------------------------------------"));
    Serial.println(F("#   NAME                TYPE LOAD  RUN  "));
    Serial.println(F("----------------------------------------"));
  }

  char line[64]{0};
  uint16_t idx = 0;
  uint16_t start_idx = page_num * PGM_PER_PAGE;
  uint16_t count_in_page = 0;
  size_t n;
  RC6502Pgm pgm_tmp;

  while (RC6502Pgm::readLine(sd_, line, sizeof(line)))
  {
    char *trimmed = RC6502Utils::trim(line);
    if (!trimmed || trimmed[0] == '#' || trimmed[0] == '\0')
    {
      continue;
    }

    if (idx >= start_idx && count_in_page < PGM_PER_PAGE)
    {
      pgm_tmp.parseCsv(trimmed);
      count_in_page++;

      // Format index (e.g. "00. ")
      if (idx < 10)
      {
        Serial.print('0');
      }
      n = Serial.print(idx, DEC);
      Serial.print(F(". "));

      if (terse_mode_)
      {
        // Name: up to 23 chars + 1 space = 24 chars
        char short_name[24]{0};
        strncpy(short_name, pgm_tmp.getDescription(), 23);
        short_name[23] = '\0';
        n = Serial.print(short_name);
        if (24 > n)
        {
          RC6502Utils::printSpaces(24 - n);
        }

        // Type: up to 4 chars + 1 space = 5 chars
        n = Serial.print(pgm_tmp.getTypeT());
        if (5 > n)
        {
          RC6502Utils::printSpaces(5 - n);
        }

        // Load: 4-digit hex zero padded
        printHex4(pgm_tmp.getLoadAddress());
        Serial.println();
      }
      else
      {
        // Name: strictly clamped to 19 chars + 1 space = 20 chars
        char short_name[20]{0};
        strncpy(short_name, pgm_tmp.getDescription(), 19);
        short_name[19] = '\0';
        n = Serial.print(short_name);
        if (20 > n)
        {
          RC6502Utils::printSpaces(20 - n);
        }

        // Type: 4 chars + 1 space = 5 chars
        n = Serial.print(pgm_tmp.getTypeT());
        if (5 > n)
        {
          RC6502Utils::printSpaces(5 - n);
        }

        // Load: 4-digit hex zero padded (4 chars) + 2 spaces = 6 chars
        printHex4(pgm_tmp.getLoadAddress());
        Serial.print(F("  "));

        // Run: 4-digit hex zero padded (4 chars)
        printHex4(pgm_tmp.getRunAddress());
        Serial.println();
      }

      if (count_in_page >= PGM_PER_PAGE)
      {
        break;
      }
    }
    idx++;
  }

  Serial.println(F("----------------------------------------"));

  if (page_num + 1 < total_pages)
  {
    Serial.print(F("-- MORE ("));
    Serial.print(start_idx + count_in_page);
    Serial.print(F("/"));
    Serial.print(total);
    Serial.print(F(") -- [SPC/Q/#/I]: "));
    state_ = State::CatalogPaging;
    input_len_ = 0;
    input_buf_[0] = '\0';
  }
  else
  {
    Serial.print(total, DEC);
    Serial.println(F(" FILE(S) LISTED."));
    state_ = State::Command;
    printPrompt();
  }
}

void RC6502MenuClass::handleCatalogPagingInput(char c)
{
  if (c == 0x1B || (input_len_ == 0 && (toupper(static_cast<unsigned char>(c)) == 'Q' || toupper(static_cast<unsigned char>(c)) == 'X')))
  {
    Serial.println();
    state_ = State::Command;
    input_len_ = 0;
    input_buf_[0] = '\0';
    printPrompt();
    return;
  }

  // Advance page on Space or Enter ONLY when no query has been typed
  if (input_len_ == 0 && (c == ' ' || c == '\r' || c == '\n'))
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }
    listCatalogPage(cat_page_ + 1);
    return;
  }

  // Submit query on Enter (Direct Load or Info lookup)
  if (c == '\r' || c == '\n')
  {
    if (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r'))
    {
      Serial.read();
    }

    char query[sizeof(input_buf_)];
    strncpy(query, input_buf_, sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';
    input_len_ = 0;
    input_buf_[0] = '\0';

    // 1. If query is just "I" or "INFO", prompt for info target
    if (strcasecmp(query, "I") == 0 || strcasecmp(query, "INFO") == 0)
    {
      doCmdInfo();
      return;
    }

    // 2. If query starts with "INFO ", route to displayProgramInfo
    if (strncasecmp(query, "INFO ", 5) == 0)
    {
      char *info_target = RC6502Utils::trim(&query[5]);
      displayProgramInfo(info_target);
      return;
    }

    // 3. If query starts with "I " or "I<digit>"
    if ((query[0] == 'I' || query[0] == 'i') && (query[1] == ' ' || isdigit(static_cast<unsigned char>(query[1]))))
    {
      char *info_target = RC6502Utils::trim(&query[1]);
      displayProgramInfo(info_target);
      return;
    }

    // 4. Otherwise, perform direct load
    executeLoad(query);
    return;
  }

  if (c == '\b' || c == 0x7F)
  {
    if (input_len_ > 0)
    {
      input_len_--;
      input_buf_[input_len_] = '\0';
      Serial.print(F("\b \b"));
    }
    return;
  }

  // Printable characters (including Space in multi-word queries)
  if (isprint(static_cast<unsigned char>(c)) && input_len_ < sizeof(input_buf_) - 1)
  {
    char uc = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    input_buf_[input_len_++] = uc;
    input_buf_[input_len_] = '\0';
    Serial.write(uc);
  }
}
