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
  enum
  {
    TYPE_UNKNOWN,
    TYPE_HEX,
    TYPE_BIN
  };

  bool begin(RC6502Sd *sd);
  bool begin(RC6502Sd *sd, uint8_t dir_number, uint16_t pgm_number);
  bool begin(RC6502Sd *sd, const char *csv_name);

  const char *getDescription(void) const;
  uint8_t getType(void) const;
  const char *getTypeT(void) const;
  const char *getPgmFile(void) const;
  uint16_t getLoadAddress(void) const;
  uint16_t getRunAddress(void) const;
  void printProgram(void) const;

private:
  bool openCsv(const char *csv_name);
  bool readCsv(char *csv, uint8_t sz_csv, const char *csv_name);
  void parseCsv(char *csv);
  void parseToken(char *token, uint8_t i);
  bool beginPgmNumber(uint8_t dir_number, uint16_t pgm_number);
  bool beginCsvName(const char *csv_name);
  void updateCsvName(char *csv_name, uint8_t dir_number, uint16_t pgm_number);

private:
  RC6502Sd *sd_{nullptr};

  char description_[24]{0};
  uint8_t type_{TYPE_UNKNOWN};
  char type_t_[8]{0};
  char pgm_file_[16]{0};
  uint16_t load_address_{0};
  uint16_t run_address_{0};
};

#endif // RCN_RC6502_PGM_H
