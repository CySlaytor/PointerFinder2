using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators
{
    public interface IPointerScannerStrategy
    {
        Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken);
    }
}