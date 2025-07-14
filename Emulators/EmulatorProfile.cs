using System;

namespace PointerFinder2.Emulators
{
    public class EmulatorProfile
    {
        public string Name { get; set; }
        public EmulatorTarget Target { get; set; }
        public string[] ProcessNames { get; set; }
        public Func<IEmulatorManager> ManagerFactory { get; set; }
        public Func<IPointerScannerStrategy> ScannerFactory { get; set; }
    }
}