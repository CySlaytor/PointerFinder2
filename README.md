# Pointer Finder 2.0

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue)
![Framework](https://img.shields.io/badge/Framework-Qt6-green)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgray)

A native C++ pointer scanning and reverse engineering utility built with Qt6, designed for game data discovery, structure mapping, and RetroAchievements development.

<img width="769" height="602" alt="attach01" src="https://github.com/user-attachments/assets/d4024816-b1ca-44ac-b346-f8cde498ae04" />

---

## Supported Emulators

- **PCSX2** (PlayStation 2)
- **DuckStation** (PlayStation 1)
- **Dolphin** (GameCube & Wii — MEM1 + MEM2)
- **PPSSPP** (PlayStation Portable)
- **RALibretro / Standalone Flycast** (Sega Dreamcast)
- **RALibretro** (Game Boy Advance via mGBA, Nintendo DS via melonDS / DeSmuME)

---

## Features

- **State-Based Differential Scanning:**
  - Capture multiple memory dumps ("states") at different execution points (e.g., Title Screen, Level 1, Level 2).
  - Uses reverse Breadth-First Search (BFS) to find stable, multi-level pointer paths that remain valid across every state.
  - Includes **Dynamic Static Detection** mode for architectures with floating/relocated memory bases (e.g., GBA).

- **Dissect Structure Workspace (`Ctrl+M`):**
  - Integrated 3-pane structure editor and virtualized memory viewer built like the PCSX2 Debugger Memory View.
  - **Live Code Note Sync**: Edit notes visually in an outline tree or as raw text with automatic two-way updating.
  - **RetroAchievements Storage Integration**: Loads `<GameID>-Notes.json` (server baseline) and commits overrides to `<GameID>-User.txt`.
  - **Context-Aware Memory Highlighting**: Highlights array boundaries, parent scopes, documented variables, and pointers with bitmask-aware nibble rendering.
  - **50-Step Navigation History**: Step back and forth through followed pointers (`Backspace` / `Alt+Left` / `Alt+Right`).
  - **Batch Stride Note Cloner**: Quickly duplicate repeating data structures (e.g., Level 1..N tables, character structs) with clipboard name pasting.

- **Array Detection & Clustered Matching:**
  - Scans base addresses to identify sequential pointer arrays and data structures in RAM.

- **Static Range Finders:**
  - Built-in utilities to extract static boundaries from executable headers (Dolphin `main.dol`, NDS ROM headers) or live RAM scans (PCSX2, PPSSPP).

- **RetroAchievements Workflow Tools:**
  - One-click clipboard export for the RA achievement editor (`I:0x...`).
  - Code Note Converter with automatic trigger paste detection.
  - Code Note Hierarchy Fixer supporting plusses (`+`), dots (`.`), and Unicode box characters (`├─`, `└─`).

---

## Workspace Keyboard Shortcuts

### Dissect Structure (`Ctrl+M`)
| Shortcut | Action |
| :--- | :--- |
| `V` | Add or Edit Variable at cursor |
| `P` | Add or Edit Pointer at cursor |
| `D` | Manage Value Descriptions / Enumerations |
| `Enter` / `F` | Follow Pointer destination in RAM |
| `H` | Locate cursor address in Structure Hierarchy |
| `Backspace` / `Alt+Left` | Navigate Back in memory history |
| `Alt+Right` | Navigate Forward in memory history |
| `Shift + Click` | Select contiguous byte range (Define Array) |
| `Ctrl+Shift+C` | Copy as RetroAchievements Trigger |
| `Ctrl+Alt+C` | Copy Value at address |
| `Ctrl+C` | Copy Address |
| `Delete` / `X` | Remove selected node / documentation |
| `Ctrl+S` | Save active notes to `User.txt` |
| `Ctrl+N` | Create new structure at a custom address |

---

## Usage Guide

### 1. State-Based Pointer Scanning
1. Start your game in a supported emulator.
2. In Pointer Finder, go to **File -> Attach to Emulator...**.
3. Click **State-Based Scan** at the bottom of the main window.
4. Enter your value's address in the current state (Slot 1) and click **Capture**.
5. Advance the game state (change levels, reload a room, or restart the game), find the new address, enter it in Slot 2, and click **Capture**.
6. Set your candidate and search limits, then click **Scan**.
7. Right-click any result to copy it in RA trigger format, build a code note, or send it to your **Bookmarks**.

### 2. Dissecting Structures
1. While attached to an emulator running a game, press `Ctrl+M` or go to **Tools -> Dissect Structure...**.
2. Pick an existing code note from the dropdown or click **New Address...** (`Ctrl+N`).
3. Click **Refresh RAM** to take a live snapshot of the emulator's memory banks.
4. Browse the hex view, click an address, and hit `V` (Variable) or `P` (Pointer) to document offsets.
5. Hit `Enter` or `F` on any pointer to jump directly to its target address.
6. Click **Save to User.txt** (`Ctrl+S`) to write your changes to your local RetroAchievements cache.

---

## Building from Source

### Prerequisites
- **CMake 3.20** or newer
- **Qt 6.x** (`Core`, `Widgets`, `Concurrent`, `Svg`, `Network`)
- A C++17 compliant compiler (MSVC 2022 on Windows)

### Build Steps (Windows)

1. Clone the repository:
```
git clone https://github.com/CySlaytor/PointerFinder2.git
cd PointerFinder2
```

2. Create a build directory:
```
mkdir build
cd build
```

3. Configure and build the project with CMake:
```
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" # Adjust path to your Qt installation
cmake --build . --config Release
```

Note for Windows builds: The CMake configuration includes a post-build step that utilizes `windeployqt` to automatically pa