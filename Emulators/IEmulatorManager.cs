using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;

namespace PointerFinder2.Emulators
{
    // Defines the contract for an emulator manager.
    // Each supported emulator needs a class that implements this interface to abstract away
    // the details of how to interact with its process, memory, and addresses.
    public interface IEmulatorManager
    {
        // The user-friendly name of the emulator (e.g., "PCSX2").
        string EmulatorName { get; }
        // The total size of the primary scannable memory region (e.g., PS2's 32MB EE RAM).
        uint MainMemorySize { get; }
        // The prefix for RetroAchievements memory addresses ("X" for PS2, "W" for PS1).
        string RetroAchievementsPrefix { get; }
        // The Process object for the attached emulator.
        Process EmulatorProcess { get; }
        // A boolean indicating if the manager is currently attached.
        bool IsAttached { get; }
        // The handle to the emulator process for memory operations.
        nint ProcessHandle { get; }
        // The base address of the emulated RAM within the host PC's memory.
        nint MemoryBasePC { get; }
        // Attaches to a specific instance of the emulator process.
        bool Attach(Process process);
        // Detaches from the emulator process.
        void Detach();
        // Reads a block of memory from the emulated system.
        byte[] ReadMemory(uint address, int count);
        // Reads a 32-bit unsigned integer from an emulated address.
        uint? ReadUInt32(uint address);
        // Validates a pointer path by re-calculating its final address.
        uint? RecalculateFinalAddress(PointerPath path, uint expectedFinalAddress);
        // Checks if a value is a valid pointer target in the emulated system.
        bool IsValidPointerTarget(uint value);
        // Formats a full emulated address into a shorter, user-friendly string.
        string FormatDisplayAddress(uint address);
        // Converts a user-entered address into a full emulated address.
        uint UnnormalizeAddress(string address);
        // Provides a set of default settings for this emulator.
        AppSettings GetDefaultSettings();
    }
}