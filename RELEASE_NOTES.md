# Pointer Finder v2.1.1 - Release Notes

Pointer Finder v2.1.1 enhances the **Dissect Structure Workspace** with seamless array template inheritance, inline array resolution, unaligned string pointer traversal, and build compatibility fixes for Qt 6.10+.

---

## What's New

- **Array Linked Structure Inheritance**:
  - Pointers defined within a declared `[NxM bytes]` array range (e.g., `[8000x4 bytes]`) now automatically consume the array's element template (`|0x0`, `|+0x4`, etc.) across the Structure Hierarchy, Memory Viewer highlights, `H` (Locate in Hierarchy), and RetroAchievements trigger generation—without duplicating sub-notes in the Code Note text.
  - Bottom-up (post-order) template resolution supports nested arrays within arrays while strictly respecting declared `[NxM bytes]` bounds.
  - Adding variables (`V`), adding/upgrading pointers (`P`), or editing value descriptions (`D`) on any array-consuming pointer or inherited child routes cleanly back to the top-level template and propagates across all entries.
- **Dynamic Array Element Projection**:
  - Clicking or following undocumented slots inside an `[NxM bytes]` array in the Memory Viewer dynamically projects the matching element template's highlights, scopes, and RA trigger paths while navigating its chain.

## Fixes & Improvements

- **Unaligned String/Byte Pointer Targets**: Allowed pointer traversal and resolution to unaligned RAM addresses when a pointer's children contain `[ASCII]`, `8-bit`, or `16-bit` data.
- **Nested Array Pipe Prefix Preservation**: Fixed `CodeNoteHierarchyEngine::rebuildTextFromTree` stripping `++` prefixes on nested `++|0x...` and `++|+0x...` lines by tracking `ancestorPipeLevel`.