using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators
{
    // Defines the contract for a pointer scanning algorithm.
    // This allows different emulators to have completely different scanning logic
    // while still being controlled by the main application in a consistent way.
    public interface IPointerScannerStrategy
    {
        // The main method that executes the pointer scan.
        // It must be asynchronous to keep the UI responsive and support cancellation.
        Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken);
    }
}