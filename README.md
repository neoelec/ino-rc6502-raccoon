# ino-rc6502-raccoon 🦝

[![Arduino](https://img.shields.io/badge/Platform-Arduino%20AVR-blue.svg)](https://www.arduino.cc/)
[![Target: Arduino Nano](https://img.shields.io/badge/MCU-ATmega328P%20%40%2016MHz-00979D.svg)](https://store.arduino.cc/products/arduino-nano)
[![Compatibility: RC6502](https://img.shields.io/badge/Hardware-RC6502%20Apple%201%20Replica-orange.svg)](https://github.com/neoelec/ino-rc6502-raccoon)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.txt)

**`ino-rc6502-raccoon`** is an advanced, high-performance Parallel I/O (PIO) controller firmware and peripheral expansion suite for the **RC6502 Apple 1 Replica** computer. Running on an **Arduino Nano (ATmega328P)**, it transforms serial communication into hardware-synchronized Apple 1 keyboard/video signals, generates an on-board 1MHz system clock, controls CPU reset lines, and provides a CFFA-1 style Micro-SD card program loader with an interactive serial console menu.

---

## 🌟 Key Features

- **Dual Operational Modes**:
  - **Classic Mode**: Standard PIO terminal relay without hardware modifications.
  - **Modded Mode**: Full-featured mode with Micro-SD card storage, on-board 1MHz clock generator, hardware reset control, and interactive CFFA-1 menu.
- **Hardware-Generated 1MHz System Clock & Reset**:
  - Replaces the external 1MHz crystal oscillator (X14) using Timer1 CTC mode (`OCR1A = 7`, 1MHz square wave on D9).
  - Software-controlled CPU hardware reset pulse generation (D8) with pull-up auto-release.
- **Dual Interrupt-Driven Keyboard & Video Handshake**:
  - Full hardware interrupt synchronization (`INT0` on D2 for Keyboard Clear, `INT1` on D3 for Video Data Available) with state machines for reliable timing without character drops.
- **Multi-Format Micro-SD Program Loader (Petit FatFs)**:
  - Rapid, automated program injection into 6502 memory for Woz Monitor format HEX and BASIC listings.
  - **On-The-Fly Binary Conversion (`Type::Bin`)**: Directly streams raw 6502 machine code binaries (`.bin`) from SD card into Woz Monitor format (`<ADDR>: XX XX ...\r`), saving up to 73% SD card storage.
  - Consolidated directory catalog tables (`CATALOG.CSV`) and master volume prefix indexing (`PREFIX.CSV`).
- **Interactive CFFA-1 Console Menu (`Ctrl+R`)**:
  - Compact, authentic CFFA-1 storage shell for prefix navigation, full catalog listing, fast program loading (by name or number), live 6502 memory inspection (`M`), terse display toggling (`T`), warm resetting (`R`), and PIO watchdog reset (`W`).
- **Multi-Bank Custom ROM Integration**:
  - Full support for bank-switched ROMs containing Integer BASIC, Krusader Assembler, Apple-II Monitor, Ohio Scientific BASIC, AppleSoft BASIC Lite, and Woz Monitor.

---

## 🏗️ System Architecture

```mermaid
flowchart TD
    subgraph Host ["Host PC / Terminal Emulator"]
        TTY["Serial Console (115200 baud, 8N1, ANSI/VT100)"]
    end

    subgraph Controller ["Arduino Nano PIO Controller (ino-rc6502-raccoon)"]
        Main["Main Loop (RC6502.ino)"]
        Pio["RC6502Pio (Mode & State Manager)"]
        Menu["RC6502Menu (CFFA-1 Console Shell)"]
        Loader["RC6502Loader (Stream Injection Service)"]
        KbdFSM["RC6502Kbd (RingBuf & FSM Engine)"]
        VideoFSM["RC6502Video (Display Capture & TTY)"]
        ClockGen["RC6502Clock (Timer1 1MHz & Reset)"]
        Storage["RC6502Sd & RC6502Pgm (PetitFatFs & Catalog)"]
    end

    subgraph Expander ["I/O Expander & Storage"]
        MCP["MCP23S17 SPI I/O Expander<br/>Port A: Video In / Port B: KBD Out"]
        SDCard["Micro-SD Card Module (SPI CS: D7)"]
    end

    subgraph RC6502 ["RC6502 Apple 1 Computer (MOS 6502)"]
        CPU["MOS 6502 CPU (1MHz Clock on Pin 9)"]
        ROM["Custom ROM (Bank-switched 27C512/28C256)"]
        RAM["SRAM (Up to 64KB)"]
        PIA["6821 PIA / Bus Interface"]
    end

    TTY <-->|Serial 115200 bps| Pio
    Main --> Pio
    Pio --> Menu
    Pio --> KbdFSM
    Pio --> VideoFSM
    Pio --> ClockGen
    Menu --> Loader
    Loader --> Storage
    Loader --> KbdFSM
    Loader --> VideoFSM

    KbdFSM -->|Strobe D4 / INT0 D2| PIA
    PIA -->|DA D3 / nRDA D5| VideoFSM
    ClockGen -->|1MHz Clock D9| CPU
    ClockGen -->|nRESET D8| CPU

    KbdFSM <-->|SPI CS: D10| MCP
    VideoFSM <-->|SPI CS: D10| MCP
    Storage <-->|SPI CS: D7| SDCard

    MCP <--> PIA
    PIA <--> CPU
    CPU <--> ROM
    CPU <--> RAM
```

### Keyboard Strobe & Handshake FSM

```mermaid
stateDiagram-v2
    direction LR

    [*] --> Idle
    Idle --> Idle: isBufferEmpty()
    Idle --> Write: !isBufferEmpty()
    Write --> WaitInt: Strobe HIGH
    WaitInt --> WaitInt: !interrupt_ (< 100ms)
    WaitInt --> PollClear: interrupt_ / Strobe LOW
    WaitInt --> Timeout: Timeout (> 100ms)
    PollClear --> PollClear: PIN_KBD_CLR != LOW (< 200ms)
    PollClear --> Idle: PIN_KBD_CLR == LOW
    PollClear --> Timeout: Timeout (> 200ms)
    Timeout --> Idle: Reset Strobe & Clear Flags
```

---

## ⚡ Performance & Architectural Highlights

- **Multi-Format Polymorphic Loader**: Dynamically supports direct ASCII streaming (`Type::Hex`, `Type::Bas`) and on-the-fly binary formatting (`Type::Bin`) with zero dynamic memory allocation.
- **Pipelined Stream Injection**: SD file data streams directly through a 64-byte RingBuffer pipeline with real-time video echo draining, keeping the 6821 PIA transmission line fully saturated without buffer overruns.
- **128-Byte SD Chunk Buffer**: Enlarged SD sector read buffer reduces Petit FatFs `CMD17` SPI block transactions by 50%.
- **8MHz Hardware SPI Acceleration**: Uses maximum AVR SPI clock speed (`SPI_CLOCK_DIV2`, 8MHz) with [ino-PetitFatFs-raccoon](https://github.com/neoelec/ino-PetitFatFs-raccoon) for 2x storage throughput.
- **Fast File Open**: Direct root directory file traversal eliminates redundant filesystem re-mounts with automated fallback recovery.
- **Non-Blocking Interrupt-Driven Video/Keyboard Handshake**: Fully compliant with the 1MHz MOS 6502 / 6821 PIA timing specifications.
- **Hardened FSM Safeguards**: 100ms and 200ms timeout guards prevent firmware freeze if the target 6502 CPU halts.
- **Watchdog Bootloop Protection**: Early `MCUSR` clear and `wdt_disable()` prevents infinite reboot loops on legacy Arduino bootloaders.
- **Clean Decoupled Architecture**: Scoped enums (`enum class`), single-responsibility `RC6502Loader`, and encapsulated auto-detect `RC6502Pio.begin()`.

---

## 🔌 Hardware Modification & Pin Mapping

### Arduino Nano Pinout

| Pin | Function / Constant | Direction | Connected To | Description |
| :--- | :--- | :---: | :--- | :--- |
| **D2** | `PIN_KBD_CLR` | INPUT | 6821 PIA / KBD Ready | `INT0` External interrupt for keyboard handshake |
| **D3** | `PIN_VIDEO_DA` | INPUT | 6821 PIA / Video DA | `INT1` External interrupt for Video Data Available |
| **D4** | `PIN_KBD_STR` | OUTPUT | 6821 PIA / KBD Strobe | Keyboard Data Available strobe |
| **D5** | `PIN_VIDEO_nRDA` | OUTPUT | 6821 PIA / Video nRDA | Video Read Data Acknowledge |
| **D7** | `PIN_SD_nSS` | OUTPUT | Micro-SD Card Module | SPI Chip Select for SD card |
| **D8** | `PIN_nRESET` | OUTPUT (Hi-Z) | 6502 CPU `nRESET` | Hardware Reset line (pulled low during reset) |
| **D9** | `PIN_CLK_1MHZ` | OUTPUT | 6502 CPU `CLK` / X14 | Timer1 OC1A 1MHz square wave clock |
| **D10** | `PIN_MCP23S17_nSS`| OUTPUT | MCP23S17 Expander | SPI Chip Select for MCP23S17 |
| **D11** | `SPI MOSI` | OUTPUT | MCP23S17 / Micro-SD | Shared SPI Master Out Slave In |
| **D12** | `SPI MISO` | INPUT | MCP23S17 / Micro-SD | Shared SPI Master In Slave Out |
| **D13** | `SPI SCK` | OUTPUT | MCP23S17 / Micro-SD | Shared SPI Serial Clock |
| **A7** | `PIN_PIO_MODE` | INPUT (Analog) | Mode Select Switch | $\le 512$: Classic Mode, $> 512$: Modded Mode |

### MCP23S17 Expander Pin Mapping

| Port | Pins | Direction | Description |
| :--- | :--- | :---: | :--- |
| **Port A** | `GPA0 ~ GPA6` | INPUT | Video Output 7-bit ASCII Data (`RC6502 -> Arduino`) |
| **Port B** | `GPB0 ~ GPB6` | OUTPUT | Keyboard Input 7-bit ASCII Data (`Arduino -> RC6502`) |
| **Port B** | `GPB7` | OUTPUT | Keyboard Data Available Flag (High bit `0x80`) |

### Hardware Modification Steps

1. **Remove 1MHz Clock Oscillator (`X14`)**:
   - De-solder and remove the standalone 1MHz oscillator `X14`.
   - Install the **Micro-SD Card Slot** and **3.3V Level Shifter** in this space.
2. **Clock Line Connection**:
   - Connect Arduino Nano **Pin 9** (`PIN_CLK_1MHZ`) to the RC6502 main board `CLOCK` line (a 10 $\Omega$ series resistor is recommended).
3. **Reset Line Connection**:
   - Connect Arduino Nano **Pin 8** (`PIN_nRESET`) to the 6502 CPU `RESET` line.
4. **Micro-SD Card Module Wiring**:
   - Connect SPI lines (`MOSI: D11`, `MISO: D12`, `SCK: D13`, `CS: D7`, `VCC: 5V`, `GND`) via the 3.3V level shifter.

### Modification Photos

| Overview | Clock & Reset Wiring |
| :---: | :---: |
| ![Mod Overview](./images/20220916_210743.jpg) | ![Wiring Detail 1](./images/20220916_210821.jpg) |
| **SD Module & Level Shifter** | **Installed Board Bottom** |
| ![Wiring Detail 2](./images/20220916_210836.jpg) | ![Board Assembly](./images/20220916_210955.jpg) |

---

## 🚀 Quick Start & Usage

### 1. Prerequisites & Required Libraries

Install the following libraries in your Arduino IDE / CLI environment:
- **[ino-PetitFatFs-raccoon](https://github.com/neoelec/ino-PetitFatFs-raccoon)** (Hardware SPI 8MHz Petit FatFs wrapper)
- **Adafruit MCP23017 Arduino Library** (`Adafruit_MCP23X17.h`)
- **RingBuffer** (`RingBuf.h`)

### 2. Building and Uploading

Open [`examples/RC6502/RC6502.ino`](examples/RC6502/RC6502.ino) in the Arduino IDE:
1. Select Board: **Arduino Nano**
2. Select Processor: **ATmega328P** (or ATmega328P Old Bootloader)
3. Compile and Upload.

```cpp
#include <rcnRC6502.h>

void setup(void)
{
  RC6502Pio.begin();
}

void loop(void)
{
  RC6502Pio.run();
}
```

### 3. Serial Terminal Connection

Connect via any serial terminal emulator (e.g. Minicom, PuTTY, screen, miniterm):
- **Baud Rate**: `115200`
- **Data Bits**: `8`
- **Parity**: `None`
- **Stop Bits**: `1`
- **Flow Control**: `None`

### 4. Interactive Console Menu (`Ctrl+R` - CFFA-1 Style)

Press <kbd>Ctrl</kbd> + <kbd>R</kbd> at any time in the terminal to enter the CFFA-1 Storage Menu:

```text
========================================
           CFFA-1 FOR RC6502            
========================================
  C - CATALOG
  P - PREFIX
  L - LOAD FILE
  I - PROGRAM INFO
  M - MEMORY DISPLAY
  T - TOGGLE TERSE MODE
  R - WARM RESET (CPU)
  W - PIO RESET (MCU)
  Q - QUIT TO MONITOR
  ? - HELP

PFX: /SYSTEM/ | MODE: VERBOSE

ENTER SELECTION: 
```

| Command | Action | Description |
| :---: | :--- | :--- |
| `C` | **Catalog** | 16-line paginated catalog in 40-column standard format (`-- MORE --` direct load supported) |
| `P` | **Prefix** | Set directory prefix (`SYSTEM`, `GAMES`, `EXTBAS`, `00`, `01`, `02`, or `?` for list) |
| `L` | **Load File** | Enter file name (e.g. `MICROCHE`) or program number to inject into 6502 memory |
| `I` | **Program Info** | Display rich 40-column information card with file path, load/run addresses, and run hint |
| `M` | **Memory Display**| Enter memory address range (e.g. `1000.10FF`) to inspect 6502 memory contents |
| `T` | **Terse Mode** | Toggle between Standard (40-col) and Compact (40-col) catalog listing |
| `R` | **Warm Reset** | Pulse 6502 hardware reset line and reset I/O expander |
| `W` | **PIO Reset** | Trigger Arduino watchdog timer (`WDTO_15MS`) to reboot PIO controller firmware |
| `Q` / `X` | **Quit** | Exit menu and return to Woz Monitor (`\`) mode |
| `?` / `H` | **Help** | Display CFFA-1 command menu |

---

## 💾 Storage Catalog & Custom ROM Banks

### SD Card Contents Organization

The SD card is structured with a root master prefix index (`PREFIX.CSV`) and directory-level catalog tables (`CATALOG.CSV`):

```text
SD Card Root /
├── PREFIX.CSV          <-- Master prefix/directory catalog
├── SYSTEM/             <-- Machine code binaries, assemblers, monitors, and languages (Index: 00)
│   ├── CATALOG.CSV     <-- Directory catalog table
│   ├── APPLE30T.TXT
│   └── MICROCHE.TXT
├── GAMES/              <-- Integer BASIC applications and retro games (Index: 01)
│   ├── CATALOG.CSV
│   ├── HAMURABI.TXT
│   └── 21.TXT
└── EXTBAS/             <-- Extended BASIC programs (Index: 02)
    ├── CATALOG.CSV
    └── ELIZA.BAS
```

### ➕ Adding New Programs to SD Card (`CATALOG.CSV`)

To add programs to a category folder, simply copy your data files into that directory and append entries into the directory's **`CATALOG.CSV`** (single unified CSV per directory).

#### 1. Directory Catalog File Format (`CATALOG.CSV`)

```csv
<PROGRAM_NAME>,<TYPE>,<FILE_PATH>,<LOAD_ADDRESS_HEX>,<RUN_ADDRESS_HEX>
```

| Field | Description | Constraints & Examples |
| :--- | :--- | :--- |
| **`PROGRAM_NAME`** | Display name shown in the interactive menu | Max 23 characters (up to 19 chars in Verbose mode, 23 in Terse mode) |
| **`TYPE`** | Storage and stream injection format | `HEX` (Woz Hex Dump), `BAS` or `BAS/W` (BASIC Dump), `BIN` (Raw Binary), `DIR` (Subdirectory) |
| **`FILE_PATH`** | Relative path to the data file on the SD card | 8.3 uppercase format (e.g., `SYSTEM/APPLE30T.TXT`, `SYSTEM/MICROCHE.TXT`) |
| **`LOAD_ADDRESS`** | 16-bit start memory address in Hexadecimal | e.g., `0280`, `004A`, `1000`, `$1000` |
| **`RUN_ADDRESS`** | 16-bit entry point / run address in Hexadecimal | e.g., `0280`, `E2B3` (BASIC Warm Entry), `E000` |

#### 2. Root Prefix Index Format (`PREFIX.CSV`)

```csv
<PREFIX_DIR>,<DESCRIPTION>
```
Example:
```csv
SYSTEM,System Soft & Languages
GAMES,Integer BASIC Games
EXTBAS,Extended BASIC Programs
```
> [!NOTE]
> Each row order in `PREFIX.CSV` (0, 1, 2...) is dynamically mapped to numeric index shortcuts (`00`, `01`, `02`, etc.). Users can switch directories at the `P` prompt by entering either the directory name (e.g. `SYSTEM`) or its numeric index (`00`).

---

#### 3. Supported Formats & Usage

##### A. Woz Monitor ASCII Hex Format (`Type::Hex` / `.TXT`)
* **Format**: Standard Apple 1 Woz Monitor ASCII text dump containing `<ADDR>: <HEX_BYTES>` lines.
* **Loader Mechanism**: Streams ASCII characters directly through the keyboard buffer into the Woz Monitor.
* **CSV Example**:
  ```csv
  APPLE 30TH,HEX,SYSTEM/APPLE30T.TXT,0280,0280
  ```
* **Data File Example (`SYSTEM/APPLE30T.TXT`)**:
  ```text
  0280: A2 0C BD 8B 02 20 EF FF
  0288: CA D0 F7 60 8D C4 CC D2
  ```
* **Execution**: In Woz Monitor, type `0280 R` (or `280R`) and press Enter.

##### B. Integer BASIC Memory Dump (`BAS` / `BAS/W` / `.TXT`)
* **Format**: Tokenized Integer BASIC program dump starting at `$004A` (Zero Page variable/program storage area) or BASIC source commands.
* **Loader Mechanism**: Streams text directly into memory via Woz Monitor.
* **CSV Example**:
  ```csv
  21,BAS/W,GAMES/21.TXT,004A,E2B3
  ```
* **Data File Example (`GAMES/21.TXT`)**:
  ```text
  004A:       00 08 00 10 CA 0E
  0050: FF FF FF FF FF FF FF FF
  ```
* **Execution**: In Woz Monitor, jump to the Integer BASIC warm-start vector `E2B3 R`, then type `RUN` or interact with the running program.

##### C. Raw 6502 Machine Code Binary (`Type::Bin` / `.BIN`)
* **Format**: Raw binary file compiled with 6502 assemblers/compilers (e.g., `ca65`/`ld65`, `vasm`, `cc65`).
* **Loader Mechanism**: `RC6502Loader` reads raw bytes directly from the SD card and dynamically formats them into Woz Monitor hex lines (`<ADDR>: XX XX ...\r`, 8 bytes per line) on-the-fly.
* **Key Benefits**: Up to **73% smaller file size** on the SD card and significantly faster transfer.
* **CSV Example**:
  ```csv
  MICROCHESS,BIN,SYSTEM/MCHESS.BIN,0280,0280
  ```
* **Execution**: In Woz Monitor, type `0280 R` and press Enter.

---

#### 4. Step-by-Step Guide to Add a New Program

1. **Prepare Program File**:
   * For binary: Assemble your 6502 code to a raw `.bin` file (e.g., `vasm6502_oldstyle -Fbin -dotdir myprog.asm -o MYPROG.BIN`).
   * For Woz Hex / BASIC: Save as an ASCII `.TXT` file.
   * Copy the file to the target category folder on the Micro-SD card (e.g., `contents/SYSTEM/MYPROG.BIN`).
2. **Add Entry to `CATALOG.CSV`**:
   * Open the directory's `CATALOG.CSV` (e.g., `contents/SYSTEM/CATALOG.CSV`).
   * Append a new line with the 5 comma-separated fields:
     ```csv
     MY COOL PROG,BIN,SYSTEM/MYPROG.BIN,0280,0280
     ```
3. **Load and Run via Interactive CFFA-1 Menu**:
   * Insert the Micro-SD card into the RC6502 Raccoon board.
   * In the serial console, press <kbd>Ctrl</kbd> + <kbd>R</kbd> to open the CFFA-1 menu.
   * Press `P` to select directory prefix (e.g., `SYSTEM`, `GAMES`, `00`), `C` to list catalog, then `L` and enter the program name (e.g., `MY COOL PROG`) or number.
   * The firmware will automatically inject the code into 6502 memory.
   * Press `Q` to exit to Woz Monitor and run the entry vector (e.g., `0280R`).

### Raccoon's Custom ROM (`rom/rc6502_crom_00`)

The ROM binary image is located at [`rom/rc6502_crom_00/rc6502_crom_00.bin`](rom/rc6502_crom_00/rc6502_crom_00.bin). It provides 4 switchable ROM banks controlled via address lines **A13** and **A14**:

| A13 | A14 | Active Program Suite | Command to Run |
| :---: | :---: | :--- | :--- |
| **R** | **R** | Integer Basic<br>Krusader Assembler<br>Woz Monitor | `E000 R`<br>`F000 R`<br>`FF00 R` |
| **L** | **R** | Integer Basic<br>Apple-II Monitor<br>Woz Monitor | `E000 R`<br>`FE65 R`<br>`FF00 R` |
| **R** | **L** | Ohio Scientific Basic<br>Woz Monitor | `FD0D R`<br>`FF00 R` |
| **L** | **L** | AppleSoft Basic Lite<br>Woz Monitor | `E000 R`<br>`FF00 R` |

---

## 📸 Screenshots

| Woz Monitor Boot | Menu & Program Selection |
| :---: | :---: |
| ![Boot](./images/rc6502-00.png) | ![Menu](./images/rc6502-01.png) |
| **Loading Program from SD** | **Integer BASIC Execution** |
| ![Loading](./images/rc6502-02.png) | ![Basic](./images/rc6502-03.png) |
| **Integer BASIC Executionr** | **Extended BASIC Execution** |
| ![Krusader](./images/rc6502-04.png) | ![Disassembler](./images/rc6502-05.png) |
| **Extended BASIC Execution** | |
| ![Game](./images/rc6502-06.png) | |

---

## 📄 License

This project is licensed under the **MIT License**. See [LICENSE.txt](LICENSE.txt) for full details.
