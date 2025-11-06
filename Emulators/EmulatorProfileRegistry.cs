using PointerFinder2.Emulators.EmulatorManager;
using PointerFinder2.Emulators.LiveScan.DuckStation;
using PointerFinder2.Emulators.LiveScan.PCSX2;
using PointerFinder2.Emulators.LiveScan.RALibretro;
using PointerFinder2.Emulators.StateBased.DuckStation;
using PointerFinder2.Emulators.StateBased.PCSX2;
using PointerFinder2.Emulators.StateBased.RALibretro;
using System.Collections.Generic;
using PointerFinder2.Emulators.LiveScan.Dolphin;
using PointerFinder2.Emulators.StateBased.Dolphin;
// Added using statements for the new PPSSPP scanner strategies.
using PointerFinder2.Emulators.LiveScan.PPSSPP;
using PointerFinder2.Emulators.StateBased.PPSSPP;

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
            new EmulatorProfile
            {
                Name = "PCSX2",
                Target = EmulatorTarget.PCSX2,
                ProcessNames = new[] { "pcsx2-qt", "pcsx2" },
                ManagerFactory = () => new Pcsx2Manager(),
                ScannerFactory = () => new Pcsx2ScannerStrategy(),
                StateBasedScannerFactory = () => new Pcsx2StateBasedScannerStrategy()
            },
            new EmulatorProfile
            {
                Name = "DuckStation",
                Target = EmulatorTarget.DuckStation,
                ProcessNames = new[] { "duckstation-qt-x64-ReleaseLTCG" },
                ManagerFactory = () => new DuckStationManager(),
                ScannerFactory = () => new DuckStationScannerStrategy(),
                StateBasedScannerFactory = () => new DuckStationStateBasedScannerStrategy()
            },
            new EmulatorProfile
            {
                Name = "RALibretro (NDS)",
                Target = EmulatorTarget.RALibretroNDS,
                ProcessNames = new[] { "RALibretro" },
                ManagerFactory = () => new RALibretroNDSManager(),
                ScannerFactory = () => new RALibretroNDSScannerStrategy(),
                StateBasedScannerFactory = () => new RALibretroNDSStateBasedScannerStrategy()
            },
            // Added the new profile for Dolphin.
            new EmulatorProfile
            {
                Name = "Dolphin",
                Target = EmulatorTarget.Dolphin,
                ProcessNames = new[] { "Dolphin" },
                ManagerFactory = () => new DolphinManager(),
                ScannerFactory = () => new DolphinLiveScannerStrategy(),
                StateBasedScannerFactory = () => new DolphinStateBasedScannerStrategy()
            },
            // Added the new profile for PPSSPP.
            new EmulatorProfile
            {
                Name = "PPSSPP",
                Target = EmulatorTarget.PPSSPP,
                ProcessNames = new[] { "PPSSPPWindows64" },
                ManagerFactory = () => new PpssppManager(),
                ScannerFactory = () => new PpssppLiveScannerStrategy(),
                StateBasedScannerFactory = () => new PpssppStateBasedScannerStrategy()
            }
        };
    }
}