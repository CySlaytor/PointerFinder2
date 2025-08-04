using PointerFinder2.Core;
using PointerFinder2.Emulators.DuckStation;
using PointerFinder2.Emulators.PCSX2;
using PointerFinder2.Emulators.RALibretro;
using System.Collections.Generic;

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
            // Profile for PCSX2.
            new EmulatorProfile
            {
                Name = "PCSX2",
                Target = EmulatorTarget.PCSX2,
                ProcessNames = new[] { "pcsx2-qt", "pcsx2" },
                ManagerFactory = () => new Pcsx2Manager(),
                ScannerFactory = () => new Pcsx2ScannerStrategy()
            },
            // Profile for DuckStation.
            new EmulatorProfile
            {
                Name = "DuckStation",
                Target = EmulatorTarget.DuckStation,
                ProcessNames = new[] { "duckstation-qt-x64-ReleaseLTCG" },
                ManagerFactory = () => new DuckStationManager(),
                ScannerFactory = () => new DuckStationScannerStrategy()
            },
            // Profile for RALibretro (NDS Cores).
            new EmulatorProfile
            {
                Name = "RALibretro (NDS)",
                Target = EmulatorTarget.RALibretroNDS,
                ProcessNames = new[] { "RALibretro" },
                ManagerFactory = () => new RALibretroNDSManager(),
                ScannerFactory = () => new RALibretroNDSScannerStrategy()
            }
        };
    }
}