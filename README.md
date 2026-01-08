# Pointer Finder 2.0

A powerful and robust multi-emulator pointer scanning tool designed for game reverse engineering and achievement creation.

<img width="968" height="624" alt="image" src="https://github.com/user-attachments/assets/913c86b3-b46a-46a3-ae77-f392c60499b9" />

## Features

*   **Multi-Emulator Support:** Works on **PCSX2** (PS2), **DuckStation** (PS1), **Dolphin** (GameCube/Wii), **PPSSPP** (PSP), and **Nintendo DS** (via RALibretro).
*   **State-Based Scanning (Differential Analysis):**
    *   Instead of guessing based on one moment in time, PF2 captures multiple memory dumps ("states") at different points in a game (e.g., Title Screen, Level 1, Level 2).
    *   The scanner finds ultra-stable pointers that are guaranteed to be valid across all captured states.
*   **Static Range Finders:** Built-in tools to automatically find the static memory range for PCSX2, Dolphin, PPSSPP, and NDS games, simplifying scan setup.
*   **Dynamic Filtering:**
    *   **Filter Dynamic Paths:** Continuously validate your results against live emulator memory, automatically removing any pointers that break in real-time.
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
5.  Click **"State-Based Scan"** (formerly Capture State).
6.  **Capture States:**
    *   Enter the target address for your value in the current game state for **State 1** and click **Capture**.
    *   Change the game state (e.g., load a save, enter a level), find the new address, enter it for **State 2**, and click **Capture**.
    *   (Optional) Repeat for 3 or 4 states for maximum accuracy.
7.  Click **Scan** to find paths that link all your captured states together.

## Building from Source

This project was built with **Visual Studio 2022** and **.NET 8**.

1.  Clone the repository: `git clone https://github.com/CySlaytor/PointerFinder2.git`
2.  Open `PointerFinder2.sln` in Visual Studio.
3.  Build the solution.
