using PointerFinder2.Core;
using PointerFinder2.Emulators.DuckStation;
using PointerFinder2.Emulators.PCSX2;
using System.Collections.Generic;

namespace PointerFinder2.Emulators
{
    // A static class that acts as a central registry for all supported emulator profiles.
    // This is the single point of configuration for adding or modifying emulator support.
    public static class EmulatorProfileRegistry
    {
        // A list containing the profiles for each supported emulator.
        public static readonly List<EmulatorProfile> Profiles = new List<EmulatorProfile>
        {
            // Profile for the PCSX2 emulator.
            new EmulatorProfile
            {
                Name = "PCSX2",
                Target = EmulatorTarget.PCSX2,
                ProcessNames = new[] { "pcsx2-qt", "pcsx2" },
                ManagerFactory = () => new Pcsx2Manager(),
                ScannerFactory = () => new Pcsx2ScannerStrategy()
            },
            // Profile for the DuckStation emulator.
            new EmulatorProfile
            {
                Name = "DuckStation",
                Target = EmulatorTarget.DuckStation,
                ProcessNames = new[] { "duckstation-qt-x64-ReleaseLTCG" },
                ManagerFactory = () => new DuckStationManager(),
                ScannerFactory = () => new DuckStationScannerStrategy()
            }
        };
    }
}