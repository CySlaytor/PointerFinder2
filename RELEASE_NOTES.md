# Pointer Finder v2.0.1 - Release Notes

Pointer Finder v2.0.1 is a maintenance and compatibility update following the complete C++17 / Qt6 rewrite, introducing updated memory layout signatures for Nintendo DS emulation cores in RALibretro and improving attachment reliability.

### Updates & Fixes in v2.0.1

- **DeSmuME (git 8f6b32c+) Support:** Replaced volatile committed region size detection with direct module enumeration (`desmume_libretro.dll`), resolving the exact module base address and mapping the updated internal RAM offset (`0x1120F40`).
- **melonDS Memory Footprint Update:** Added support for the updated melonDS memory allocation layout (`0x10DC000` / 17.68 MB).
- **Backward Compatibility:** Retained fallback signatures for older DeSmuME and melonDS core builds, ensuring seamless attachment across all core revisions.