using System;
using System.Windows.Forms;

namespace PointerFinder2.Emulators
{
    // A data class that defines a profile for a supported emulator.
    // This allows the application to be easily extended with new emulators
    // by simply creating a new profile and its associated manager/scanner classes.
    public class EmulatorProfile
    {
        // The user-friendly name of the emulator (e.g., "PCSX2").
        public string Name { get; set; }

        // An enum uniquely identifying the target, mainly for settings management.
        public EmulatorTarget Target { get; set; }

        // An array of possible process names for this emulator.
        public string[] ProcessNames { get; set; }

        // A factory function that creates an instance of the emulator's manager.
        public Func<IEmulatorManager> ManagerFactory { get; set; }

        // A factory function that creates an instance of the emulator's live-memory scanning strategy.
        public Func<IPointerScannerStrategy> ScannerFactory { get; set; }

        // A factory function that creates an instance of the emulator's state-based scanning strategy.
        public Func<IPointerScannerStrategy> StateBasedScannerFactory { get; set; }

        // Added properties to make UI feature availability data-driven.
        // If true, this profile supports a tool for finding the static memory range.
        public bool SupportsStaticRangeFinder { get; set; } = false;

        // A factory function that creates an instance of the static range finder form.
        public Func<IEmulatorManager, Form> StaticRangeFinderFactory { get; set; }
    }
}