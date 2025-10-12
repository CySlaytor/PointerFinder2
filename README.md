# Pointer Finder 2.0

A powerful and robust multi-emulator pointer scanning tool designed for game reverse engineering and achievement creation.

<img width="968" height="624" alt="image" src="https://github.com/user-attachments/assets/913c86b3-b46a-46a3-ae77-f392c60499b9" />

## Features

*   **Multi-Emulator Support:** Works on **PCSX2** (PS2), **DuckStation** (PS1), **Dolphin** (GameCube/Wii), and **Nintendo DS** (via RALibretro).
*   **Two Powerful Scanning Algorithms:**
    *   **Live Scan (Algorithm 1):** A fast, real-time scan on a running emulator. Perfect for initial discovery and rapid iteration.
    *   **State-Based Scan (Algorithm 2):** Capture multiple memory dumps ("states") at different points in a game. The scanner then finds ultra-stable pointers that are valid across all states, ideal for pointers that break between level loads or game resets.
*   **Static Range Finders:** Built-in tools to automatically find the static memory range for PCSX2, Dolphin, and NDS games, simplifying scan setup.
*   **Advanced Filtering & Refining:**
    *   **Refine Scan:** Intersect a new scan with your existing results to quickly narrow down candidates.
    *   **Filter Dynamic Paths:** Continuously validate results against live memory, automatically removing any pointers that have broken.
*   **Smart Attachment & Session Management:**
    *   Automatically detects running emulators. If multiple are found, it provides a clean selection prompt.
    *   Save and load entire scan sessions, including results, parameters, and emulator attachment info.
*   **Memory Management Tools:** Includes a **"Purge Memory"** function to reduce RAM usage and a **"Smart Restart"** to preserve your session while completely resetting the application.
*   **RetroAchievements Format:** Copies pointer paths to the clipboard in a format ready to be pasted directly into the RetroAchievements toolkit, with intelligent defaults for memory size prefixes (`X`, `W`, `G`) and system-specific requirements (like Dolphin's address mask).

## How to Use

1.  Download the latest release from the [Releases page](https://github.com/CySlaytor/PointerFinder2/releases/).
2.  Run `PointerFinder2.exe`.
3.  Run a game in a supported emulator.
4.  In the tool, go to **File -> Attach to Emulator...**.
5.  Choose your scanning method:
    *   **For a Live Scan:** Click **"New Pointer Scan"** to configure and start your search.
    *   **For a State-Based Scan:** Click **"Capture State"**, enter a target address for "State 1" and click **Capture**. Change the game state (e.g., load a level, enter a menu), enter the new target address for "State 2" and click **Capture**. Repeat for 2-4 states, then click **Scan**.

## Building from Source

This project was built with **Visual Studio 2022** and **.NET 8**.

1.  Clone the repository: `git clone https://github.com/CySlaytor/PointerFinder2.git`
2.  Open `PointerFinder2.sln` in Visual Studio.
3.  Build the solution.
