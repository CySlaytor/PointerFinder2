using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;

namespace PointerFinder2.Emulators
{
    // Defines the contract for an emulator manager.
    // Each supported emulator must have a class that implements this interface.
    // It abstracts away the specific details of how to interact with an emulator's process.
    public interface IEmulatorManager
    {
        // The user-friendly name of the emulator (e.g., "PCSX2").
        string EmulatorName { get; }
        // The prefix used for RetroAchievements memory addresses ("X" for PS2, "W" for PS1).
        string RetroAchievementsPrefix { get; }
        // The System.Diagnostics.Process object for the attached emulator.
        Process EmulatorProcess { get; }
        // A boolean indicating if the manager is currently attached to a process.
        bool IsAttached { get; }
        // The handle to the emulator process, used for memory operations.
        nint ProcessHandle { get; }
        // The base address of the emulated RAM within the host PC's memory space.
        nint MemoryBasePC { get; }
        // Attaches to a specific instance of the emulator process.
        bool Attach(Process process);
        // Detaches from the emulator process and cleans up resources.
        void Detach();
        // Reads a block of memory from an address in the emulated system's memory map.
        byte[] ReadMemory(uint address, int count);
        // Reads a 32-bit unsigned integer from an emulated address.
        uint? ReadUInt32(uint address);
        // Validates a pointer path by re-calculating its final address in the current memory state.
        uint? RecalculateFinalAddress(PointerPath path, uint expectedFinalAddress);
        // Checks if a given value is a valid target for a pointer in the emulated system.
        bool IsValidPointerTarget(uint value);
        // Formats a full emulated memory address into a shorter, user-friendly string.
        string FormatDisplayAddress(uint address);
        // Converts a user-entered address (potentially in short format) into a full emulated memory address.
        uint UnnormalizeAddress(string address);
        // Provides a set of default settings tailored for this specific emulator.
        AppSettings GetDefaultSettings();
    }
}