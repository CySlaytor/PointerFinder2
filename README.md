# Pointer Finder 2.0

A powerful, multi-emulator pointer scanning tool designed for game reverse engineering and achievement creation.

<img width="968" height="624" alt="image" src="https://github.com/user-attachments/assets/913c86b3-b46a-46a3-ae77-f392c60499b9" />

## Features

*   **Multi-Emulator Support:** Works on PCSX2, DuckStation and Nintendo DS on RAlibretro v1.8.1.
*   **Profiled Settings:** All scanner settings (max level, offsets, static ranges) are saved separately for each emulator profile.
*   **Advanced Scanning:**
    *   **PCSX2:** Features an intelligent scanning algorithm with 16-byte alignment option.
    *   **DuckStation:** Uses a lean, fast scanner optimized for the PS1's 2MB RAM block.
    *   **Nintendo DS:** Uses a new scanning strategy, slighlty similar to the PS1, with an interactive range slider for easy, visual selection of the static address range.
*   **Smart Attach/Auto-Detach:** Automatically detects and disconnects running emulators. If multiple are found, it provides a clean selection prompt.
*   **RetroAchievements Format:** Copies pointer paths to the clipboard in a format ready to be pasted directly into the RetroAchievements toolkit.

## How to Use

1.  Download the latest release from the [Releases page](https://github.com/CySlaytor/PointerFinder2/releases/)
2.  Run `PointerFinder2.exe`.
3.  Run a game in a supported emulator.
4.  In the tool, go to **File -> Attach to Emulator...**.
5.  Click **"New Live Pointer Scan"** to configure and start your search.

## Building from Source

This project was built with **Visual Studio 2022** and **.NET Framework 8**

1.  Clone the repository: `git clone https://github.com/CySlaytor/PointerFinder2.git`
2.  Open `PointerFinder2.sln` in Visual Studio.
3.  Build the solution.
