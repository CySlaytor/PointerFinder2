# Pointer Finder 2.0

![alt text](https://img.shields.io/badge/Language-C%2B%2B17-blue)
![alt text](https://img.shields.io/badge/Framework-Qt6-green)
![alt text](https://img.shields.io/badge/Platform-Windows-lightgray)

A native C++ pointer scanning utility built with Qt6, designed for game reverse engineering and retro achievement development.

<img width="769" height="602" alt="attach01" src="https://github.com/user-attachments/assets/d4024816-b1ca-44ac-b346-f8cde498ae04" />


## Features

- **Multi-Emulator Support:** Works on **PCSX2** (PS2), **DuckStation** (PS1), **Dolphin** (GameCube/Wii), **PPSSPP** (PSP), and **Nintendo DS** (via RALibretro).
- **State-Based Scanning (Differential Analysis):**
  - Captures multiple memory dumps ("states") at different execution points (e.g., Title Screen, Level 1, Level 2).
  - The scanner utilizes a backwards Breadth-First Search (BFS) to isolate stable pointer paths that remain valid across all captured states.
- **Tabbed Workspace:** Dedicated tabs organize your workflow into **Search Results**, **Bookmarks**, and **Array Detection**.
- **Array Detection & Clustered Matching:** Post-processes discovered base addresses to locate contiguous memory blocks or structured variables in the target dumps.
- **Bookmarks (Watchlist) Panel:** Direct tracking of dynamic paths in a dedicated tab view, displaying real-time address evaluation.
- **Static Range Finders:** Built-in analysis tools to scan or parse executable structures (such as Dolphin's `main.dol` or DS headers) to automatically configure scan boundaries.
- **Dynamic Filtering:** Real-time validation against live memory to automatically prune paths that fail to resolve correctly.
- **Smart Attachment & Session Management:**
  - Automatically detects running emulators. If multiple are found, it provides a clean selection prompt.
  - Save and load entire scan sessions, including results, parameters, bookmarks, and emulator attachment info using the `.pfs` session format.
- **RetroAchievements Format Export:** Copies pointer paths to the clipboard in a format ready to be pasted directly into the RetroAchievements toolkit, with intelligent defaults for memory size prefixes (`X`, `W`, `G`) and system-specific requirements (like Dolphin's address mask).
- **Automatic Update Checker:** Native update mechanism that checks for newer releases via the GitHub API.

## How to Use

1. Download the latest release from the [Releases page](https://github.com/CySlaytor/PointerFinder2/releases/).
2. Run the `PointerFinder2` executable.
3. Start your game in a supported emulator.
4. In the tool, go to **File -> Attach to Emulator...** to link with the process.
5. Click **State-Based Scan** at the bottom of the window.
6. Enter the target memory address for your value in the current game state (e.g., Slot 1) and click **Capture**.
8. Change the game state (e.g., load a save, enter a level), find the new address, enter it for the next state slot (e.g., Slot 2), and click **Capture**.
9. Click **Scan** to run the multi-state BFS scanner.

## Building from Source

This project requires a C++17 compliant compiler, Qt6, and CMake.

### Prerequisites

- CMake 3.20 or newer
- Qt6 (Core, Widgets, Concurrent, Svg, Network)
- A compatible C++17 compiler (MSVC 2022, GCC, or Clang)

### Build Steps

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

Note for Windows builds: The CMake configuration includes a post-build step that utilizes `windeployqt` to automatically package the necessary Qt dependencies inside the output directory.
