using System;

namespace PointerFinder2.Emulators
{
    // A data class that defines a profile for a supported emulator.
    // This allows the application to be easily extended with new emulators.
    public class EmulatorProfile
    {
        // The user-friendly name of the emulator (e.g., "PCSX2").
        public string Name { get; set; }

        // An enum identifying the target platform for settings management.
        public EmulatorTarget Target { get; set; }

        // An array of possible process names for this emulator.
        public string[] ProcessNames { get; set; }

        // A factory function that creates an instance of the emulator's manager (IEmulatorManager).
        public Func<IEmulatorManager> ManagerFactory { get; set; }

        // A factory function that creates an instance of the emulator's scanning strategy (IPointerScannerStrategy).
        public Func<IPointerScannerStrategy> ScannerFactory { get; set; }
    }
}