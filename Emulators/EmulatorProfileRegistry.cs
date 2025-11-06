using PointerFinder2.Emulators.EmulatorManager;
using PointerFinder2.Emulators.LiveScan.Dolphin;
using PointerFinder2.Emulators.LiveScan.DuckStation;
using PointerFinder2.Emulators.LiveScan.PCSX2;
// Added using statements for the new PPSSPP scanner strategies.
using PointerFinder2.Emulators.LiveScan.PPSSPP;
using PointerFinder2.Emulators.LiveScan.RALibretro;
using PointerFinder2.Emulators.StateBased.Dolphin;
using PointerFinder2.Emulators.StateBased.DuckStation;
using PointerFinder2.Emulators.StateBased.PCSX2;
using PointerFinder2.Emulators.StateBased.PPSSPP;
using PointerFinder2.Emulators.StateBased.RALibretro;
// Added using statement for the static range finder forms.
using PointerFinder2.UI.StaticRangeFinders;
using System;
using System.Collections.Generic;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace PointerFinder2.Emulators
{
    // A central registry for all supported emulator profiles.
    // This is the main place to add or modify emulator support. To add a new
    // emulator, create its Manager and Scanner classes, then add a new
    // EmulatorProfile entry to this list.
    public static class EmulatorProfileRegistry
    {
        public static readonly List<EmulatorProfile> Profiles = new List<EmulatorProfile>
        {
            // Specify that this profile supports a static range finder and provide its factory.
            new EmulatorProfile
            {
                Name = "PCSX2",
                Target = EmulatorTarget.PCSX2,
                ProcessNames = new[] { "pcsx2-qt", "pcsx2" },
                ManagerFactory = () => new Pcsx2Manager(),
                ScannerFactory = () => new Pcsx2ScannerStrategy(),
                StateBasedScannerFactory = () => new Pcsx2StateBasedScannerStrategy(),
                SupportsStaticRangeFinder = true,
                StaticRangeFinderFactory = (manager) => new Pcsx2RamScanRangeFinderForm(manager)
            },
            new EmulatorProfile
            {
                Name = "DuckStation",
                Target = EmulatorTarget.DuckStation,
                ProcessNames = new[] { "duckstation-qt-x64-ReleaseLTCG" },
                ManagerFactory = () => new DuckStationManager(),
                ScannerFactory = () => new DuckStationScannerStrategy(),
                StateBasedScannerFactory = () => new DuckStationStateBasedScannerStrategy(),
                SupportsStaticRangeFinder = false
            },
            new EmulatorProfile
            {
                Name = "RALibretro (NDS)",
                Target = EmulatorTarget.RALibretroNDS,
                ProcessNames = new[] { "RALibretro" },
                ManagerFactory = () => new RALibretroNDSManager(),
                ScannerFactory = () => new RALibretroNDSScannerStrategy(),
                StateBasedScannerFactory = () => new RALibretroNDSStateBasedScannerStrategy(),
                SupportsStaticRangeFinder = true,
                StaticRangeFinderFactory = (manager) => new NdsStaticRangeFinderForm(manager)
            },
            // Added the new profile for Dolphin.
            new EmulatorProfile
            {
                Name = "Dolphin",
                Target = EmulatorTarget.Dolphin,
                ProcessNames = new[] { "Dolphin" },
                ManagerFactory = () => new DolphinManager(),
                ScannerFactory = () => new DolphinLiveScannerStrategy(),
                StateBasedScannerFactory = () => new DolphinStateBasedScannerStrategy(),
                SupportsStaticRangeFinder = true,
                StaticRangeFinderFactory = (manager) => new DolphinFileRangeFinderForm(manager)
            },
            // Added the new profile for PPSSPP.
            new EmulatorProfile
            {
                Name = "PPSSPP",
                Target = EmulatorTarget.PPSSPP,
                ProcessNames = new[] { "PPSSPPWindows64" },
                ManagerFactory = () => new PpssppManager(),
                ScannerFactory = () => new PpssppLiveScannerStrategy(),
                StateBasedScannerFactory = () => new PpssppStateBasedScannerStrategy(),
                SupportsStaticRangeFinder = true,
                StaticRangeFinderFactory = (manager) => new PpssppRamScanRangeFinderForm(manager)
            }
        };
    }
}