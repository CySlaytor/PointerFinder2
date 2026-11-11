# Pointer Finder v2.1.0 - Release Notes

Pointer Finder v2.1.0 introduces the **Dissect Structure Workspace**, native support for **Dreamcast (Flycast)**, dynamic memory base resolution for libretro cores, and direct integration with local **RetroAchievements User.txt / Notes.json** files.

---

## What's New

### 1. Dissect Structure Workspace (`Tools -> Dissect Structure...` or `Ctrl+M`)
A full reverse engineering workspace designed to map out game structures and data tables without doing manual offset math in external tools:
- **Dissect Address Bar**: Pulls notes directly from your local RetroAchievements cache. Includes multi-keyword filtering, manual address creation, and one-click saving to `User.txt`.
- **Synchronized Structure Hierarchy**: Dual tree views show variable names on the left and Size, Offset, Resolved RAM addresses, and live Pointer targets on the right. Both panes stay locked in sync when expanding, scrolling, or selecting. Metadata columns can be toggled on/off.
- **PCSX2-Style Hex & ASCII Memory Viewer**:
  - Virtualized memory grid supporting 8-bit, 16-bit, 32-bit, 32-bit Big-Endian, Float, and Float Big-Endian.
  - Float timer formatting: translates raw floats into seconds, centiseconds, milliseconds, 50Hz frames, or 60Hz frames.
  - Color-coded structure highlights: array bounds (purple), active scopes (blue), variables (cobalt blue), and pointers (crimson red) with nibble-level pointer bitmask rendering.
  - Shift+Click range selection with a floating action banner to define arrays.
  - 50-step navigation history stack (`Backspace`, `Alt+Left`, `Alt+Right`).
  - Pinned center-width splitter that prevents the memory view from getting squished during window resizing.
- **Synchronized Code Note Editor**: Live, two-way sync between the structure hierarchy tree and the code note text.

### 2. RetroAchievements Local Storage Integration
- Automatically detects the active emulator's `RACache/Data/` folder.
- Reads `<GameID>-Notes.json` (server baseline) and writes local developer overrides to `<GameID>-User.txt`.
- Atomic writes via temporary files to avoid note corruption.
- Built-in button to revert local overrides back to the clean server note.

### 3. Clone Structure Stride (Batch Note Generator)
- Batch-generates sequential code notes for repeating tables (e.g., Level 1..N stats, Player 1..N structures, inventory arrays).
- Auto-detects structure byte stride from tags like `[144 bytes]` or `[30x16 bytes]`.
- Supports `{INDEX}`, `{INDEX:02}`, `{INDEX:03}`, `{OFFSET}`, and `{NAME}` tokens.
- Excel-style editable preview table with a button to paste names from clipboard text directly into rows.

### 4. Dreamcast (Flycast) Support
- Added scanning and memory management support for the Dreamcast via RALibretro (Flycast core).
- Full 16MB SH4 RAM access (`0x8C000000` cached space) with automatic 24-bit `W` trigger prefixes for RetroAchievements.

### 5. Dynamic libretro Memory Resolution
- Replaced fragile DLL offset heuristics and region-size checks for GBA (mGBA) and Nintendo DS (melonDS / DeSmuME).
- Uses remote execution stubs to call `retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM)` directly inside target emulator processes (x86 and x64), keeping attachments stable across emulator core updates.

---

## Usability & Interface Improvements

- **Smoother Tooltip Fade**: Replaced the default Windows tooltip popup animation with a clean, hardware-accelerated fade-in/fade-out filter. -Thanks to [PS2Hagrid](https://retroachievements.org/user/PS2Hagrid) for the report.
- **Standardized Keyboard Shortcuts**:
  - `V`: Add or edit variable
  - `P`: Add or edit pointer
  - `D`: Open value descriptions / enumerations
  - `Enter` / `F`: Follow pointer to destination in RAM
  - `H`: Highlight selected address in Structure Hierarchy
  - `Ctrl+Shift+C`: Copy as RA Trigger
  - `Ctrl+Alt+C`: Copy Value
  - `Ctrl+C`: Copy Address
  - `Delete` / `X`: Remove selected node or documentation
  - `Ctrl+S`: Save active note to `User.txt`
- **Code Note Guidelines Popup**: Added a reference dialog in Settings explaining RetroAchievements QA formatting standards (`+` for pointers, `.` for enums, `|` for arrays).
- **Auto-Populate Pointer Templates**: Option to automatically fill intermediate pointer levels with platform-appropriate templates (e.g., `[32-bit BE & 0x1FFFFFFF] Pointer`).
- **Multi-Region Memory Viewer**: Stacks multiple memory banks seamlessly in the memory viewer (Wii MEM1 + MEM2 in Dolphin, EWRAM + IWRAM in GBA).

---

## Fixes

- Fixed PCSX2 pointer trigger generation to apply the `&536870911` mask only when pointers reference the `0x2` uncached space.
- Fixed Dolphin address unnormalization when crossing boundaries between MEM1 and MEM2.
- Refactored core modules into clean, dedicated subsystems (`Core/Scanning`, `Core/CodeNotes`, `Core/Storage`, `Core/Structure`, `Core/System`).