// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 YOUNGJIN JOO (neoelec@gmail.com)

#include <Arduino.h>

#include "RC6502Loader.h"
#include "RC6502Utils.h"

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

  if (!streamFile(sd, kbd, video, file_name))
  {
    Serial.println();
    Serial.println(F("RCN: Load aborted due to error!"));
    return false;
  }

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

bool RC6502Loader::streamFile(RC6502Sd *sd, RC6502Kbd *kbd, RC6502Video *video, const char *file_name)
{
  bool empty_file = true;
  uint8_t buffer[128];
  uint8_t error = FR_OK;
  uint8_t sz_read = 0;

  do
  {
    error = sd->read(buffer, sizeof(buffer), sz_read);
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
      // If the keyboard buffer is full, process FSM until space is available
      while (kbd->isBufferFull())
      {
        kbd->run();
        video->run();
      }

      feedOneCharacter(kbd, buffer[i]);

      // Step the FSM forward immediately to keep transmission pipeline saturated
      kbd->run();
      video->run();
    }
  } while (sz_read == sizeof(buffer));

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

  while (!kbd->isBufferEmpty())
  {
    kbd->run();
    video->run();
  }
}
