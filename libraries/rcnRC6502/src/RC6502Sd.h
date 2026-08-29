// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#ifndef RCN_RC6502_SD_H
#define RCN_RC6502_SD_H

#include <stdint.h>

#include <PetitFatFs.h>

class RC6502Sd
{
public:
  enum
  {
    MOUNT = 0,
    OPEN,
    READ,
    WRITE,
    SEEK,
  };

  bool begin(uint8_t pin_ss);
  uint8_t mount(void);
  uint8_t open(const char *file_name);
  inline uint8_t read(void *buf, uint8_t sz_to_read, uint8_t &sz_read);
  inline uint8_t write(const void *buf, uint8_t sz_to_write, uint8_t &sz_wrote);
  inline uint8_t lseek(uint32_t sz_offset);
  void printError(uint8_t error, uint8_t operation, const char *file_name = nullptr);
  void waitKey(void);

private:
  void printErrorCode(FRESULT error);
  void printOperation(uint8_t operation);
  void waitTtyRx(void);

  FATFS fatfs_;
  uint8_t pin_ss_{0};
};

inline uint8_t RC6502Sd::read(void *buf, uint8_t sz_to_read, uint8_t &sz_read_out)
{
  UINT sz_read = 0;
  FRESULT error = PetitFatFs.read(buf, sz_to_read, &sz_read);
  sz_read_out = static_cast<uint8_t>(sz_read);
  return static_cast<uint8_t>(error);
}

inline uint8_t RC6502Sd::write(const void *buf, uint8_t sz_to_write, uint8_t &sz_wrote_out)
{
  UINT sz_wrote = 0;
  FRESULT error = PetitFatFs.write(buf, sz_to_write, &sz_wrote);
  sz_wrote_out = static_cast<uint8_t>(sz_wrote);
  return static_cast<uint8_t>(error);
}

inline uint8_t RC6502Sd::lseek(uint32_t sz_offset)
{
  FRESULT error = PetitFatFs.lseek(sz_offset);
  return static_cast<uint8_t>(error);
}

#endif // RCN_RC6502_SD_H
