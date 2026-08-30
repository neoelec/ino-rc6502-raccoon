// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Loader.h"
#include "RC6502Utils.h"

static inline char toHexNibble(uint8_t nibble)
{
  nibble &= 0x0F;
  return (nibble < 10) ? static_cast<char>('0' + nibble) : static_cast<char>('A' + (nibble - 10));
}

bool RC6502Loader::load(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const RC6502Pgm &pgm)
{
  if (!sd || !kbd || !video)
  {
    return false;
  }

  const char *file_name = pgm.getPgmFile();

  if (!openFile(sd, file_name))
  {
    Serial.println();
    Serial.println(F("RCN: Could not open program file. Load aborted!"));
    return false;
  }

  Serial.println();
  Serial.print(F("RCN: Loading program ("));
  Serial.print(file_name);
  Serial.println(F(")..."));

  bool success = false;
  if (pgm.getType() == RC6502Pgm::Type::Bin)
  {
    success = streamBinaryFile(sd, kbd, video, file_name, pgm.getLoadAddress());
  }
  else
  {
    success = streamTextFile(sd, kbd, video, file_name);
  }

  if (!success)
  {
    Serial.println();
    Serial.println(F("RCN: Load aborted due to error!"));
    return false;
  }

  // Actively pump video output until 6502 echo settles (quiet for 250ms)
  drainVideo(kbd, video, 250, 2000);

  Serial.println();
  pgm.printProgram();
  Serial.println();

  return true;
}

bool RC6502Loader::openFile(RC6502Sd *sd, const char *file_name)
{
  uint8_t error = sd->open(file_name);
  if (error != FR_OK)
  {
    // Re-mount once and retry opening in case filesystem state was invalidated
    sd->mount();
    error = sd->open(file_name);
  }

  if (error != FR_OK)
  {
    sd->printError(error, RC6502Sd::Operation::Open, file_name);
    return false;
  }

  return true;
}

static uint8_t s_sd_chunk_buf[128];

bool RC6502Loader::streamTextFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name)
{
  bool empty_file = true;
  uint8_t error = FR_OK;
  uint8_t sz_read = 0;

  do
  {
    error = sd->read(s_sd_chunk_buf, sizeof(s_sd_chunk_buf), sz_read);
    if (sz_read > 0)
    {
      empty_file = false;
    }

    if (error != FR_OK)
    {
      sd->printError(error, RC6502Sd::Operation::Read, file_name);
      return false;
    }

    for (uint8_t i = 0; i < sz_read; i++)
    {
      feedCharPipelined(kbd, video, static_cast<char>(s_sd_chunk_buf[i]));
    }
  } while (sz_read == sizeof(s_sd_chunk_buf));

  if (empty_file)
  {
    Serial.println();
    Serial.println(F("RCN: Empty file - Load aborted!"));
    return false;
  }

  // Drain all remaining queued characters
  busyWaitConsole(kbd, video);

  // Send final carriage return and drain
  feedOneCharacter(kbd, '\r');
  busyWaitConsole(kbd, video);

  return true;
}

bool RC6502Loader::streamBinaryFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name, uint16_t start_address)
{
  bool empty_file = true;
  uint8_t error = FR_OK;
  uint8_t sz_read = 0;
  uint16_t current_addr = start_address;

  do
  {
    error = sd->read(s_sd_chunk_buf, sizeof(s_sd_chunk_buf), sz_read);
    if (sz_read > 0)
    {
      empty_file = false;
    }

    if (error != FR_OK)
    {
      sd->printError(error, RC6502Sd::Operation::Read, file_name);
      return false;
    }

    // Process buffer in chunks of up to 8 bytes per Woz Monitor line
    uint8_t offset = 0;
    while (offset < sz_read)
    {
      uint8_t chunk_len = ((sz_read - offset) >= 8) ? 8 : (sz_read - offset);

      // Output address and colon (e.g. "0280: ")
      feedHexAddress(kbd, video, current_addr);
      feedCharPipelined(kbd, video, ':');
      feedCharPipelined(kbd, video, ' ');

      // Output data bytes (e.g. "A9 FF 48 ...")
      for (uint8_t j = 0; j < chunk_len; j++)
      {
        feedHexByte(kbd, video, s_sd_chunk_buf[offset + j]);
        if (j + 1 < chunk_len)
        {
          feedCharPipelined(kbd, video, ' ');
        }
      }

      // End of line carriage return
      feedCharPipelined(kbd, video, '\r');

      current_addr += chunk_len;
      offset += chunk_len;
    }
  } while (sz_read == sizeof(s_sd_chunk_buf));

  if (empty_file)
  {
    Serial.println();
    Serial.println(F("RCN: Empty file - Load aborted!"));
    return false;
  }

  // Drain all remaining queued characters
  busyWaitConsole(kbd, video);

  // Send final carriage return and drain
  feedOneCharacter(kbd, '\r');
  busyWaitConsole(kbd, video);

  return true;
}

void RC6502Loader::feedHexByte(RC6502Kbd *kbd, RC6502Video *video, uint8_t val)
{
  feedCharPipelined(kbd, video, toHexNibble(val >> 4));
  feedCharPipelined(kbd, video, toHexNibble(val));
}

void RC6502Loader::feedHexAddress(RC6502Kbd *kbd, RC6502Video *video, uint16_t addr)
{
  feedCharPipelined(kbd, video, toHexNibble(static_cast<uint8_t>(addr >> 12)));
  feedCharPipelined(kbd, video, toHexNibble(static_cast<uint8_t>(addr >> 8)));
  feedCharPipelined(kbd, video, toHexNibble(static_cast<uint8_t>(addr >> 4)));
  feedCharPipelined(kbd, video, toHexNibble(static_cast<uint8_t>(addr)));
}

void RC6502Loader::feedCharPipelined(RC6502Kbd *kbd, RC6502Video *video, char c)
{
  // If the keyboard buffer is full, process FSM until space is available
  while (kbd->isBufferFull())
  {
    kbd->run();
    video->run();
  }

  feedOneCharacter(kbd, c);

  // Step the FSM forward immediately to keep transmission pipeline saturated
  kbd->run();
  video->run();
}

void RC6502Loader::feedOneCharacter(RC6502Kbd *kbd, int c)
{
  if (!kbd)
  {
    return;
  }

  c = (c == '\n') ? '\r' : c;
  kbd->pushToBuffer(c);
}

void RC6502Loader::busyWaitConsole(RC6502Kbd *kbd, RC6502Video *video)
{
  if (!kbd || !video)
  {
    return;
  }

  while (!kbd->isIdle())
  {
    kbd->run();
    video->run();
  }
}

void RC6502Loader::drainVideo(RC6502Kbd *kbd, RC6502Video *video, uint32_t quiet_ms, uint32_t max_wait_ms)
{
  if (!kbd || !video)
  {
    return;
  }

  // Ensure keyboard transmission pipeline is fully drained to Idle state
  while (!kbd->isIdle())
  {
    kbd->run();
    video->run();
  }

  // Actively pump video output until the line has been quiet for quiet_ms
  uint32_t start_ms = millis();
  uint32_t last_activity_ms = millis();

  while ((millis() - last_activity_ms < quiet_ms) && (millis() - start_ms < max_wait_ms))
  {
    kbd->run();
    if (video->run())
    {
      last_activity_ms = millis();
    }
  }
}
