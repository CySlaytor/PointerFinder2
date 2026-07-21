# Pointer Finder v2.0.0 - Release Notes

Pointer Finder 2.0, had a complete rewrite of the application from C# (.NET / WinForms) to C++17 utilizing the Qt6 framework. This architectural transition establishes a portable foundation for future cross-platform development while optimizing search performance and system resource usage.

### Key Additions & Improvements

#### 1. Core Architecture Rewrite
- **C++ & Qt6 Migration:** Replaced the Windows Forms interface with a native Qt6 presentation layer.
- **CMake Build System:** Standardized project builds with CMake, allowing easier development setup and compilation.
- **Optimized Search Engine:** Redesigned the backward BFS search strategy to run with parallel thread-safe sharded pools, reducing lock contention during deep, multi-level pointer scans.
- **Fast List Rendering:** Custom list views now use stack-allocated formatting buffers to reduce memory allocations during viewport scrolling.

#### 2. New Features
- **Contiguous Array Detection:** Added an experimental "Array Detection" tab that parses memory blocks to discover structured groups of adjacent variable arrays in RAM.
- **mGBA / GBA Support:** Added full support for GBA emulation via a new RALibretro mGBA core manager.
- **Dynamic Base Detection:** Added a "Multi-Step Explore" option for platforms without fixed, traditional static address bounds, tracking pointer chains until stable roots are resolved.
- **Bookmarks:** Monitor multiple pointer paths simultaneously in a dedicated "Watchlist" tab with automatic recalculation.

#### 3. UI/UX Polish
- **Dynamic Theme Engine:** Native support for both Dark and Light visual themes.
- **Vector Icons:** Dynamic SVG icon parsing recolors interface assets on-the-fly for optimal contrast.
- **Improved Dialogs:** Converted all platform range finders (Dolphin, PPSSPP, PCSX2, NDS) into responsive Qt dialog layouts.
- **Robust Session Management:** Workspaces can be saved as JSON-based `.pfs` files, preserving bookmarks, discovered arrays, and scan states.