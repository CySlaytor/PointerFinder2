using System.Collections.Generic;
using System.Windows.Forms;

namespace PointerFinder2.DataModels
{
    // Represents all the data required to save and load a pointer finding session.
    public class SessionData
    {
        // The target name of the emulator used for the session (e.g., "PCSX2").
        public string EmulatorTargetName { get; set; }

        // The process ID of the emulator instance that was attached.
        public int ProcessId { get; set; }

        // The parameters used for the last scan that produced the results.
        public ScanParameters LastScanParameters { get; set; }

        // The list of pointer paths found in the session.
        public List<PointerPath> Results { get; set; }

        // The name of the column the results were sorted by.
        public string SortedColumnName { get; set; }

        // The direction of the sort.
        public SortOrder SortDirection { get; set; }
    }
}