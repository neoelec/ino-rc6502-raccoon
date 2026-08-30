// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_PGM_H
#define RCN_RC6502_PGM_H

#include <stdint.h>
#include <stddef.h>

#include "RC6502Sd.h"

class RC6502Pgm
{
public:
  enum class Type : uint8_t
  {
    Unknown = 0,
    Hex,
    Bin,
    Bas,
    Dir
  };

  void reset(void);
  bool begin(RC6502Sd *sd);
  bool begin(RC6502Sd *sd, const char *prefix, uint16_t index);
  bool begin(RC6502Sd *sd, uint8_t dir_number, uint16_t index);
  bool find(RC6502Sd *sd, const char *prefix, const char *query);
  bool find(RC6502Sd *sd, uint8_t dir_number, const char *query);

  const char *getDescription(void) const;
  Type getType(void) const;
  const __FlashStringHelper *getTypeT(void) const;
  const char *getPgmFile(void) const;
  uint16_t getLoadAddress(void) const;
  uint16_t getRunAddress(void) const;
  void printProgram(void) const;

  static bool readLine(RC6502Sd *sd, char *buf, uint8_t max_len);
  static void buildCatalogPath(char *out_path, size_t max_len, const char *prefix);
  void parseCsv(char *csv);

private:
  void parseToken(char *token, uint8_t i);

private:
  RC6502Sd *sd_{nullptr};

  char description_[24]{0};
  Type type_{Type::Unknown};
  char pgm_file_[32]{0};
  uint16_t load_address_{0};
  uint16_t run_address_{0};
};

#endif // RCN_RC6502_PGM_H
